# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — S3-2: Fresh board, fresh reboot (triggered 2026-03-17 14:10)

### Context
We switched from ESP32-C3 to a **new ESP32-S3 SuperMini** (dual-core). Just flashed heartbeat-only firmware. The S3 WiFi MAC is `e0:72:a1:e8:78:2c` but the BLE MAC may differ — scan by name.

### Steps
1. `git pull` to get latest code
2. `sudo systemctl restart bluetooth && sleep 3`
3. Do a **broad BLE scan** — look for "BLE-Remote" by name:
   ```python
   import asyncio
   from bleak import BleakScanner
   async def scan():
       devices = await BleakScanner.discover(timeout=15.0, scanning_mode="active")
       for d in sorted(devices, key=lambda x: x.rssi, reverse=True):
           print(f"  {d.address}  RSSI={d.rssi:4d}  name={d.name}")
   asyncio.run(scan())
   ```
4. If "BLE-Remote" is found:
   - Note the BLE MAC address
   - Update `KNOWN_MAC` in `pi/ble_receiver.py` to match
   - Run `python3 pi/ble_receiver.py` for 2 minutes
   - Report: connection time, heartbeat count, any disconnects
5. If NOT found after 3 scan attempts (with bluetooth restart between each):
   - Report all scan results so we can debug
6. **Commit and push REPORT.md** with results

### Success criteria
- ESP32-S3 found in scan
- Connected and received 30+ heartbeats without disconnect
