#!/usr/bin/env python3
"""
BLE Remote Receiver - Raspberry Pi

Step 1: Connect to ESP32-C3 "BLE-Remote", subscribe to heartbeat
notifications, and log everything with verbose debug output.
Reconnects automatically on disconnect.
"""

import asyncio
import struct
import signal
import sys
from datetime import datetime

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

# Must match the ESP32 firmware
DEVICE_NAME = "BLE-Remote"
SERVICE_UUID = "4e520001-7354-4288-9a71-81a9bf56c4a8"
BUTTON_CHAR_UUID = "4e520002-7354-4288-9a71-81a9bf56c4a8"

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
        log("Known address not found, falling back to name scan", "WARN")

    # Scan by name
    log(f"Scanning for device named '{DEVICE_NAME}' (timeout={SCAN_TIMEOUT}s)...")

    devices_found = 0

    def detection_callback(device, advertisement_data):
        nonlocal devices_found
        devices_found += 1
        if devices_found <= 10 or (device.name and "BLE" in (device.name or "")):
            log(f"  SCAN: {device.address} name={device.name!r} RSSI={advertisement_data.rssi}dBm", "DEBUG")

    scanner = BleakScanner(detection_callback=detection_callback)
    await scanner.start()
    await asyncio.sleep(SCAN_TIMEOUT)
    await scanner.stop()

    log(f"Scan complete: {devices_found} devices seen")

    # Find our device in results
    for d in scanner.discovered_devices:
        if d.name == DEVICE_NAME:
            log(f"Found target: {d.name} ({d.address})")
            return d

    log(f"Device '{DEVICE_NAME}' not found in scan results", "WARN")
    return None


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
        notification_count = 0
        while client.is_connected:
            await asyncio.sleep(5.0)
            if client.is_connected:
                log(f"STATUS: connected={client.is_connected} mtu={client.mtu_size}")

    log("Connection closed (BleakClient exited)")


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
            log(f"BLE error: {e}", "ERROR")
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
