# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 40: NVS erase in firmware (triggered 2026-03-17 22:20)

### Context
Tests 39 and 39b STILL showed "EasyPlay" despite erasing OTA data, NVS, and app1 from the Mac side. This time we've added **`nvs_flash_erase()` directly in the firmware setup()** — the ESP32 now erases ALL NVS data on every boot before initializing BLE. We also explicitly set the advertising name to "BLE-Remote" in the advertising data.

If "EasyPlay" STILL appears at MAC `38:44:BE:45:AD:86`, then it's NOT coming from this board — it might be a neighbor's device or BlueZ cache that survived the restart.

BLE MAC: `38:44:BE:45:AD:86` (base MAC +2).

### Steps
1. `git pull`
2. `sudo systemctl restart bluetooth && sleep 3`
3. Clear BlueZ cache: `sudo rm -rf /var/lib/bluetooth/*/cache/`
4. Also try removing the device from BlueZ: `sudo bluetoothctl remove 38:44:BE:45:AD:86` (ignore errors)
5. Broad BLE scan — report ALL device names found, especially anything at MAC `38:44:BE:45:AD:86`
6. If "BLE-Remote" found — connect with `ble_receiver.py` for 2 minutes
7. If "EasyPlay" STILL appears at that MAC — note it but try connecting anyway
8. If "EasyPlay" appears at a DIFFERENT MAC — it's a neighbor's device, ignore it
9. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device name is now "BLE-Remote" (not "EasyPlay")
- Connected and received 30+ heartbeats
