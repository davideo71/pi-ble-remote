# Pi Test Report: Step 2 — Button Handling

## Test 32 — 2026-03-17 01:22 UTC (button reading with debounce, serial only)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: PARTIAL — Connected and received heartbeats, but connection unstable

#### Connection
- Found by UUID in **4.5 seconds** (early exit, device #15)
- **Connected on FIRST attempt** in 11.2 seconds
- Script start → first heartbeat: **~27 seconds**

#### Heartbeats: Received but IRREGULAR
- **18 heartbeats** (#6-#23) over 43 seconds
- Timing was **uneven** — some gaps and bursts:
  - Normal: #6-#15 every ~2 seconds (20 seconds, 10 heartbeats) ✓
  - **Gap**: 5.2s between #15 and #16 (expected ~2s)
  - **Burst**: #17, #18, #19 arrived within 0.4s of each other
  - **Burst**: #20, #21 arrived within 0.4s
  - Then back to normal: #22, #23 every ~2 seconds
- This suggests the **button polling (200Hz digitalRead) is occasionally blocking the BLE stack**, causing heartbeat notifications to queue up and burst

#### Connection dropped after 48 seconds
- DISCONNECTED at 01:23:33 — connection lasted **48 seconds**
- This is shorter than Tests 30-31 (73-78s, still alive at timeout)
- After disconnect, direct-connect reconnection failed ("Device not found" × 3)

### Comparison with previous tests

| Test | Added code | Heartbeats | Timing | Duration | Result |
|------|-----------|------------|--------|----------|--------|
| 30 | Heartbeat only | 37, steady | Even ~2s | 73s+ (alive) | PASS |
| 31 | + GPIO init | 48, steady | Even ~2s | 78s+ (alive) | PASS |
| **32** | **+ button read (serial)** | **18, irregular** | **Gaps + bursts** | **48s (dropped)** | **PARTIAL** |

### Analysis

The button polling at 200Hz (5ms loop delay) is **interfering with BLE but not fatally**:
- Connection succeeds (unlike Tests 24-29 which couldn't connect at all)
- Heartbeats arrive but with timing jitter
- Connection drops earlier than heartbeat-only firmware

The 5ms loop delay means `digitalRead()` runs 200 times per second across 5 pins. This is likely starving NimBLE's internal task scheduler. The original button firmware (Tests 24-29) was worse because it also had BLE notifications + simulated presses, but even without those, the polling alone causes instability.

### Suggestion
1. **Reduce polling rate** — try 50ms (20Hz) or 100ms (10Hz) loop delay instead of 5ms
2. **Use interrupts instead of polling** — attach ISR to each GPIO pin for press/release, only process events when they occur
3. **Add `delay(1)` or `yield()` calls** to give NimBLE processing time between digitalRead calls

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 32 (01:22) | **PARTIAL** — connected, heartbeats irregular, dropped after 48s |
| 31 (01:14) | PASS — GPIO init OK, 48 heartbeats, 78s stable |
| 30 (01:06) | PASS — heartbeat only, 37 heartbeats, 73s stable |
| 29-24 | FAILED — full button firmware breaks all connections |
| 23-21 | Success — heartbeat-only firmware |
