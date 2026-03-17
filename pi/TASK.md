# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 37: C3 with antenna, clean slate (triggered 2026-03-17 20:30)

### Context
We're back on the **ESP32-C3** with an external antenna. Previous test (36b) connected to the WRONG device — it found "EasyPlay" (likely an old S3 board cached by BlueZ). We need a completely clean slate.

The C3 should advertise as **"BLE-Remote"** with MAC **38:44:BE:45:AD:84**.

### CRITICAL: Clear ALL cached BLE devices first
```bash
sudo systemctl stop bluetooth
sudo rm -rf /var/lib/bluetooth/*/
sudo systemctl start bluetooth
sleep 5
```

### Steps
1. `git pull` to get latest code
2. Run the cache clearing commands above
3. Do a **broad BLE scan** first — find "BLE-Remote":
   ```python
   import asyncio
   from bleak import BleakScanner
   async def scan():
       devices = await BleakScanner.discover(timeout=15.0, scanning_mode="active")
       for d in sorted(devices, key=lambda x: x.rssi, reverse=True):
           print(f"  {d.address}  RSSI={d.rssi:4d}  name={d.name}")
   asyncio.run(scan())
   ```
4. Look for **"BLE-Remote"** specifically — ignore "EasyPlay" or any other device
5. If "BLE-Remote" found at MAC `38:44:BE:45:AD:84`:
   - Make sure `KNOWN_MAC` in `pi/ble_receiver.py` is set to `38:44:BE:45:AD:84`
   - Run `python3 pi/ble_receiver.py` for 2 minutes
   - Report: connection time, heartbeat count, RSSI, any disconnects
6. If "BLE-Remote" NOT found, report all scan results
7. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device "BLE-Remote" found (NOT "EasyPlay")
- Connected and received 30+ heartbeats without disconnect
