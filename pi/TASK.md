# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 17)
Two fixes for the "multiple failed attempts" issue from Test 16:
1. **Connect timeout back to 15s** (was 10s) — BlueZ sometimes needs >10s to complete the connection handshake
2. **Only `bluetoothctl remove` on first scan** — not on retries. Removing every time was forcing BlueZ to re-discover from scratch, causing repeated failures.

## Expected
- First connection attempt should succeed (like Test 14)
- Fast discovery via early exit scan still works
- Stable connection once established

## Key question
Does the first connection attempt succeed? (Test 16 needed 4 attempts)
