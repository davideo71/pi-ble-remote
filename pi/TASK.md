# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 36: C3 with antenna, fresh Pi reboot (triggered 2026-03-17 16:45)

### Context
We're back on the **ESP32-C3** but now with an **external antenna** for better range. Pi was just rebooted (clean BlueZ state). The C3 MAC should be `38:44:BE:45:AD:84` (same as before).

### Steps
1. `git pull` to get latest code
2. `sudo systemctl restart bluetooth && sleep 3` (fresh bluetooth state)
3. Run `python3 pi/ble_receiver.py` for 2 minutes
4. Report: connection time, heartbeat count, RSSI if available, any disconnects
5. If connection fails, do a broad scan to check if the device is visible:
   ```python
   import asyncio
   from bleak import BleakScanner
   async def scan():
       devices = await BleakScanner.discover(timeout=15.0, scanning_mode="active")
       for d in sorted(devices, key=lambda x: x.rssi, reverse=True):
           print(f"  {d.address}  RSSI={d.rssi:4d}  name={d.name}")
   asyncio.run(scan())
   ```
6. **Commit and push REPORT.md** with results — Mac-Claude is waiting!

### Success criteria
- Connected on first or second attempt
- 30+ heartbeats without disconnect
- Better RSSI than previous tests (was around -89 dBm)
