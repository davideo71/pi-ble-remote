# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 44: Fix adv name + clean NVS (triggered 2026-03-17 23:20)

### Context
Name still showed "None" — `setName()` on the advertising object wasn't enough. Now using `NimBLEAdvertisementData` objects to explicitly set scan response data with the name.

Also did full flash erase before upload to clear any corrupted NVS from the deinit(true) experiment in Test 42.

The updated `ble_receiver.py` now does proper cache clearing on startup (stop→delete→start).

BLE MAC: `38:44:BE:45:AD:86` (base MAC +2).

### Steps
1. `git pull`
2. **Use the updated `ble_receiver.py`** — it now handles cache clearing automatically on startup
3. Run `python3 pi/ble_receiver.py` for 2 minutes
4. Also run `python3 pi/raw_scan.py` to check if name now appears
5. **If possible, ask the user to move the C3 closer** — RSSI -83 to -87 may be too weak for GATT
6. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device name "BLE-Remote" visible in scan response
- Connected and received 30+ heartbeats
