# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 29)
**ESP32: 5-second grace period after connection before sending notifications.** Test 28b showed the ESP32 drops connections during GATT service discovery. The simulated button notifications were firing immediately on connect, before the Pi finished subscribing. Now the ESP32 waits 5 seconds after connection before sending any notifications (buttons or heartbeat).

## Expected
- Scan-based discovery finds ESP32
- Connection holds through GATT discovery (no more drops)
- After 5s grace period: simulated button events + heartbeats arrive
- Stable connection

## Key question
Does the 5s grace period fix the connection drops during GATT discovery?
