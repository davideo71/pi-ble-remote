# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **3-4 minutes** (we need to see if connections last longer than 25-27 seconds now)
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 14)
1. **ESP32: relaxed connection parameters** — on connect, the ESP32 now requests: interval 30-60ms, latency 0, supervision timeout **6 seconds** (was likely default ~2s). This should fix the ~25-second disconnects.
2. **ESP32: heartbeat notify() return value logged** — to diagnose the "Connection 2 had no heartbeats" issue from Test 13.
3. **Pi code: unchanged** — adapter reset + active scanning already working well.
4. **ESP32: flash erased and reflashed** — clean state.

## Expected
- Discovery should still be reliable (as in Test 13: 16 hits in one scan)
- Connections should last **much longer than 25 seconds** now
- Heartbeat notifications every ~2 seconds
- If a connection still drops, check the disconnect reason code in the ESP32 serial logs

## Key questions to answer
1. Do connections survive past the 25-27 second mark?
2. Do heartbeats arrive consistently on every connection (not just 2 of 3)?
3. Does auto-reconnect still work reliably?
