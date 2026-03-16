# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 16)
**Fix for Test 15 regression:** Reverted adapter reset timing back to 3s (1s off + 2s on). The 1.5s timing caused "No powered Bluetooth adapters found" and all connections to timeout.

Everything else from Test 15 is kept:
- Early exit scan (stop on first match) — worked great in Test 15
- Fast reconnect path (3s address scan without adapter reset)
- Shorter reconnect delay (1s)
- InProgress immediate retry

## Expected
- Discovery should be fast (early exit scan, <2 seconds)
- Connections should work again (like Test 14)
- Connection should be stable

## Key questions
1. Does the connection succeed now? (Test 15 regression fixed?)
2. How long from script start to first heartbeat?
