# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 41: Clean BLE init (triggered 2026-03-17 22:45)

### Context
Great detective work on Test 40b! You proved:
1. The C3 IS our board (EasyPlay disappears when unplugged)
2. `nvs_flash_erase()` killed NimBLE (it needs NVS to function)
3. The old "EasyPlay" was from the OTA partition's old firmware

We've now removed `nvs_flash_erase()` and reflashed. Since the OTA data, NVS, and app1 were already erased externally, NimBLE should init fresh with "BLE-Remote" as the device name.

BLE MAC: `38:44:BE:45:AD:86` (base MAC +2).

### Steps
1. `git pull`
2. `sudo systemctl restart bluetooth && sleep 3`
3. `sudo rm -rf /var/lib/bluetooth/*/cache/`
4. `sudo bluetoothctl remove 38:44:BE:45:AD:86` (ignore errors)
5. Run `python3 pi/raw_scan.py` first — check if name is now "BLE-Remote"
6. If device found — connect with `ble_receiver.py` for 2 minutes
7. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device advertises as "BLE-Remote" (not "EasyPlay")
- Connected and received 30+ heartbeats
