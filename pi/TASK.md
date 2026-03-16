# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. **Before running the test**, restart the bluetooth service to clear all state:
   ```bash
   sudo systemctl restart bluetooth
   sleep 3
   ```
3. Run `python3 pi/ble_receiver.py` for about **2 minutes**
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 28)
No code changes. **Full bluetooth service restart before test.** Tests 24-26 all failed after Tests 21-23 worked perfectly. Suspicion is that accumulated BlueZ state from repeated failures is the problem. A full service restart should clear everything.

ESP32 is confirmed advertising with simulated button presses (cycling L, R, U, D, O every 500ms).

## Expected
- Clean BlueZ state after service restart
- Connection on first attempt (like Tests 21-23)
- Simulated button events arriving correctly

## Key question
Does restarting the bluetooth service fix the connection reliability?
