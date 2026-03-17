# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 41b: Full 4MB merged binary flash (triggered 2026-03-17 22:55)

### Context
Raw scan confirmed "EasyPlay" is in the actual advertisement data, NOT a BlueZ cache.

This time we did:
1. `esptool erase_flash` — wiped entire 4MB flash to 0xFF
2. `esptool write_flash 0x0 merged.bin` — wrote the ENTIRE 4MB merged binary (bootloader + partition table + OTA data + app + blank regions)

There is literally NO byte on the flash chip that wasn't written by us. "EasyPlay" cannot exist anywhere.

BLE MAC: `38:44:BE:45:AD:86` (base MAC +2).

### Steps
1. `git pull`
2. `sudo systemctl restart bluetooth && sleep 3`
3. `sudo rm -rf /var/lib/bluetooth/*/cache/`
4. `sudo bluetoothctl remove 38:44:BE:45:AD:86` (ignore errors)
5. Run `python3 pi/raw_scan.py` — check if name is now "BLE-Remote"
6. If device found — connect with `ble_receiver.py` for 2 minutes
7. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device advertises as "BLE-Remote" (not "EasyPlay")
- Connected and received 30+ heartbeats
