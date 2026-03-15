# Pi Test Report: Step 1 BLE Connection Test

## Test 10 — 2026-03-16 00:34 UTC (cache removal before every scan)

**Duration:** ~90 seconds filtered + 15s unfiltered

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: `service_uuids` FILTER IS UNRELIABLE — ESP32 visible unfiltered but invisible filtered

**CRITICAL FINDING:**

| Scan method | Duration | ESP32 found? |
|-------------|----------|-------------|
| bleak + `service_uuids` filter | 90s (5 cycles) | **NO** (0 devices each cycle) |
| bleak unfiltered (immediately after) | 15s | **YES** — `38:44:BE:45:AD:86 name=EasyPlay` |

**The `service_uuids` filter is filtering OUT our device.** The ESP32 is advertising and visible, but BlueZ's D-Bus filter doesn't match it consistently.

### Root cause theory

The `service_uuids` parameter in `BleakScanner` tells BlueZ to only report devices that include the specified UUID in their **advertisement data**. However:
- The service UUID may only appear in the **scan response** (not the primary advertisement packet)
- Since we removed `enableScanResponse(true)` in the minimal config, the scan response may not be sent at all, or the UUID placement may be inconsistent
- BlueZ's filter operates at the D-Bus level before bleak sees the data — if the UUID isn't in the right place, the device is silently dropped

In Test 9, the filter worked intermittently (3/5 scans), and in Test 3 (before minimal config) it also worked. The inconsistency suggests the UUID is sometimes in the advertisement and sometimes not.

### Recommendation

**Remove the `service_uuids` filter from the scanner** and match by UUID in the detection callback instead (which is already being done). This way:
- BlueZ reports all LE devices (like before — 50+ devices)
- The callback checks `advertisement_data.service_uuids` and only acts on matches
- No silent filtering at the D-Bus level

The current code already does the UUID matching in the callback — the filter is redundant and harmful.

### Cache removal working correctly

```
[00:34:46.5404] [INFO ] Removing 38:44:BE:45:AD:86 from BlueZ cache...
[00:34:46.5487] [DEBUG]   Not in cache (OK):
```

No stale cache entries found (as expected after bluetooth restart).

### Unfiltered scan result

```
Unfiltered scan for 15s...
  ** ESP32: 38:44:BE:45:AD:86 name=EasyPlay
Total: 9 devices, ESP32 FOUND
```

---

## Previous Tests Summary

| Test | Time | Result |
|------|------|--------|
| 10 | 00:34 | **Filtered: not found. Unfiltered: FOUND.** Filter is the problem. |
| 9 | 00:29 | Visible (3/5 filtered scans), connection timeout |
| 8 | 00:18 | Not visible (broken adv config) |
| 7 | 00:10 | Not visible (broken adv config) |
| 6 | 00:00 | Not visible (broken adv config) |
| 5 | 23:50 | Found, bluetoothctl remove broke connect |
| 4 | 23:24 | BlueZ stuck |
| 3 | 23:18 | Found (3 hits), br-connection-canceled |
| 2 | 23:12 | MAC found as "EasyPlay" |
| 1 | 23:00 | Fixed bleak .rssi bug |
