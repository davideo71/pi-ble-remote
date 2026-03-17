# Pi Test Report: Step 2 — Button Handling

## Test 32b — 2026-03-17 09:50 UTC (button reading with 10ms loop delay — retry after power cycle)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — 10ms loop delay does NOT fix the connection drops

#### Discovery: Working
- ESP32 found by UUID on **3 out of 7 scans** (RSSI -85 dBm, consistent)
- Early exit scan working correctly

#### Connections: ALL timed out
- **Attempt 1**: Found in 4.4s → connect → DISCONNECTED after 19s → timeout
- **Attempt 2**: "No powered adapters" (adapter timing)
- **Attempt 3**: Found in 0.3s → connect → DISCONNECTED after 19.5s → timeout
- **Attempt 4-5**: "No powered adapters"
- **Attempt 6**: Not found in scan (39 devices)
- **Attempt 7**: Found in 1.7s → connect → timeout (15s, no DISCONNECTED)
- **Attempt 8**: (test timed out)

**Zero successful connections.** Every connection attempt either timed out or got DISCONNECTED during service discovery.

### Conclusion: Loop delay is NOT the root cause

| Loop delay | Button read | Connection result |
|-----------|------------|-------------------|
| 10ms | No (Test 30-31) | **PASS** — stable 73-78s |
| 5ms | Yes (Test 32) | **PARTIAL** — connected but dropped at 48s |
| 10ms | Yes (Test 32b) | **FAIL** — can't connect at all |

Test 32b is **worse** than Test 32 (5ms), not better. This rules out loop delay as the primary cause. The `digitalRead()` polling + debounce code itself is the problem, regardless of timing.

### Analysis

The difference between Test 31 (PASS, GPIO init only) and Test 32b (FAIL, GPIO init + digitalRead):
- Test 31: `pinMode()` in setup only, loop does nothing with GPIO
- Test 32b: `digitalRead()` on 5 pins every 10ms + debounce state tracking + serial output

Possible causes:
1. **`digitalRead()` may be disabling interrupts** briefly on ESP32-C3, disrupting NimBLE's timing-critical radio operations
2. **Serial output in the button handler** (`Serial.printf` on button change) may block for too long
3. **The debounce state struct + comparison logic** may be creating timing jitter

### Suggestion
1. **Try removing Serial.printf from button events** — serial output can block, especially at high baud rates
2. **Try reading only 1 GPIO pin** instead of 5 — isolate if it's the quantity of reads
3. **Switch to interrupt-driven buttons** — `attachInterrupt()` on each pin, set a flag, process in loop. This avoids continuous polling entirely.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 32b (09:50) | **FAIL** — 10ms delay doesn't help, all connections timeout |
| 32 (01:22) | PARTIAL — 5ms, connected but irregular, dropped at 48s |
| 31 (01:14) | PASS — GPIO init only, 48 heartbeats, 78s stable |
| 30 (01:06) | PASS — heartbeat only, 37 heartbeats, 73s stable |
| 29-24 | FAILED — full button firmware |
| 23-21 | Success — heartbeat-only firmware |
