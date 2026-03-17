# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 32b)
**Test 32 was PARTIAL** — button polling at 5ms (200Hz) starved NimBLE on the single-core C3. Heartbeats had gaps/bursts and connection dropped after 48s.

Test 32b: **Same as Test 32 but with 10ms loop delay** (100Hz polling instead of 200Hz). This gives NimBLE more processing time between digitalRead calls.

Note: User also moved the ESP32 further from the Pi between tests. RSSI may be different.

## Incremental progress
| Test | Added | Loop delay | Result |
|------|-------|-----------|--------|
| 30 | Heartbeat only | 10ms | PASS (73s+) |
| 31 | + GPIO init | 10ms | PASS (78s+) |
| 32 | + button read (serial) | **5ms** | PARTIAL (48s, irregular heartbeats) |
| **32b** | + button read (serial) | **10ms** | **?** |

## Expected
- If 10ms fixes it: 5ms was too aggressive for single-core C3 + NimBLE
- If still unstable: digitalRead itself is the problem regardless of timing
