# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 42: deinit(true) + clean build (triggered 2026-03-17 23:10)

### Context
Binary was verified clean (no "EasyPlay" string), yet device still advertised "EasyPlay". This time:
1. Deleted Arduino build cache entirely
2. Added `NimBLEDevice::deinit(true)` before init — this specifically erases NimBLE's NVS namespace
3. Clean compile with `--clean` flag
4. Full flash erase + fresh upload

The firmware now does: `init("") → deinit(true) → delay(500) → init("BLE-Remote")` to ensure NimBLE's NVS is wiped before setting the name.

BLE MAC: `38:44:BE:45:AD:86` (base MAC +2).

### Steps
1. `git pull`
2. `sudo systemctl restart bluetooth && sleep 3`
3. `sudo rm -rf /var/lib/bluetooth/*/cache/`
4. `sudo bluetoothctl remove 38:44:BE:45:AD:86` (ignore errors)
5. Run `python3 pi/raw_scan.py` — check advertisement name
6. If "BLE-Remote" found — connect with `ble_receiver.py` for 2 minutes
7. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Advertisement name is "BLE-Remote" (not "EasyPlay")
- Connected and received 30+ heartbeats
