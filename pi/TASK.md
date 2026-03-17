# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 30)
**ESP32: Reverted to heartbeat-only firmware (no buttons, no GPIO).** Tests 24-29 ALL failed with the button firmware — connections drop at the BLE level before GATT discovery completes. This test strips the firmware back to match the working Tests 21-23 to confirm the ESP32 hardware is still OK.

Changes from the button firmware:
- Removed ALL button code: struct, GPIO init, button scanning, simulated presses
- Removed grace period (not needed)
- Heartbeat sends immediately on connect (like Tests 21-23)
- Loop delay back to 10ms (was 5ms for button polling)

## Expected
- Scan-based discovery finds ESP32
- Connection succeeds on first or second attempt
- Heartbeats arrive every 2 seconds
- Stable connection for the full 2 minutes
- This should work exactly like Tests 21-23

## Key question
Does removing the button code restore stable connections? If YES → the button code is the problem and we'll add it back incrementally. If NO → something else changed (Pi-side or environment).
