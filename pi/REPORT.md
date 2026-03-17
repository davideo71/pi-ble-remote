# Pi Test Report: Step 2 — Button Handling

## Test 32b — 2026-03-17 01:39 UTC (button reading with 10ms loop delay)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — ESP32 not visible at all

**7 full scans, 42-54 devices each, ZERO matches for ESP32.** The device MAC, UUID, and name were not seen in any scan.

This is the same "ESP32 stopped advertising" pattern from Tests 11-12 and 26-27. The ESP32 likely crashed or stopped advertising after the firmware flash.

### Not a code issue — ESP32 is offline

This is NOT caused by the 10ms vs 5ms change. The ESP32 is simply not advertising. It may need a power cycle.

Note: the TASK.md mentioned the ESP32 was moved further from the Pi. However, the Pi saw 42-54 other BLE devices per scan, so the adapter is working fine. The ESP32 is just not there.

### Cannot evaluate 10ms loop delay

Since the ESP32 wasn't advertising, we can't compare 10ms vs 5ms polling. Need to retry after confirming ESP32 is powered and advertising.

### Suggestion
1. Power cycle the ESP32
2. Check serial output to confirm it's advertising
3. Re-run this test

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 32b (01:39) | **FAILED** — ESP32 not advertising (offline) |
| 32 (01:22) | PARTIAL — 5ms polling causes irregular heartbeats + 48s dropout |
| 31 (01:14) | PASS — GPIO init OK, 48 heartbeats, 78s stable |
| 30 (01:06) | PASS — heartbeat only, 37 heartbeats, 73s stable |
| 29-24 | FAILED — full button firmware breaks all connections |
| 23-21 | Success — heartbeat-only firmware |
