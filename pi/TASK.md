# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 29)
The code now has TWO key fixes that may not have been in effect for Test 28 (git timing):

1. **`connected_address = None` on startup** — forces scan-based discovery on first connection (not direct-connect by MAC). This was the root cause of "Device not found" errors.
2. **`systemctl restart bluetooth` runs automatically at script startup** — built into the Python script now.

Also: ESP32 was removed from the protoboard (buttons disconnected) in case the wiring was causing hardware interference.

## Expected
- Script restarts bluetooth service automatically
- Scan finds the ESP32 (like Tests 13-18)
- Connection succeeds on first or second attempt
- Simulated button events arrive (L, R, U, D, O cycling)
- Heartbeats every 2s

## Key question
Does scan-based first connection work? Do simulated button events arrive?
