# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. **Confirm Pi was freshly rebooted** — check uptime (`uptime`)
3. Run `python3 pi/ble_receiver.py` for about **2 minutes**
4. Update `pi/REPORT.md` with results (include uptime), commit and push

## What changed this iteration (Test 35 — fresh reboot + interrupt buttons)
Pi was just rebooted to clear all accumulated BlueZ/adapter state. Test 31b proved the baseline had degraded — same firmware that connected instantly at 01:14 took 8 attempts at 10:53 after 20+ test cycles.

Firmware is Test 34 (interrupt-driven buttons, no digitalRead polling). This is the first fair test of this firmware on a clean Pi.

## Expected
- If PASS on first or second attempt: confirms Pi state was the problem, interrupt-driven buttons work fine
- If FAIL: firmware genuinely has issues — need to revisit approach
