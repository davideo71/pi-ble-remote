# Pi Test Report: Step 2 — Button Handling

## Test 25 — 2026-03-17 00:12 UTC (reverted to full adapter reset + button test)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — 6 attempts, all timed out. ESP32 dropping connections.

#### Attempt log
1. Full reset (4.2s) → connect → **DISCONNECTED after 5.4s** → timeout at 15s
2. Full reset (4.2s) → connect → timeout at 17s (no disconnect callback)
3. Full reset (4.1s) → connect → **DISCONNECTED after 13.8s** → timeout
4. Recovery (cache remove) → full reset → connect → **DISCONNECTED after 4.4s** → timeout
5. Full reset (4.2s) → connect → **DISCONNECTED after 6.8s** → timeout
6. Full reset (4.2s) → connect → (test timed out)

**Zero successful connections in 120 seconds.**

#### Key observation: the problem is NOT BlueZ
- Full adapter reset eliminated all InProgress errors (good — this confirms Test 24's light reset was the issue there)
- But connections are now **consistently dropping during service discovery** — the DISCONNECTED callback fires 4-14 seconds into the connection attempt
- This is a **different failure mode** from Tests 15-17 (which were BlueZ state issues) and Test 24 (which was InProgress)

#### This is an ESP32-side issue
The DISCONNECTED events during connection setup strongly suggest the **ESP32 is terminating the connection**. Possible causes:
1. **Button GPIO configuration is interfering with BLE** — the new button firmware initializes GPIO 0-4 with pull-ups, and GPIO 0 is often a boot/strapping pin on ESP32-C3 that can affect boot mode
2. **The ESP32 NimBLE stack may be crashing** when the Pi connects and triggers GATT service discovery
3. **The button debounce ISR or task may be consuming too much CPU**, starving the BLE stack

### Suggestion
1. **Check ESP32 serial output** — is there a crash/reboot when the Pi tries to connect?
2. **Check if GPIO 0 is safe to use** on the ESP32-C3 Super Mini — it may be a strapping pin
3. **Try disabling button handling temporarily** to confirm it's the button firmware causing the issue
4. **Compare ESP32 heap/stack** before and after the button firmware change

### Button events
Not tested — couldn't establish a connection.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 25 (00:12) | **FAILED** — 6 attempts, ESP32 drops connection during service discovery |
| 24 (00:07) | FAILED — 8 attempts, InProgress errors (light reset unreliable) |
| 23 (00:02) | Connection stable, heartbeats OK, no buttons pressed |
| 22 (23:37) | Light reset, 12.6s to heartbeat, 107s stable |
| 21 (23:27) | Direct connect by MAC, 13s to heartbeat, 105s stable |
| 18 (23:04) | Best scan+connect, 9s to heartbeat |
| 14 (22:30) | First stable connection (3m41s) |
| 1-13 | Early tests — discovery and connection issues |
