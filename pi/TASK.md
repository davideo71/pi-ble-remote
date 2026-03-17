# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. **Restart bluetooth service**: `sudo systemctl restart bluetooth && sleep 3`
3. **Do a broad BLE scan first** — we need to find the S3's actual BLE address:
   ```python
   import asyncio
   from bleak import BleakScanner
   async def scan():
       devices = await BleakScanner.discover(timeout=10.0, scanning_mode="active")
       for d in sorted(devices, key=lambda x: x.rssi, reverse=True):
           print(f"  {d.address}  RSSI={d.rssi:4d}  name={d.name}")
   asyncio.run(scan())
   ```
   Look for "BLE-Remote" by name, or any device with our service UUID `4e520001-...`. The MAC might NOT be `A0:F2:62:EC:7A:D0` — NimBLE often uses a different address than the WiFi MAC.
4. If found, update the MAC in the report and run `python3 pi/ble_receiver.py` for 2 minutes
5. Update `pi/REPORT.md` with results, commit and push

## Context
Test S3-1 failed because device wasn't found. We re-flashed and confirmed the firmware uploaded successfully, but can't get serial output from the S3's native USB to verify the BLE address. The firmware should be advertising as "BLE-Remote" with service UUID `4e520001-7354-4288-9a71-81a9bf56c4a8`.
