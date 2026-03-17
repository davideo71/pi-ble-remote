# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 31)
**Test 30 PASSED — heartbeat-only firmware works perfectly (37 heartbeats, 73s stable).**

Now adding button code back incrementally. **Test 31 adds ONLY GPIO init** — `pinMode(0-4, INPUT_PULLUP)` in setup(). No button reading in loop(), no notifications, no simulated presses. Just the pinMode calls.

This tests whether initializing GPIO pins as INPUT_PULLUP interferes with BLE.

## Expected
- Should work exactly like Test 30 (heartbeats flowing, stable connection)
- If it fails: GPIO init itself is the problem (maybe GPIO 2 strapping pin on C3?)
- If it passes: the issue is in the button reading/notification code, not GPIO init
