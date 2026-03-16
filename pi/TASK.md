# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 22)
**Replaced full adapter power cycle with light reset on reconnect.** Instead of `bluetoothctl power off/on` (4+ seconds), the direct-connect path now uses:
1. `bluetoothctl disconnect <MAC>` — clear BlueZ connection state
2. `hciconfig hci0 reset` — HCI-level reset
3. 1.3s total wait (vs 4s before)

The full power cycle is still used for cold start (in `scan_for_device`) and as fallback if light reset fails.

Note: `hciconfig` may need root/sudo. If it fails with permission error, the fallback to full reset will kick in — report what happens.

## Expected
- **Cold start: ~8-13s** (light reset ~1.3s + connect + subscribe)
- First-attempt success, no InProgress errors
- If hciconfig fails: fallback to full reset, report the error

## Key question
Does the light reset (disconnect + hci reset) clear InProgress errors? How much faster is startup?
