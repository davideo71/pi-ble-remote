# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 40b: Raw advertisement scan (triggered 2026-03-17 22:35)

### Context
We confirmed "EasyPlay" at `38:44:BE:45:AD:86` disappears when the C3 is unplugged — so it IS our board. But it survived `nvs_flash_erase()` in the firmware!

Theory: **BlueZ is caching the old "EasyPlay" name** and showing it instead of the actual advertised name. The raw advertisement packets may actually say "BLE-Remote".

### Steps
1. `git pull`
2. `sudo systemctl restart bluetooth && sleep 3`
3. `sudo rm -rf /var/lib/bluetooth/*/cache/`
4. `sudo bluetoothctl remove 38:44:BE:45:AD:86` (ignore errors)
5. **Run the raw scan script**: `python3 pi/raw_scan.py`
   - This compares BlueZ name vs actual advertisement name
   - If they differ, BlueZ is caching!
6. If "BLE-Remote" appears in advertisement — try connecting with `ble_receiver.py`
7. **Commit and push REPORT.md** — Mac-Claude is waiting!

### What to report
- The BlueZ name vs the advertisement name — are they different?
- RSSI level (new antenna should improve signal)
- Connection result if attempted

### Success criteria
- Advertisement name shows "BLE-Remote" (even if BlueZ still says "EasyPlay")
- Connected and received heartbeats
