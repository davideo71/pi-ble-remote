# Pi Test Report: Step 2 — Button Handling

## Test 34 — 2026-03-17 10:20 UTC (interrupt-driven buttons, no polling)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — But may be a SIGNAL STRENGTH issue, not firmware

#### Discovery: Working but device is far away
- ESP32 found by UUID on **4 out of 5 scans**
- RSSI: **-90 to -91 dBm** — this is MUCH weaker than all previous successful tests
- Device appears late in scans (after 6-31 other devices)

#### Connections: ALL timed out
- **Attempt 1**: Found → timeout (15s, no DISCONNECTED)
- **Attempt 2**: Found → DISCONNECTED after 8.8s → timeout
- **Attempt 3**: Found → DISCONNECTED after 19.7s → timeout
- **Attempt 4**: "No powered adapters"
- **Attempt 5**: Found → DISCONNECTED after 19.5s → timeout

### CRITICAL OBSERVATION: RSSI has dropped significantly

| Tests (working) | RSSI | Connection |
|-----------------|------|------------|
| 13-14 | -81 to -91 | Connected |
| 18 | -91 | Connected |
| 21-22 | -84 to -87 | Connected |
| 30-31 | -81 to -88 | **Connected** |

| Tests (failing) | RSSI | Connection |
|-----------------|------|------------|
| 32b | -85 | Failed |
| 33 | -81 to -91 | Failed |
| **34** | **-90 to -91** | **Failed** |

The user confirmed moving the ESP32 further from the Pi between tests. At -90 to -91 dBm, we're at the **edge of BLE connection range**. BLE connections need a stronger signal than advertising — you can receive advertisements at -90 dBm but may not sustain a connection.

### This test may NOT be valid for firmware comparison

With no buttons wired, no interrupts should fire, so this firmware is functionally equivalent to Test 31 (which PASSED at -84 dBm). The failure is likely due to:
1. **Weak signal (-90 dBm)** making connections unreliable
2. **Not the interrupt-driven button code** (no interrupts are firing)

### Suggestion
1. **Move the ESP32 closer to the Pi** — back to the ~1 meter distance used in Tests 18-31
2. **Re-run Test 34** at the closer distance
3. If it PASSES at close range: the failures since Test 32 may have ALL been signal-related, not firmware-related!

This is worth investigating because Test 32 (the first "PARTIAL" result with button polling) also coincided with the ESP32 being moved. The timing irregularities may have been signal dropouts, not CPU starvation.

---

## Previous Tests Summary

| Test | Result | RSSI |
|------|--------|------|
| 34 (10:20) | **FAIL** — all timeouts, RSSI -90 to -91 (too weak?) |
| 33 (10:06) | FAIL — RSSI -81 to -91 |
| 32b (09:50) | FAIL — RSSI -85 |
| 32 (01:22) | PARTIAL — RSSI -81 |
| 31 (01:14) | PASS — RSSI -84 |
| 30 (01:06) | PASS — RSSI -83 to -88 |
