#!/usr/bin/env python3
"""
BLE Remote Receiver v2 - Raspberry Pi

Supports both firmware variants:
  - "BLE-Remote" (Arduino/NimBLE on ESP32-C3) — custom service UUID
  - "EasyPlay"   (MicroPython on ESP32-S3)    — Nordic UART Service

Key fix: proper BlueZ cache clearing (stop → delete → restart) to prevent
stale cached GATT data from causing connection timeouts. BlueZ aggressively
caches device names, GATT tables, and connection parameters in memory.
Deleting on-disk cache while bluetoothd is running does nothing — the daemon
just regenerates it from its in-memory copy.
"""

import asyncio
import struct
import subprocess
import sys
from datetime import datetime

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

# ── Device profiles ──────────────────────────────────────────────────────────
# BLE-Remote (C3, Arduino/NimBLE)
BLEREMOTE_NAME       = "BLE-Remote"
BLEREMOTE_SERVICE    = "4e520001-7354-4288-9a71-81a9bf56c4a8"
BLEREMOTE_CHAR       = "4e520002-7354-4288-9a71-81a9bf56c4a8"
BLEREMOTE_MAC        = "38:44:BE:45:AD:86"

# EasyPlay (S3, MicroPython) — Nordic UART Service
EASYPLAY_NAME        = "EasyPlay"
EASYPLAY_SERVICE     = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
EASYPLAY_TX_CHAR     = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # notifications from ESP32
EASYPLAY_MAC         = None  # set after first scan

# Which device we're connected to (detected at scan time)
active_profile = None  # "bleremote" or "easyplay"

# All known names and UUIDs to match during scanning
KNOWN_NAMES    = {BLEREMOTE_NAME, EASYPLAY_NAME}
KNOWN_SERVICES = {BLEREMOTE_SERVICE.lower(), EASYPLAY_SERVICE.lower()}
KNOWN_MACS     = {BLEREMOTE_MAC}

# ── Timing ───────────────────────────────────────────────────────────────────
SCAN_TIMEOUT       = 15.0    # longer scan — S3 may be waking from sleep
CONNECT_TIMEOUT    = 15.0
RECONNECT_DELAY    = 1.0
MAX_RECONNECT_DELAY = 10.0

# State
connected_address = None


def ts():
    """Timestamp prefix for log lines."""
    return datetime.now().strftime("[%H:%M:%S.%f")[:-3] + "]"


def log(msg, level="INFO"):
    """Print a timestamped log line."""
    print(f"{ts()} [{level:5s}] {msg}", flush=True)


BUTTON_NAMES = {
    'L': 'LEFT press',  'l': 'LEFT release',
    'R': 'RIGHT press', 'r': 'RIGHT release',
    'U': 'UP press',    'u': 'UP release',
    'D': 'DOWN press',  'd': 'DOWN release',
    'O': 'ON/OFF press','o': 'ON/OFF release',
}


def notification_handler(characteristic, data: bytearray):
    """Called for every BLE notification received."""
    # Single byte = button event (ASCII char) — EasyPlay sends these
    if len(data) == 1:
        char = chr(data[0])
        name = BUTTON_NAMES.get(char, f"unknown({char})")
        log(f"BUTTON: '{char}' -> {name}")
    # 4 bytes = heartbeat counter — BLE-Remote sends these
    elif len(data) == 4:
        value = struct.unpack("<I", data)[0]
        log(f"HEARTBEAT #{value}")
    else:
        log(f"NOTIFICATION: raw={data.hex()} ({len(data)} bytes)")


def disconnected_callback(client: BleakClient):
    """Called when the BLE connection drops."""
    log(f"DISCONNECTED from {client.address}", "WARN")


# ── BlueZ cache management ──────────────────────────────────────────────────

