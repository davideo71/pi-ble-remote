# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 39b: After OTA+NVS+app1 erase (triggered 2026-03-17 22:10)

### Context
We found the root cause! The old "EasyPlay" firmware was in the **app1 OTA partition**, and the bootloader was loading it instead of our new code in app0. We have now:
1. Erased the OTA data partition (forces boot from app0)
2. Erased the NVS partition (clears cached BLE name)
3. Erased the entire app1 partition (removes old EasyPlay firmware)
4. Reflashed our firmware to app0

The LED is slow-blinking (confirmed our code runs). Device should now advertise as **"BLE-Remote"**.

BLE MAC: `38:44:BE:45:AD:86` (base MAC +2).

### Steps
1. `git pull`
2. `sudo systemctl restart bluetooth && sleep 3`
3. Clear BlueZ cache: `sudo rm -rf /var/lib/bluetooth/*/cache/`
4. Broad BLE scan — report ALL device names found, especially anything at MAC `38:44:BE:45:AD:86`
5. If "BLE-Remote" found — connect with `ble_receiver.py` for 2 minutes
6. If "EasyPlay" STILL appears — just try connecting anyway and report what happens
7. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device name is now "BLE-Remote" (not "EasyPlay")
- Connected and received 30+ heartbeats
