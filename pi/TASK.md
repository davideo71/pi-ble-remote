# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 18)
**Removed `bluetoothctl remove` from normal scan flow.** Tests 16-17 showed that removing the device from BlueZ cache before scanning causes the first connection to fail consistently. Now:
- Normal flow: just adapter reset + scan (no cache removal)
- Recovery: `bluetoothctl remove` only triggers after 3+ consecutive failures
- This matches the pattern from Test 14 (which never removed the cache and connected first try)

## Expected
- First connection attempt should succeed (like Test 14)
- Discovery still fast via early exit scan
- Stable connection once established

## Key question
Does the FIRST connection attempt succeed now?
