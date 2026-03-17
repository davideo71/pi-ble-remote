# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 33)
**Test 32b FAILED** — all connections timed out or disconnected during service discovery, even at 10ms delay. Loop delay is NOT the root cause.

Test 33: **Same as Test 32b but with Serial.printf REMOVED from button event handling.** Everything else is identical — same digitalRead polling on 5 pins, same debounce logic, same 10ms loop delay. We're testing whether Serial.printf in the button handler was blocking long enough to disrupt NimBLE.

ESP32 will need to be re-flashed and power-cycled before this test.

## Incremental progress
| Test | Added | Loop delay | Result |
|------|-------|-----------|--------|
| 30 | Heartbeat only | 10ms | PASS (73s+) |
| 31 | + GPIO init | 10ms | PASS (78s+) |
| 32 | + button read + serial output | **5ms** | PARTIAL (48s, irregular heartbeats) |
| 32b | + button read + serial output | **10ms** | FAIL (all connections timeout) |
| **33** | + button read, **NO serial on events** | **10ms** | **?** |

## Expected
- If PASS: Serial.printf was the culprit — it blocks long enough to disrupt NimBLE radio timing
- If FAIL: digitalRead itself or the debounce logic is the problem → try interrupt-driven buttons next
