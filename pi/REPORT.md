# Pi Test Report: Step 2 — Button Handling

## Test 26 — 2026-03-17 00:20 UTC (simulated button presses on ESP32)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — ESP32 not visible at all ("Device not found")

#### Attempt log
1. Full reset → connect → **"Device with address 38:44:BE:45:AD:86 was not found"** (15s)
2. Full reset → connect → timeout (19s)
3. Full reset → connect → **"Device not found"** (15s)
4. Recovery (cache remove) → full reset → **"Device not found"** (15s)
5. Full reset → **"Device not found"** (15s)
6. Full reset → connect → (test timed out)

**New error: "Device was not found"** — this is different from Tests 24-25:
- Test 24: InProgress errors (BlueZ state issue)
- Test 25: DISCONNECTED during service discovery (ESP32 dropping connection)
- **Test 26: ESP32 not discoverable at all**

### Analysis

The ESP32 appears to be **completely invisible** to the Pi's Bluetooth adapter. Possible causes:

1. **The simulated button firmware may crash before starting BLE** — if the button simulation code runs before `NimBLEDevice::init()` completes, it could crash the ESP32
2. **ESP32 stopped advertising** — the periodic re-advertising pattern from earlier tests. The ESP32 may need a power cycle.
3. **The new firmware may have a compilation/upload error** — worth checking the serial output

### Note on direct-connect without scan
The direct-connect path (`BleakClient(address)`) relies on BlueZ having the device in its cache or being able to discover it in the background. When the device isn't advertising at all, this path gives "Device not found" instead of a timeout. This is actually a better error than a 15s timeout — it fails faster.

### Suggestion
1. **Check ESP32 serial output** — is it booting and advertising?
2. **Power cycle the ESP32** — may need a fresh start after the firmware change
3. **If the ESP32 is advertising but still not found**: try falling back to scan-based discovery instead of direct MAC connect, as scanning may pick up the device even when direct connect can't

### Button events
Not tested — ESP32 not visible.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 26 (00:20) | **FAILED** — ESP32 not visible ("Device not found") |
| 25 (00:12) | FAILED — ESP32 drops connections during discovery |
| 24 (00:07) | FAILED — InProgress errors (light reset unreliable) |
| 23 (00:02) | Connection stable, heartbeats OK, no buttons pressed |
| 22 (23:37) | Light reset, 12.6s to heartbeat, 107s stable |
| 21 (23:27) | Direct connect by MAC, 13s to heartbeat, 105s stable |
| 18 (23:04) | Best scan+connect, 9s to heartbeat |
| 14 (22:30) | First stable connection (3m41s) |
| 1-13 | Early tests — discovery and connection issues |