async def nuclear_cache_clear():
    """Stop bluetoothd, delete ALL on-disk caches, restart.

    This is the ONLY reliable way to clear BlueZ cache. The daemon keeps
    everything in memory, so deleting files while it's running is useless —
    it regenerates them immediately from its in-memory copy.

    This fixes:
      - Stale device names (e.g. "EasyPlay" showing for a device now named "BLE-Remote")
      - Stale GATT service tables causing connection timeouts
      - Stale connection parameters from previous firmware versions
    """
    log("Nuclear cache clear: stop bluetooth -> delete cache -> restart...")
    try:
        subprocess.run(["sudo", "systemctl", "stop", "bluetooth"],
                       capture_output=True, timeout=10)
        await asyncio.sleep(0.5)
        # Delete ALL cached device data while daemon is stopped
        subprocess.run(["sudo", "bash", "-c", "rm -rf /var/lib/bluetooth/*/cache/*"],
                       capture_output=True, timeout=5)
        subprocess.run(["sudo", "systemctl", "start", "bluetooth"],
                       capture_output=True, timeout=10)
        await asyncio.sleep(3)  # BlueZ LE subsystem needs time to initialize
        log("Nuclear cache clear complete")
    except Exception as e:
        log(f"Nuclear cache clear failed: {e}", "ERROR")
        # Fallback: try just power cycling the adapter
        await adapter_reset()


async def adapter_reset():
    """Quick adapter reset — use between connection attempts."""
    log("Adapter reset (power off/on)...")
    try:
        subprocess.run(["bluetoothctl", "power", "off"],
                       capture_output=True, timeout=5)
        await asyncio.sleep(1)
        subprocess.run(["bluetoothctl", "power", "on"],
                       capture_output=True, timeout=5)
        await asyncio.sleep(2)
        log("Adapter reset complete")
    except Exception as e:
        log(f"Adapter reset failed: {e}", "ERROR")


