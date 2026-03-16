#!/usr/bin/env python3
"""
BLE Remote Receiver - Raspberry Pi

Step 1: Connect to ESP32-C3 "BLE-Remote", subscribe to heartbeat
notifications, and log everything with verbose debug output.
Reconnects automatically on disconnect.
"""

import asyncio
import struct
import subprocess
import sys
from datetime import datetime

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

# Must match the ESP32 firmware
DEVICE_NAME = "BLE-Remote"
SERVICE_UUID = "4e520001-7354-4288-9a71-81a9bf56c4a8"
BUTTON_CHAR_UUID = "4e520002-7354-4288-9a71-81a9bf56c4a8"
KNOWN_MAC = "38:44:BE:45:AD:86"  # ESP32-C3 BLE address (for pre-scan cache clearing)

# Timing — tuned for fast reconnection
SCAN_TIMEOUT = 8.0         # max seconds to scan (usually finds device in <1s)
CONNECT_TIMEOUT = 15.0     # seconds to wait for connection (10s caused early timeouts)
RECONNECT_DELAY = 1.0      # seconds between reconnection attempts (fast!)
MAX_RECONNECT_DELAY = 10.0 # max backoff delay

# State
connected_address = None   # Remember MAC for faster reconnect


def ts():
    """Timestamp prefix for log lines."""
    return datetime.now().strftime("[%H:%M:%S.%f]")[:-3] + "]"


def log(msg, level="INFO"):
    """Print a timestamped log line."""
    print(f"{ts()} [{level:5s}] {msg}", flush=True)


def notification_handler(characteristic, data: bytearray):
    """Called for every BLE notification received."""
    hex_str = data.hex()
    # Try to decode as uint32 heartbeat counter
    if len(data) == 4:
        value = struct.unpack("<I", data)[0]
        log(f"NOTIFICATION: raw={hex_str} decoded=heartbeat#{value}")
    else:
        log(f"NOTIFICATION: raw={hex_str} ({len(data)} bytes)")


def disconnected_callback(client: BleakClient):
    """Called when the BLE connection drops."""
    log(f"DISCONNECTED from {client.address}", "WARN")


async def scan_for_device():
    """Scan for our BLE remote device. Returns the device or None.

    Strategy for speed:
    1. If we have a cached MAC, try a quick address-based scan first (no reset needed)
    2. If that fails, do a full scan with adapter reset to clear BlueZ state
    3. Stop scanning the moment we find our device (don't wait for timeout)
    """
    global connected_address

    # Fast path: if we've connected before, try by address without resetting
    if connected_address:
        log(f"Quick scan for known address {connected_address}...")
        device = await BleakScanner.find_device_by_address(
            connected_address, timeout=3.0,
            scanning_mode="active",
        )
        if device:
            log(f"Found by address in quick scan: {device.name} ({device.address})")
            return device
        log("Quick scan failed, falling back to full scan with adapter reset", "WARN")

    # Full scan path: reset adapter to clear BlueZ duplicate filter.
    # Do NOT remove from BlueZ cache here — removal forces BlueZ to
    # re-discover from scratch and the first connection after fresh
    # discovery consistently fails (Tests 16-17). Only use removal
    # as a recovery step after multiple failed connections (see main loop).
    await reset_bluetooth_adapter()

    log(f"Scanning for BLE devices (max {SCAN_TIMEOUT}s, early exit on match)...")

    devices_found = 0
    target_device = None
    scan_done = asyncio.Event()  # Signal to stop scanning early

    def detection_callback(device, advertisement_data):
        nonlocal devices_found, target_device
        devices_found += 1

        # Match by service UUID, name, or known MAC address
        has_our_uuid = SERVICE_UUID in [str(u).lower() for u in advertisement_data.service_uuids]
        is_our_mac = device.address and device.address.upper() == KNOWN_MAC
        is_our_name = device.name in (DEVICE_NAME, "EasyPlay")  # EasyPlay = old cached name

        if has_our_uuid or is_our_mac or is_our_name:
            match_reason = "UUID" if has_our_uuid else ("MAC" if is_our_mac else "name")
            log(f"  SCAN: ** MATCH ({match_reason}) ** {device.address} name={device.name!r} "
                f"RSSI={advertisement_data.rssi}dBm UUIDs={advertisement_data.service_uuids}", "DEBUG")
            target_device = device
            scan_done.set()  # Stop scanning immediately!
        elif devices_found <= 5:
            log(f"  SCAN: {device.address} name={device.name!r} RSSI={advertisement_data.rssi}dBm", "DEBUG")

    # Active scanning triggers ESP32's scan response containing the service UUID
    scanner = BleakScanner(
        detection_callback=detection_callback,
        scanning_mode="active",
    )
    await scanner.start()

    # Wait for either: device found (early exit) or timeout
    try:
        await asyncio.wait_for(scan_done.wait(), timeout=SCAN_TIMEOUT)
        log(f"Early exit: device found after scanning {devices_found} devices")
    except asyncio.TimeoutError:
        log(f"Scan timeout: {devices_found} devices seen, target not found", "WARN")

    await scanner.stop()

    if target_device:
        log(f"Found target: {target_device.name} ({target_device.address})")
        return target_device

    # Fallback: check by name in case service UUID wasn't in advertisement
    for d in scanner.discovered_devices:
        if d.name == DEVICE_NAME:
            log(f"Found target by name: {d.name} ({d.address})")
            return d

    log(f"Device not found (tried service UUID and name '{DEVICE_NAME}')", "WARN")
    return None


