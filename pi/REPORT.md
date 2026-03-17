# Pi Test Report: Step 2 — Button Handling

## Test 31 — 2026-03-17 01:14 UTC (GPIO init only — no button reading)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: SUCCESS — GPIO init does NOT break BLE

#### Connection
- Found by UUID in **0.7 seconds** (device #4, early exit)
- **Connected on FIRST attempt** in 13.3 seconds
- Script start → first heartbeat: **~25 seconds** (3s service restart + 4s adapter reset + 0.7s scan + 13.3s connect + 3s subscribe)

#### Stability
- **48 consecutive heartbeats** (#8-#55), every ~2 seconds, zero gaps
- Connection held for **78+ seconds** — still alive at test timeout
- No disconnects, no errors

### Isolation result: GPIO init is NOT the problem

`pinMode(0-4, INPUT_PULLUP)` in setup() does not interfere with BLE. The issue is in one of:
- Button reading in loop() (`digitalRead`)
- Button event notification via BLE
- Simulated button press code
- The combination of button polling + BLE notification timing

### Incremental test progress

| Test | Firmware | GPIO init | Button read | BLE notify | Simulated | Result |
|------|----------|-----------|-------------|------------|-----------|--------|
| 30 | Heartbeat only | No | No | No | No | **PASS** |
| **31** | **+ GPIO init** | **Yes** | **No** | **No** | **No** | **PASS** |
| Next | + Button read | Yes | Yes | No | No | ? |

### Next step
Add `digitalRead()` button polling in loop() with serial output only (no BLE notifications). This will test whether the polling itself interferes with BLE.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 31 (01:14) | **PASS** — GPIO init OK, 48 heartbeats, 78s stable |
| 30 (01:06) | PASS — heartbeat only, 37 heartbeats, 73s stable |
| 29-24 | FAILED — button firmware breaks all connections |
| 23-21 | Success — heartbeat-only firmware |
| 18 | Best result (9s to heartbeat) |
