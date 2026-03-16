# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 20)
**Pre-emptive adapter reset on reconnect.** Test 19 showed the direct-MAC reconnect works but BlueZ "InProgress" error blocks it until an adapter reset. Now the adapter reset happens immediately at the start of `connect_and_listen_by_address()` so it's built into the reconnect path instead of being an extra retry.

Reconnect flow is now: adapter reset (3.2s) → direct connect by MAC (no scan) → subscribe.

## Test procedure
1. Let it connect normally (cold start)
2. Once receiving heartbeats, wait for a natural disconnect OR I may reset the ESP32 via serial
3. Measure reconnection time

## Expected
- Cold start: ~9-18 seconds (same as before)
- **Reconnection: ~5-8 seconds** (3.2s reset + connect, no scan)
- No "InProgress" errors on reconnect

## Key question
How fast is reconnection now? Does the pre-emptive reset avoid the InProgress error?