async def remove_device(address):
    """Remove a specific device from BlueZ to force fresh GATT discovery."""
    log(f"Removing {address} from BlueZ...")
    try:
        result = subprocess.run(
            ["bluetoothctl", "remove", address],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            log(f"  Removed from BlueZ")
        else:
            log(f"  Not in cache (OK)", "DEBUG")
        await asyncio.sleep(0.3)
    except Exception:
        pass


# ── Scanning ─────────────────────────────────────────────────────────────────

async def scan_for_device():
    """Scan for any known ESP32 remote. Returns (device, profile_name) or (None, None).

    Matches by: service UUID, device name, or known MAC address.
    Supports both BLE-Remote and EasyPlay firmware.
    """
    global active_profile

    await adapter_reset()

    log(f"Scanning for BLE devices (max {SCAN_TIMEOUT}s)...")
    log(f"  Looking for names: {KNOWN_NAMES}")
    log(f"  Looking for services: {KNOWN_SERVICES}")

    target_device = None
    target_profile = None
    devices_found = 0
    scan_done = asyncio.Event()

    def detection_callback(device, adv):
        nonlocal target_device, target_profile, devices_found
        devices_found += 1

        adv_uuids = {str(u).lower() for u in (adv.service_uuids or [])}
        has_our_uuid = bool(adv_uuids & KNOWN_SERVICES)
        is_known_mac = device.address and device.address.upper() in KNOWN_MACS
        is_known_name = (device.name in KNOWN_NAMES) or (adv.local_name in KNOWN_NAMES if adv.local_name else False)

        if has_our_uuid or is_known_mac or is_known_name:
            # Determine which profile
            if BLEREMOTE_SERVICE.lower() in adv_uuids or device.name == BLEREMOTE_NAME:
                target_profile = "bleremote"
            elif EASYPLAY_SERVICE.lower() in adv_uuids or device.name == EASYPLAY_NAME:
                target_profile = "easyplay"
            else:
                target_profile = "bleremote"  # default

            match_reason = "UUID" if has_our_uuid else ("MAC" if is_known_mac else "name")
            log(f"  ** MATCH ({match_reason}, profile={target_profile}) ** "
                f"{device.address} name={device.name!r} "
                f"RSSI={adv.rssi}dBm UUIDs={list(adv_uuids)}")
            target_device = device
            scan_done.set()
        elif devices_found <= 8:
            log(f"  {device.address} name={device.name!r} RSSI={adv.rssi}dBm", "DEBUG")

    scanner = BleakScanner(
        detection_callback=detection_callback,
        scanning_mode="active",
    )
    await scanner.start()
    try:
        await asyncio.wait_for(scan_done.wait(), timeout=SCAN_TIMEOUT)
        log(f"Device found after scanning {devices_found} devices")
    except asyncio.TimeoutError:
        log(f"Scan timeout: {devices_found} devices seen, target not found", "WARN")
    await scanner.stop()

    if target_device:
        active_profile = target_profile
        log(f"Target: {target_device.name} ({target_device.address}) profile={active_profile}")
        return target_device

    log("Device not found. Note: EasyPlay sleeps after 3min — press a button to wake it!", "WARN")
    return None


# ── Connection ───────────────────────────────────────────────────────────────

async def connect_and_listen(device):
    """Connect, subscribe to notifications, and listen until disconnect."""
    global connected_address, active_profile

    # Remove device from BlueZ before connecting — forces fresh GATT discovery
    # This prevents stale cached GATT tables from causing timeouts
    await remove_device(device.address)
    await asyncio.sleep(0.5)

    # Pick the right characteristic UUID based on detected profile
    if active_profile == "easyplay":
        char_uuid = EASYPLAY_TX_CHAR
    else:
        char_uuid = BLEREMOTE_CHAR

    log(f"Connecting to {device.address} (profile={active_profile}, timeout={CONNECT_TIMEOUT}s)...")

    async with BleakClient(
        device,
        disconnected_callback=disconnected_callback,
        timeout=CONNECT_TIMEOUT,
    ) as client:
        log("========== CONNECTED ==========")
        log(f"  Address: {client.address}")
        log(f"  MTU: {client.mtu_size}")
        log(f"  Profile: {active_profile}")
        connected_address = client.address

        # List services for debug
        log("  Services:")
        for service in client.services:
            log(f"    [{service.uuid}] {service.description}")
            for char in service.characteristics:
                props = ", ".join(char.properties)
                log(f"      [{char.uuid}] ({props})")

        # Subscribe to notifications
        log(f"Subscribing to {char_uuid}...")
        await client.start_notify(char_uuid, notification_handler)
        log("Subscribed — listening for notifications")
        log("--------------------------------------------")

        # Stay connected
        while client.is_connected:
            await asyncio.sleep(5.0)
            if client.is_connected:
                log(f"STATUS: connected=yes mtu={client.mtu_size} profile={active_profile}")

    log("Connection closed")


# ── Main loop ────────────────────────────────────────────────────────────────

async def main():
    """Main reconnection loop with proper BlueZ cache management."""
    print()
    print("============================================")
    print("  BLE Remote Receiver v2 — Raspberry Pi")
    print("  Supports: BLE-Remote + EasyPlay")
    print("============================================")
    log(f"Known names:    {KNOWN_NAMES}")
    log(f"Known services: {KNOWN_SERVICES}")
    log(f"Known MACs:     {KNOWN_MACS}")

    # Nuclear cache clear on startup — THE critical fix
    # This prevents stale BlueZ data from previous sessions
    await nuclear_cache_clear()
    for mac in KNOWN_MACS:
        await remove_device(mac)
    print("--------------------------------------------\n")

    global connected_address
    reconnect_delay = RECONNECT_DELAY
    consecutive_failures = 0
    connected_address = None

    while True:
        try:
            # After 3 consecutive failures, nuclear cache clear
            if consecutive_failures >= 3:
                log(f"Recovery: nuclear cache clear after {consecutive_failures} failures", "WARN")
                await nuclear_cache_clear()
                if connected_address:
                    await remove_device(connected_address)
                connected_address = None
                consecutive_failures = 0

            # Scan for device
            device = await scan_for_device()
            if device is None:
                consecutive_failures += 1
                log(f"Retrying in {reconnect_delay:.0f}s...", "WARN")
                await asyncio.sleep(reconnect_delay)
                reconnect_delay = min(reconnect_delay * 1.5, MAX_RECONNECT_DELAY)
                continue

            # Reset backoff on successful scan
            reconnect_delay = RECONNECT_DELAY

            # Connect and listen
            await connect_and_listen(device)

            # Connection was established then closed — do adapter reset before reconnecting
            # This clears any stale connection state in BlueZ
            await adapter_reset()
            consecutive_failures = 0

        except BleakError as e:
            consecutive_failures += 1
            log(f"BLE error: {e}", "ERROR")
            if "InProgress" in str(e) or "in progress" in str(e).lower():
                log("BlueZ stuck — nuclear cache clear...", "WARN")
                await nuclear_cache_clear()
                continue
        except asyncio.TimeoutError:
            consecutive_failures += 1
            log("Connection timed out", "ERROR")
        except OSError as e:
            consecutive_failures += 1
            log(f"OS error: {e}", "ERROR")
        except asyncio.CancelledError:
            log("Shutting down...")
            break

        log(f"Reconnecting in {reconnect_delay:.0f}s...")
        await asyncio.sleep(reconnect_delay)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print(f"\n{ts()} [INFO ] Ctrl+C received, exiting.")
        sys.exit(0)
