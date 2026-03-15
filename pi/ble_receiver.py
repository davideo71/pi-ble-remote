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

# Reconnection settings
SCAN_TIMEOUT = 10.0        # seconds to scan before retrying
CONNECT_TIMEOUT = 15.0     # seconds to wait for connection
RECONNECT_DELAY = 3.0      # seconds between reconnection attempts
MAX_RECONNECT_DELAY = 15.0 # max backoff delay

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
    """Scan for our BLE remote device. Returns the device or None."""
    global connected_address

    # If we've connected before, try by address first (faster)
    if connected_address:
        log(f"Scanning for known address {connected_address}...")
        device = await BleakScanner.find_device_by_address(
            connected_address, timeout=SCAN_TIMEOUT / 2
        )
        if device:
            log(f"Found by address: {device.name} ({device.address})")
            return device
        log("Known address not found, falling back to service UUID scan", "WARN")

    # Remove stale BlueZ cache BEFORE scanning so the device is discovered fresh as LE
    # Always remove — BlueZ may have it cached as BR/EDR from old firmware ("EasyPlay")
    addr_to_remove = connected_address or KNOWN_MAC
    await remove_bluez_device(addr_to_remove)

    # Scan by service UUID (more reliable than name — BlueZ caches names)
    log(f"Scanning for BLE devices (timeout={SCAN_TIMEOUT}s, matching UUID in callback)...")

    devices_found = 0
    target_device = None

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
        elif devices_found <= 5:
            log(f"  SCAN: {device.address} name={device.name!r} RSSI={advertisement_data.rssi}dBm", "DEBUG")

    # No service_uuids filter — BlueZ D-Bus filter is unreliable with NimBLE's
    # advertisement format. Instead we match by UUID in the detection callback above.
    scanner = BleakScanner(
        detection_callback=detection_callback,
    )
    await scanner.start()
    await asyncio.sleep(SCAN_TIMEOUT)
    await scanner.stop()

    log(f"Scan complete: {devices_found} devices seen")

    if target_device:
        log(f"Found target by service UUID: {target_device.name} ({target_device.address})")
        return target_device

    # Fallback: also check by name in case service UUID wasn't in advertisement
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
    await asyncio.sleep(0.5)


async def connect_and_listen(device):
    """Connect to the device, subscribe, and listen until disconnect."""
    global connected_address

    log(f"Connecting to {device.address} (timeout={CONNECT_TIMEOUT}s)...")

    # Note: we no longer remove the BlueZ cache here — that destroys the D-Bus
    # device object bleak needs. The service_uuids scanner filter already ensures
    # BlueZ creates a proper LE device entry.
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
        notification_count = 0
        while client.is_connected:
            await asyncio.sleep(5.0)
            if client.is_connected:
                log(f"STATUS: connected={client.is_connected} mtu={client.mtu_size}")

    log("Connection closed (BleakClient exited)")


async def reset_bluetooth_adapter():
    """Reset the BlueZ adapter to recover from stuck states."""
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

    while True:
        try:
            # Scan
            device = await scan_for_device()
            if device is None:
                log(f"Retrying in {reconnect_delay:.0f}s...", "WARN")
                await asyncio.sleep(reconnect_delay)
                reconnect_delay = min(reconnect_delay * 1.5, MAX_RECONNECT_DELAY)
                continue

            # Reset backoff on successful scan
            reconnect_delay = RECONNECT_DELAY

            # Connect and listen
            await connect_and_listen(device)

        except BleakError as e:
            error_str = str(e)
            log(f"BLE error: {e}", "ERROR")
            if "InProgress" in error_str or "in progress" in error_str.lower():
                log("BlueZ stuck — resetting Bluetooth adapter...", "WARN")
                await reset_bluetooth_adapter()
        except asyncio.TimeoutError:
            log("Connection timed out", "ERROR")
        except OSError as e:
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
