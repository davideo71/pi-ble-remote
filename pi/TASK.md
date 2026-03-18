# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 45b: Fresh S3 with always-on advertising (triggered 2026-03-18)

### What changed from Test 45
- **Fresh ESP32-S3 SuperMini** flashed with Arduino/NimBLE firmware (not MicroPython EasyPlay)
- **Always-on advertising** — NO sleep mode, device never stops advertising
- **New MAC address: `A0:F2:62:EC:51:CA`** (base `A0:F2:62:EC:51:C8` + 2 for BLE)
- Device name: "BLE-Remote"
- Service UUID: `4e520001-7354-4288-9a71-81a9bf56c4a8`
- Char UUID: `4e520002-7354-4288-9a71-81a9bf56c4a8`
- Sends heartbeat (uint32 counter) every 2 seconds when connected
- Advertising interval: 20-40ms (fast for testing)
- TX power: +9 dBm (max)
- Restarts advertising automatically after disconnect

### ble_receiver.py v2 update needed
The receiver already supports this device. Just update the MAC:
- Change `KNOWN_MAC` or add `A0:F2:62:EC:51:CA` to `KNOWN_MACS` set
- Or just let it find by service UUID / name (should work without MAC)

### Steps
1. `git pull` (check `git log --oneline -3` to confirm new commit)
2. Run `python3 pi/ble_receiver.py` for 2+ minutes
3. Device should be found immediately (always advertising, no sleep)
4. Report: RSSI, connection success, heartbeat count
5. **Commit and push REPORT.md**

### Success criteria
- Device "BLE-Remote" found in scan with RSSI better than -80 dBm
- Connected and GATT service discovery completes
- Received 30+ heartbeats (= 1 minute connected)