async def remove_bluez_device(address):
    """Remove a device from BlueZ cache to force fresh LE connection."""
    log(f"Removing {address} from BlueZ cache...")
    result = subprocess.run(
        ["bluetoothctl", "remove", address],
        capture_output=True, text=True, timeout=5
    )
    if result.returncode == 0:
        log(f"  Removed from BlueZ cache")
    else:
        log(f"  Not in cache (OK): {result.stderr.strip()}", "DEBUG")
    await asyncio.sleep(0.3)


async def connect_and_listen(device):
    """Connect to the device, subscribe, and listen until disconnect."""
    global connected_address

    log(f"Connecting to {device.address} (timeout={CONNECT_TIMEOUT}s)...")

    async with BleakClient(
        device,
        disconnected_callback=disconnected_callback,
        timeout=CONNECT_TIMEOUT,
    ) as client:
        log("========== CONNECTED ==========")
        log(f"  Address: {client.address}")
        log(f"  MTU: {client.mtu_size}")
        connected_address = client.address

        # List services for debug
        log("  Services:")
        for service in client.services:
            log(f"    [{service.uuid}] {service.description}")
            for char in service.characteristics:
                props = ", ".join(char.properties)
                log(f"      [{char.uuid}] {char.description} ({props})")

        # Subscribe to button characteristic
        log(f"Subscribing to notifications on {BUTTON_CHAR_UUID}...")
        await client.start_notify(BUTTON_CHAR_UUID, notification_handler)
        log("Subscribed - listening for notifications")
        log("--------------------------------------------")

        # Stay connected and print a status line periodically
        while client.is_connected:
            await asyncio.sleep(5.0)
            if client.is_connected:
                log(f"STATUS: connected={client.is_connected} mtu={client.mtu_size}")

    log("Connection closed (BleakClient exited)")


async def reset_bluetooth_adapter():
    """Reset the BlueZ adapter to recover from stuck states.

    Timing: 1s off + 2s on = 3s total. Faster resets (1.5s) caused
    "No powered Bluetooth adapters found" — BlueZ LE subsystem needs
    the full 2s after power-on to initialize properly.
    """
    log("Cycling Bluetooth adapter off/on...")
    try:
        subprocess.run(["bluetoothctl", "power", "off"], capture_output=True, timeout=5)
        await asyncio.sleep(1)
        subprocess.run(["bluetoothctl", "power", "on"], capture_output=True, timeout=5)
        await asyncio.sleep(2)
        log("Bluetooth adapter reset complete")
    except Exception as e:
        log(f"Adapter reset failed: {e}", "ERROR")


async def main():
    """Main reconnection loop."""
    print()
    print("============================================")
    print("  BLE Remote Receiver - Raspberry Pi")
    print("  Step 1: Connection + Heartbeat Debug")
    print("============================================")
    log(f"Target device: {DEVICE_NAME}")
    log(f"Service UUID:  {SERVICE_UUID}")
    log(f"Char UUID:     {BUTTON_CHAR_UUID}")
    print("--------------------------------------------\n")

    reconnect_delay = RECONNECT_DELAY
    consecutive_failures = 0

    while True:
        try:
            # After 3+ consecutive failures, try clearing BlueZ cache as recovery
            if consecutive_failures >= 3:
                addr_to_remove = connected_address or KNOWN_MAC
                log(f"Recovery: removing {addr_to_remove} from BlueZ cache after {consecutive_failures} failures", "WARN")
                await remove_bluez_device(addr_to_remove)
                consecutive_failures = 0

            # Scan
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

            # If we get here, connection was established and then closed cleanly
            consecutive_failures = 0

        except BleakError as e:
            consecutive_failures += 1
            error_str = str(e)
            log(f"BLE error: {e}", "ERROR")
            if "InProgress" in error_str or "in progress" in error_str.lower():
                log("BlueZ stuck — resetting adapter and retrying immediately...", "WARN")
                await reset_bluetooth_adapter()
                continue  # Skip the reconnect delay — retry now
        except asyncio.TimeoutError:
            consecutive_failures += 1
            log("Connection timed out", "ERROR")
        except OSError as e:
            consecutive_failures += 1
            log(f"OS error: {e}", "ERROR")
        except asyncio.CancelledError:
            log("Shutting down...", "INFO")
            break

        log(f"Reconnecting in {reconnect_delay:.0f}s...")
        await asyncio.sleep(reconnect_delay)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print(f"\n{ts()} [INFO ] Ctrl+C received, exiting.")
        sys.exit(0)
