# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. **Before running the test, do a full bluetooth recovery:**
   ```bash
   sudo systemctl restart bluetooth
   sleep 3
   ```
3. Run `python3 pi/ble_receiver.py` for about **2 minutes**
4. Update `pi/REPORT.md` with results (include RSSI and uptime), commit and push

## What changed this iteration (Test 36 — fresh BT service + improved recovery)
ESP32 has been moved closer. `ble_receiver.py` now has improved recovery: after 3 consecutive failures it restarts the bluetooth service (not just cache clear), and resets `connected_address` to force a fresh scan.

Previous test (35b/35c) failed because ESP32 was out of range. It's been moved closer. The Pi may also have degraded BlueZ state from the failed scans — hence the bluetooth restart before testing.

## Important
- If Pi was rebooted, great — that gives us clean state
- If not rebooted, the bluetooth restart above should help
- Report RSSI so we can track signal strength
