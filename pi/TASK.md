# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results (include RSSI), commit and push

## What changed this iteration (Test 35b — range test)
**No firmware or Pi changes.** Same interrupt-driven button firmware, same fresh Pi session. ESP32 has been moved to a different (likely further) location.

Test 35 passed perfectly at -89 dBm. This test checks if the connection holds at the new distance.

## Expected
- Note the RSSI compared to Test 35 (-89 dBm)
- If PASS: we have good range and can proceed to adding BLE button notifications
- If FAIL: we know the range limit and need to keep the ESP32 closer
