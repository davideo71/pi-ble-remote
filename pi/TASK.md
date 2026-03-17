# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 32)
**Test 31 PASSED — GPIO init alone does NOT break BLE (48 heartbeats, 78s stable).**

Test 32 adds **button reading with debounce** in loop() — `digitalRead()` on GPIO 0-4 every 5ms with 50ms debounce. Button events are logged to Serial only. **NO BLE notifications for buttons** — only heartbeats go over BLE.

This tests whether the digitalRead polling + debounce logic in the main loop interferes with BLE.

## Incremental progress so far
| Test | Added | Result |
|------|-------|--------|
| 30 | Heartbeat only | PASS |
| 31 | + GPIO init | PASS |
| **32** | **+ button reading (serial only)** | **?** |

## Expected
- Should work like Tests 30-31 (heartbeats flowing, stable connection)
- If it fails: digitalRead polling at 200Hz is the problem
- If it passes: the issue is in BLE button notifications or simulated presses
