# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 34)
**Tests 32-33 proved that `digitalRead()` polling disrupts NimBLE** on the single-core C3, regardless of loop delay or serial output. The continuous GPIO register access starves NimBLE's timing-critical radio operations.

Test 34: **Interrupt-driven buttons.** Instead of polling `digitalRead()` every loop iteration, we use `attachInterrupt(pin, handler, CHANGE)` on each GPIO. The ISR just sets a flag. The loop only does a single `digitalRead()` when a flag is set (i.e., when a button actually changed). No buttons are wired, so no interrupts should fire — the loop should be as clean as Test 31.

## Incremental progress
| Test | Added | Loop delay | Result |
|------|-------|-----------|--------|
| 30 | Heartbeat only | 10ms | PASS (73s+) |
| 31 | + GPIO init | 10ms | PASS (78s+) |
| 32 | + digitalRead polling + serial | 5ms | PARTIAL (48s) |
| 32b | + digitalRead polling + serial | 10ms | FAIL |
| 33 | + digitalRead polling, no serial | 10ms | FAIL |
| **34** | **Interrupt-driven (no polling)** | **10ms** | **?** |

## Expected
- Should PASS — with no buttons wired, no interrupts fire, so the loop is effectively the same as Test 31 (which passed). This confirms the architecture works before we add BLE notifications.
