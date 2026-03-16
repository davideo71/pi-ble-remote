# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 24)
No code changes. **Buttons are now soldered and working on the ESP32.** Test 23 confirmed the connection and heartbeat handler work. This test should capture actual button press/release events.

Buttons will be pressed manually during the test.

## Expected
- Connection as before
- **Button events**: `BUTTON: 'L' → LEFT press` etc. mixed in with heartbeats
- All 5 buttons should appear (L, R, U, D, O)

## Key question
Do button events arrive and get decoded correctly on the Pi?
