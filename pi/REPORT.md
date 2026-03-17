# Pi Test Report: Step 2 — Button Handling

## Test 30 — 2026-03-17 01:06 UTC (reverted to heartbeat-only firmware)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: SUCCESS — Heartbeat-only firmware works perfectly. Button code confirmed as the problem.

#### Connection
- **Attempt 1**: Found by UUID in 2.5s → connect → timeout (15s). First attempt after service restart often fails.
- **Attempt 2**: Found by UUID in 3.4s → **CONNECTED** in 10s → SUCCESS

**Script start → first heartbeat: ~46 seconds** (3s service restart + 4s adapter reset + 2.5s scan + 15s timeout + retry + 10s connect)

#### Stability: 73 seconds, 37 heartbeats
- **37 consecutive heartbeats** (#4-#40), every ~2 seconds, zero gaps
- Connection held for **73+ seconds** — still alive at test timeout
- No disconnects, no errors after connecting

### CONFIRMED: Button firmware is the problem

| Firmware | Tests | Connections | Stability |
|----------|-------|-------------|-----------|
| Heartbeat only (Test 14) | 14 | First attempt | 3m41s stable |
| Heartbeat only (Test 18) | 18 | First attempt | 111s stable |
| Heartbeat only (Test 21-22) | 21-22 | First attempt | 105-107s stable |
| Heartbeat only (Test 23) | 23 | First attempt | 97s stable |
| **Button firmware** | **24-29** | **ZERO successful** | **All connections drop** |
| **Heartbeat only (Test 30)** | **30** | **2nd attempt** | **73s+ stable** |

Reverting to heartbeat-only firmware immediately restored working connections. The ESP32 hardware is fine — the button code is breaking BLE.

### Answer to key question

**Does removing the button code restore stable connections?** YES — definitively. The button firmware was the sole cause of Tests 24-29 failures.

### Next steps for button support

Add button code incrementally to isolate the breaking change:
1. **Test A**: Add GPIO init only (no button reading, no notifications) → test
2. **Test B**: Add button reading with serial output (no BLE notifications) → test
3. **Test C**: Add BLE button notifications → test

The likely culprit is one of:
- GPIO ISR handlers conflicting with NimBLE's Bluetooth ISR
- Stack overflow from button task + BLE task
- GPIO 0 being a strapping pin on ESP32-C3

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 30 (01:06) | **SUCCESS** — heartbeat-only firmware, 2nd attempt connect, 37 heartbeats stable |
| 29-24 | FAILED — button firmware breaks all connections |
| 23-21 | Success — heartbeat-only firmware |
| 18 | Best result (9s to heartbeat) |
| 14 | First stable connection (3m41s) |
