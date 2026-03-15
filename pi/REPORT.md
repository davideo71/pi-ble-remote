# Pi Test Report: Step 1 BLE Connection Test

## Test 12 — 2026-03-16 00:50 UTC (scan response re-enabled + name/MAC fallback)

**Duration:** ~90 seconds (5 scan cycles)

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: ESP32 NOT ADVERTISING

5 scan cycles, 45-54 devices each, **MAC `38:44:BE:45:AD:86` did not appear at all**. The callback now matches by UUID, MAC, or name ("BLE-Remote"/"EasyPlay") — none triggered.

### Verified: Pi-side code is correct

The detection callback (lines 87-102 of `ble_receiver.py`) matches by:
- Service UUID in `advertisement_data.service_uuids`
- MAC address `38:44:BE:45:AD:86`
- Name `BLE-Remote` or `EasyPlay`

None of these matched because the device simply isn't present in any scan.

### The pattern is clear: ESP32 advertising is unreliable

| Test | Time | Visible? | Notes |
|------|------|----------|-------|
| 1 | 23:00 | No | First test, no ESP32 |
| 2 | 23:12 | **Yes** | By MAC, cached name |
| 3 | 23:18 | **Yes** | By UUID (3 hits) |
| 5 | 23:50 | **Yes** | By UUID (1 hit) |
| 6-8 | 00:00-00:18 | No | Broken adv config |
| 9 | 00:29 | **Yes** | After minimal config revert (3/5 scans) |
| 10 | 00:34 | **Yes** (unfiltered only) | Filter was the issue |
| 11 | 00:40 | No | Intermittent |
| 12 | 00:50 | No | After reflash with scan response |

The ESP32 works for ~20-30 minutes after a fresh flash, then stops advertising. This suggests:
1. **NimBLE stops advertising after some internal timeout or error**
2. **Memory leak or stack overflow** causing crash
3. **Watchdog reset** that doesn't restart advertising properly

### Suggestion

- **Check ESP32 serial monitor** — is the heap shrinking? Are there any error messages? Is it still printing heartbeat status?
- **Add a periodic re-start of advertising** in the ESP32 loop as a safety measure
- **Add watchdog reset logging** to detect silent crashes
- **Consider a simpler test**: use `NimBLEAdvertising::start()` in the loop every 30 seconds to force re-advertising

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 12 (00:50) | Not found — ESP32 stopped advertising |
| 11 (00:40) | Not found — intermittent |
| 10 (00:34) | Found unfiltered, filter was unreliable |
| 9 (00:29) | Found 3/5 scans, connection timeout |
| 6-8 | Not found — broken adv config |
| 3-5 | Found — connection issues (BR/EDR, cache, timeout) |
| 1-2 | Initial tests, bleak bug fixed |
