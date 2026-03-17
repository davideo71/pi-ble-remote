# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 31b — baseline recheck)
Flashed the **exact same Test 31 firmware** that passed on the first run (GPIO init only, no digitalRead, no attachInterrupt). This is a sanity check to confirm the baseline still works after all the firmware iterations.

If this FAILS, then the problem is NOT firmware-related at all — something changed on the Pi side (BlueZ state, adapter issue, etc.).

## Expected
- If PASS: baseline still works, button-related code is genuinely the problem
- If FAIL: something on the Pi or environment has changed since Test 31 originally passed
