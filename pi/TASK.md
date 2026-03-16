# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 23 — Step 2: Buttons!)
**ESP32 now has button handling.** 5 buttons on GPIO 0-4, wired to GND with pull-ups. Sends single ASCII chars as BLE notifications:
- Press: uppercase (L, R, U, D, O)
- Release: lowercase (l, r, u, d, o)

**Pi receiver updated** to decode button events. The notification handler now recognizes 1-byte button chars and logs them with names (e.g. `BUTTON: 'L' → LEFT press`).

Heartbeat still runs every 2s (4-byte uint32) for connection monitoring.

## Expected
- Connection as before (~12s cold start)
- Button presses show as `BUTTON: 'L' → LEFT press` etc.
- Heartbeats still flow between button presses
- Buttons that arrive will be pressed manually during the test

## Key question
Do button notifications arrive correctly on the Pi? Are they decoded as single ASCII chars?
