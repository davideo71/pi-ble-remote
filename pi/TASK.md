# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 43: Clean init + name in adv data (triggered 2026-03-17 23:15)

### Context
GREAT WORK solving the "EasyPlay" mystery! It was BlueZ cache regenerating during scans.

This firmware has:
- Clean single `NimBLEDevice::init("BLE-Remote")` (no deinit dance)
- `pAdvertising->setName("BLE-Remote")` to put name in actual advertising packets
- TX power at max (+9 dBm)

**IMPORTANT: Proper cache clearing sequence:**
1. `sudo systemctl stop bluetooth`
2. `sudo rm -rf /var/lib/bluetooth/*/cache/`
3. `sudo systemctl start bluetooth && sleep 3`
(Stop FIRST, delete, THEN start — so cache doesn't regenerate from memory)

Also, if possible, can the user **move the C3 closer to the Pi**? RSSI -87 to -91 is marginal.

BLE MAC: `38:44:BE:45:AD:86` (base MAC +2).

### Steps
1. `git pull`
2. Proper cache clear (stop → delete → start, as above)
3. `sudo bluetoothctl remove 38:44:BE:45:AD:86` (ignore errors)
4. Run `python3 pi/raw_scan.py` — should now see "BLE-Remote" in adv data
5. If found — connect with `ble_receiver.py` for 2 minutes
6. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device advertises as "BLE-Remote" in advertisement data
- Connected and received 30+ heartbeats
