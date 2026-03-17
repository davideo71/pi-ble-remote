# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 38: C3 after full flash erase + reflash (triggered 2026-03-17 21:00)

### Context
The C3 was still running old "EasyPlay" firmware despite our uploads. We did a **full flash erase** (`esptool erase_flash`) then reflashed. The base MAC is `38:44:BE:45:AD:84` but the BLE address may be `38:44:BE:45:AD:86` (BLE adds +2 on some ESP32s).

The device should now advertise as **"BLE-Remote"** (not "EasyPlay").

### Steps
1. `git pull` to get latest code
2. `sudo systemctl restart bluetooth && sleep 3`
3. Do a broad BLE scan:
   ```python
   import asyncio
   from bleak import BleakScanner
   async def scan():
       devices = await BleakScanner.discover(timeout=15.0, scanning_mode="active")
       for d in sorted(devices, key=lambda x: x.rssi, reverse=True):
           print(f"  {d.address}  RSSI={d.rssi:4d}  name={d.name}")
   asyncio.run(scan())
   ```
4. Look for **"BLE-Remote"** — it may be at `...AD:84` OR `...AD:86`
5. If found:
   - Update `KNOWN_MAC` in `pi/ble_receiver.py` to match the actual BLE MAC
   - Run `python3 pi/ble_receiver.py` for 2 minutes
   - Report: connection time, heartbeat count, RSSI
6. If still showing "EasyPlay" — report that (means flash still didn't take)
7. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device advertises as "BLE-Remote" (confirms flash erase + reflash worked)
- Connected and received 30+ heartbeats
