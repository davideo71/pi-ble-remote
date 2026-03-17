#!/usr/bin/env python3
"""Raw BLE scan - reads actual advertisement data, bypasses BlueZ name cache."""

import asyncio
from bleak import BleakScanner
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData

TARGET_MAC = "38:44:BE:45:AD:86"
OUR_UUID = "4e520001-7354-4288-9a71-81a9bf56c4a8"

found_devices = {}

def callback(device: BLEDevice, adv: AdvertisementData):
    mac = device.address.upper()

    # Show ALL devices with our service UUID, or our target MAC
    dominated = False
    if mac == TARGET_MAC or OUR_UUID in [str(u).lower() for u in (adv.service_uuids or [])]:
        print(f"\n{'='*60}")
        print(f"  *** TARGET/MATCH FOUND ***")
        print(f"  MAC:  {mac}")
        print(f"  Name from BlueZ:    {device.name}")
        print(f"  Name from adv data: {adv.local_name}")
        print(f"  RSSI: {adv.rssi} dBm")
        print(f"  Service UUIDs: {adv.service_uuids}")
        print(f"  Manufacturer data: {adv.manufacturer_data}")
        print(f"  Service data: {adv.service_data}")
        print(f"  TX power: {adv.tx_power}")
        print(f"{'='*60}")
        found_devices[mac] = {
            'bluez_name': device.name,
            'adv_name': adv.local_name,
            'rssi': adv.rssi,
            'uuids': adv.service_uuids
        }

async def main():
    print("Raw BLE scan - 15 seconds")
    print(f"Looking for MAC {TARGET_MAC} and UUID {OUR_UUID}")
    print("-" * 60)

    scanner = BleakScanner(detection_callback=callback)
    await scanner.start()
    await asyncio.sleep(15)
    await scanner.stop()

    print("\n\n=== SUMMARY ===")
    if not found_devices:
        print("No matching devices found!")
    for mac, info in found_devices.items():
        print(f"\nMAC: {mac}")
        print(f"  BlueZ name:        {info['bluez_name']}")
        print(f"  Advertisement name: {info['adv_name']}")
        print(f"  RSSI: {info['rssi']}")
        print(f"  UUIDs: {info['uuids']}")
        if info['bluez_name'] != info['adv_name']:
            print(f"  *** NAME MISMATCH! BlueZ is caching an old name! ***")

if __name__ == "__main__":
    asyncio.run(main())
