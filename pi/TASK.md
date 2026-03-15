# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth: `sudo systemctl restart bluetooth && sleep 2`
3. Run `python3 pi/ble_receiver.py` for about 60-90 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **Always remove BlueZ cache before scanning** — previously `remove_bluez_device()` was only called after a successful connection (which never happened). Now it always removes the known MAC `38:44:BE:45:AD:86` before every service UUID scan.
2. **Root cause of connection timeout**: BlueZ still had the device cached as BR/EDR from old "EasyPlay" firmware. The name "EasyPlay" still appearing in Test 9 confirms the stale cache. By removing it before scanning, BlueZ will re-discover it as a fresh LE device.
3. **No ESP32 firmware changes** — same minimal advertising config that works.

## Expected
- Device should be visible (like Test 9)
- BlueZ cache removal should allow the connection to succeed this time
- If connected → heartbeat notifications should flow (every 2 seconds)
- ESP32 serial monitor is running — we can confirm if onConnect fires on the ESP32 side
