# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2-3 minutes**
3. **Important**: After it connects and runs for ~30 seconds, **kill the ble_receiver.py process** (Ctrl+C), wait 5 seconds, then **start it again**. This simulates a reconnection scenario.
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 15)
Pi-side only — no ESP32 changes. Optimized for fast connection and reconnection:
1. **Early exit scan** — stops scanning the instant the ESP32 is found (was waiting full 10s)
2. **Fast reconnect path** — on reconnect, tries a quick 3s address-based scan first, skips adapter reset
3. **Faster adapter reset** — 1.5s total (was 3s)
4. **Shorter reconnect delay** — 1s (was 3s)
5. **InProgress error** — retries immediately instead of waiting

## Expected
- First connection should be faster than Test 14 (early exit scan)
- After kill + restart, reconnection should be noticeably faster
- Connection should still be stable once established

## Key questions to answer
1. How long from script start to first heartbeat received? (Time the initial connection)
2. How long from script restart to first heartbeat? (Time the reconnection)
3. Does the connection stay stable after reconnecting?
