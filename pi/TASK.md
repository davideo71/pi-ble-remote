# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 27)
No code changes. **ESP32 has been freshly reset and is confirmed advertising.** The previous test failures (24-26) were likely due to ESP32 being reflashed/reset from the Mac side during the test window.

ESP32 firmware sends simulated button presses (cycling L, R, U, D, O every 500ms) plus heartbeats every 2s.

## Expected
- Connection on first attempt
- Simulated button events cycling through all 5 buttons
- Heartbeats between button events

## Key question
Do button events arrive and decode correctly? This is the same test as 26 but with a confirmed-good ESP32.
