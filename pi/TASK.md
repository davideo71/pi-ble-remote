# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 21)
Two small fixes based on Test 20 feedback:
1. **Adapter reset on-wait increased from 2s to 3s** (1s off + 3s on = 4s total) — eliminates intermittent "No powered adapters" errors
2. **`connected_address` pre-seeded with KNOWN_MAC** — so the direct-connect path (skip scanning) is used from the very first run, not just after a successful connection

This means the startup flow is now: adapter reset (4s) → direct connect by MAC (no scan) → subscribe.

## Expected
- **Cold start: ~6-8 seconds** (4s reset + connect, no scan at all)
- First-attempt success (no "No powered adapters" errors)
- Stable connection once established

## Key question
Does pre-seeding the MAC address allow direct-connect on first startup (skipping scan entirely)? And is the 3s on-wait enough to eliminate adapter errors?
