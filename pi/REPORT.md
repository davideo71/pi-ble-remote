# Pi Test Report: Step 2 — Button Handling

## Test 34c — 2026-03-17 10:43 UTC (BlueZ cache cleared + interrupt-driven buttons)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Pre-test: Cache cleared
```
sudo systemctl stop bluetooth
sudo rm -rf /var/lib/bluetooth/*/38:44:BE:45:AD:86
sudo rm -rf /var/lib/bluetooth/*/cache/38:44:BE:45:AD:86
sudo systemctl start bluetooth
```

### Result: FAILED — BlueZ cache is NOT the problem

#### RSSI: Good (-83 to -88 dBm)
Device found on all 6 scans, quick discovery (2-45 devices before match).

#### Connections: ALL dropped during service discovery
- **Attempt 1**: DISCONNECTED after 3.2s → timeout
- **Attempt 2**: "failed to discover services, device disconnected" after 8s
- **Attempt 3**: DISCONNECTED after 4.4s → timeout
- **Attempt 4**: DISCONNECTED after 9.8s → timeout
- **Attempt 5**: DISCONNECTED after 4.1s → timeout
- **Attempt 6**: (test timed out)

Same pattern as all tests since 32. Cache clearing made no difference.

### What we've now ruled out

| Hypothesis | Test | Result |
|-----------|------|--------|
| Loop delay too fast | 32b (10ms vs 5ms) | Both fail |
| Serial.printf blocking | 33 (no serial) | Still fails |
| Signal strength | 34b (closer) | Still fails at -88 dBm |
| BlueZ stale cache | **34c (cache cleared)** | **Still fails** |
| Interrupt conflicts | 34/34b (attachInterrupt) | Also fails |

### What we KNOW

1. **Test 31 PASSED** — `pinMode()` only, no `digitalRead()`, no `attachInterrupt()`
2. **Every test with any GPIO read activity fails** — polling or interrupt-driven
3. The ESP32 connects briefly (DISCONNECTED fires 3-20s in) then drops
4. RSSI and cache are not factors

### Remaining theories

1. **`attachInterrupt()` on unconnected pins with INPUT_PULLUP causes phantom interrupts** from RF noise. Each phantom interrupt triggers the ISR + a `digitalRead()`, creating the same interference as polling.
   - **Test**: flash firmware with `attachInterrupt()` but remove the ISR body (make it empty) or remove the `digitalRead()` in the flag-check code.

2. **The ESP32 NimBLE connection parameters changed** between Test 31 and Test 32 firmware. Check if the connection parameter settings (supervision timeout, connection interval) are still the same as the working Test 14/22 firmware.

3. **Something in the firmware binary is different** beyond just the button code. Compare the full .ino between Test 31 (PASS) and Test 34 (FAIL) line by line.

### Suggestion
Do a **minimal diff test**: take the exact Test 31 firmware and add ONLY `attachInterrupt(0, emptyISR, CHANGE)` on a single pin with an empty ISR (`void IRAM_ATTR emptyISR() {}`). No digitalRead, no debounce, no state tracking. If this fails, `attachInterrupt` itself is incompatible with NimBLE on this chip.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 34c (10:43) | **FAIL** — cache cleared, still can't connect |
| 34b (10:28) | FAIL — closer range, still fails |
| 34 (10:20) | FAIL — interrupt-driven, far range |
| 33 (10:06) | FAIL — no Serial |
| 32b (09:50) | FAIL — 10ms delay |
| 32 (01:22) | PARTIAL — 5ms, only test that partially worked |
| 31 (01:14) | **PASS** — GPIO init only |
| 30 (01:06) | **PASS** — heartbeat only |
