# Pi Test Report: Step 2 — Button Handling

## Test 33 — 2026-03-17 10:06 UTC (button reading WITHOUT Serial.printf on events)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — Serial.printf is NOT the culprit

#### Discovery: Working (intermittent)
- ESP32 found on 4 out of 7 scans (3 by MAC, 1 by UUID)
- RSSI: -81 to -91 dBm
- ESP32 advertising is intermittent again — missed 2 consecutive scans (49 and 47 devices each)

#### Connections: ALL failed
- **Attempt 1**: Found by UUID → DISCONNECTED after 7s → timeout
- **Attempt 2**: Found by MAC → DISCONNECTED after 14.2s → timeout
- **Attempt 3**: Found by MAC → DISCONNECTED after 7.2s → timeout
- **Attempt 4**: Not found (2 scans)
- **Attempt 5**: Found by MAC → (test timed out during connect)

**Zero successful connections.** Same pattern as Test 32b.

### Isolation complete: `digitalRead()` + debounce is the problem

| Test | digitalRead | Serial on events | Result |
|------|-----------|-----------------|--------|
| 31 | No | No | **PASS** |
| 32 | Yes | Yes (5ms) | PARTIAL |
| 32b | Yes | Yes (10ms) | **FAIL** |
| **33** | **Yes** | **No** | **FAIL** |

Removing Serial.printf made no difference. The problem is the **`digitalRead()` polling + debounce logic itself**. Reading 5 GPIO pins every 10ms with state tracking is enough to disrupt NimBLE's connection handling on the single-core ESP32-C3.

### Why digitalRead breaks NimBLE on ESP32-C3

The ESP32-C3 has a single RISC-V core. NimBLE runs as a FreeRTOS task and needs the CPU at precise moments for:
- Connection parameter negotiation (within ms of connection)
- GATT service discovery responses
- Link-layer packet acknowledgments

`digitalRead()` on ESP32 uses the GPIO hardware registers directly but may briefly disable interrupts or hold a mutex. Doing this 5 times per loop iteration (5 pins) creates repeated micro-delays that accumulate and cause NimBLE to miss its timing windows.

### Suggestion: Switch to interrupt-driven buttons

Instead of polling `digitalRead()` every loop iteration:
1. Use `attachInterrupt(pin, handler, CHANGE)` on each GPIO pin
2. In the ISR, just set a flag or push to a queue
3. In `loop()`, only process the flag/queue (no GPIO access)
4. This means GPIO reads happen only on actual button changes, not 100x/second

This is the standard approach for BLE + buttons on ESP32-C3.

---

## Incremental Isolation Summary

| Test | What's in the firmware | Result | Conclusion |
|------|----------------------|--------|------------|
| 30 | Heartbeat only | PASS | Baseline works |
| 31 | + `pinMode()` | PASS | GPIO init is fine |
| 32 | + `digitalRead()` + debounce + Serial | PARTIAL | Something in the additions breaks BLE |
| 32b | + `digitalRead()` + debounce + Serial (10ms) | FAIL | Not a timing issue |
| 33 | + `digitalRead()` + debounce (no Serial) | FAIL | Not Serial.printf |
| **Next** | **Interrupt-driven buttons** | **?** | **Should avoid polling entirely** |

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 33 (10:06) | **FAIL** — removing Serial didn't help, digitalRead itself is the problem |
| 32b (09:50) | FAIL — 10ms delay doesn't help |
| 32 (01:22) | PARTIAL — 5ms, connected but dropped at 48s |
| 31 (01:14) | PASS — GPIO init only |
| 30 (01:06) | PASS — heartbeat only |
| 29-24 | FAILED — full button firmware |
