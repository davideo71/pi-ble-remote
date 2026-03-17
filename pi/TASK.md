# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 34b — closer range)
ESP32 has been moved closer to the Pi. Same firmware as Test 34 (interrupt-driven buttons, no polling). No code changes.

Test 34 failed but RSSI was -90 to -91 dBm — edge of BLE range. All failures (Tests 32-34) happened after the ESP32 was moved further away. All passes (Tests 30-31) were at -81 to -88 dBm.

**This test determines whether the failures were signal-related or firmware-related.**

## Incremental progress
| Test | Added | RSSI | Result |
|------|-------|------|--------|
| 30 | Heartbeat only | -83 to -88 | PASS (73s+) |
| 31 | + GPIO init | -84 | PASS (78s+) |
| 32 | + digitalRead polling + serial | -81 | PARTIAL (48s) |
| 32b | + digitalRead polling + serial | -85 | FAIL |
| 33 | + digitalRead polling, no serial | -81 to -91 | FAIL |
| 34 | Interrupt-driven (no polling) | -90 to -91 | FAIL |
| **34b** | **Same as 34, closer to Pi** | **?** | **?** |

## Expected
- If PASS with stronger RSSI: distance was the problem all along, and we may need to re-evaluate Tests 32-33
- If FAIL with stronger RSSI: interrupt-driven approach also has issues
