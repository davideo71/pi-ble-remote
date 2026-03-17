# Pi Test Report: Step 2 — Button Handling

## Test 34b — 2026-03-17 10:28 UTC (interrupt-driven buttons, closer range)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — Signal strength improved but connections still fail

#### RSSI improved to -88 dBm (was -90 to -91)
- ESP32 found by UUID/MAC on **5 out of 6 scans**
- RSSI: **steady -88 dBm** — in the range where Tests 30-31 worked

#### Connections: ALL timed out
- **Attempt 1**: Found (RSSI -88) → DISCONNECTED after 19.5s → timeout
- **Attempt 2**: "No powered adapters"
- **Attempt 3**: Found (RSSI -88) → DISCONNECTED after 3.9s → timeout
- **Attempt 4**: Found (RSSI -88) → DISCONNECTED after 8.2s → timeout
- **Attempt 5**: Found (RSSI -88) → DISCONNECTED after 15.5s → timeout
- **Attempt 6**: Found (RSSI -88) → (test timed out)

### Conclusion: This IS a firmware issue, NOT signal strength

RSSI -88 dBm is in the same range where Tests 30 (-83 to -88) and 31 (-84) passed. The closer distance improved RSSI from -90 to -88 but connections still fail. **The interrupt-driven button firmware has a problem.**

### But wait — why?

With no buttons wired, no interrupts should fire. The only difference from Test 31 (which PASSED):
- Test 31: `pinMode()` only in setup, nothing in loop
- Test 34b: `pinMode()` + `attachInterrupt()` in setup, interrupt flag check in loop

The `attachInterrupt()` itself may be the issue. Possible causes:

1. **Floating GPIO pins trigger spurious interrupts.** Without buttons wired, the GPIO pins with INPUT_PULLUP are floating or picking up noise. Even with pullups, nearby RF (from the Pi's Bluetooth!) could trigger phantom interrupts on unconnected pins, which then run the ISR and process a `digitalRead()`.

2. **`attachInterrupt()` on 5 pins may conflict with NimBLE's interrupt handlers.** NimBLE uses hardware interrupts for radio timing. Having 5 additional GPIO interrupt handlers may cause priority conflicts.

### Suggestion
1. **Test with only 1 interrupt pin** instead of 5 — see if reducing interrupt count helps
2. **Connect the buttons with real pullup resistors** — floating pins with just internal pullup may be bouncing from RF noise
3. **Try `attachInterrupt()` with `FALLING` instead of `CHANGE`** — halves the interrupt frequency
4. **Or: go back to polling but only read 1 pin at 50ms** to isolate whether quantity or method matters

---

## Updated Isolation Summary

| Test | What's in firmware | RSSI | Result |
|------|-------------------|------|--------|
| 30 | Heartbeat only | -83/-88 | **PASS** |
| 31 | + `pinMode()` | -84 | **PASS** |
| 32 | + `digitalRead()` polling (5ms) | -81 | PARTIAL (48s) |
| 32b | + `digitalRead()` polling (10ms) | -85 | FAIL |
| 33 | + `digitalRead()` no serial | -81/-91 | FAIL |
| 34 | + `attachInterrupt()` (far) | -90/-91 | FAIL |
| **34b** | **+ `attachInterrupt()` (close)** | **-88** | **FAIL** |

Both polling AND interrupts break BLE. The common factor: **any GPIO read activity** (whether polled or interrupt-driven) disrupts NimBLE connections.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 34b (10:28) | **FAIL** — closer range (-88 dBm), still can't connect |
| 34 (10:20) | FAIL — too far (-90 dBm) |
| 33 (10:06) | FAIL — no Serial, still fails |
| 32b (09:50) | FAIL — 10ms delay doesn't help |
| 32 (01:22) | PARTIAL — only test that partially worked |
| 31 (01:14) | PASS — GPIO init only |
| 30 (01:06) | PASS — heartbeat only |
