# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 26)
**ESP32 now sends simulated button presses** — cycles through all 5 buttons (L, R, U, D, O) automatically every 500ms when connected. Press+release pairs. No manual button pressing needed.

This tests the full button pipeline end-to-end without human timing issues.

## Expected
- Connection on first attempt
- Automatic button events: L/l, R/r, U/u, D/d, O/o cycling continuously
- Heartbeats still flowing between button events
- Log shows `BUTTON: 'L' → LEFT press` etc.

## Key question
Do all 5 button events arrive and get decoded correctly? Any dropped events?
