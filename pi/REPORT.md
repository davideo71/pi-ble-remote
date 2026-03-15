# Pi Test Report: Step 1 BLE Connection Test

## Test 6 — 2026-03-16 00:00 UTC (cache removal moved before scan)

**Duration:** ~90 seconds (6 scan cycles) + separate bluetoothctl scan

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: ESP32 NOT VISIBLE — appears to be offline

**0 devices seen** across all 6 scan cycles (~90 seconds total). The `service_uuids` filter is active, so only devices advertising our UUID would show up.

Verified with unfiltered `bluetoothctl scan le` (10 seconds) — MAC `38:44:BE:45:AD:86` was **not present** in the raw scan either. The ESP32 is not advertising at all right now.

### Analysis

- **The ESP32 appears to be offline or not advertising.** Previous tests showed it intermittently (1 out of 5 scans in Test 5, found on cycle 2 in Test 3). But now it's completely absent even from raw bluetoothctl scans.
- **Could not test the cache removal fix** since the device never appeared.
- **Possible causes:** ESP32 may have crashed, battery died, or is in a hung state. May need physical power cycle.
- **Note on `service_uuids` filter:** When the filter is active, bleak reports 0 devices total (not 50+ like before). This is correct — BlueZ filters at the D-Bus level. However, this means we can't tell if the Pi's BLE stack is working. Consider logging the filter status or doing an occasional unfiltered scan as a health check.

### Full Output

```
============================================
  BLE Remote Receiver - Raspberry Pi
  Step 1: Connection + Heartbeat Debug
============================================
[00:00:12.2223] [INFO ] Target device: BLE-Remote
[00:00:12.2224] [INFO ] Service UUID:  4e520001-7354-4288-9a71-81a9bf56c4a8
[00:00:12.2224] [INFO ] Char UUID:     4e520002-7354-4288-9a71-81a9bf56c4a8
--------------------------------------------

[00:00:12.2224] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:00:22.2715] [INFO ] Scan complete: 0 devices seen
[00:00:22.2716] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[00:00:22.2716] [WARN ] Retrying in 3s...
[00:00:25.2748] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:00:35.2995] [INFO ] Scan complete: 0 devices seen
[00:00:35.2995] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[00:00:35.2996] [WARN ] Retrying in 4s...
[00:00:39.8030] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:00:49.8298] [INFO ] Scan complete: 0 devices seen
[00:00:49.8299] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[00:00:49.8299] [WARN ] Retrying in 7s...
[00:00:56.5803] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:01:06.6083] [INFO ] Scan complete: 0 devices seen
[00:01:06.6083] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[00:01:06.6083] [WARN ] Retrying in 10s...
[00:01:16.7361] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:01:26.7702] [INFO ] Scan complete: 0 devices seen
[00:01:26.7703] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[00:01:26.7704] [WARN ] Retrying in 15s...
[00:01:41.7868] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
(timed out during cycle 6)
```

### bluetoothctl verification

```bash
bluetoothctl --timeout 10 scan le | grep "38:44:BE:45:AD:86"
# (no output — device not present)
```

---

## Previous Tests

### Test 5 (23:50) — Device found by UUID, but bluetoothctl remove between scan/connect destroyed BlueZ entry
### Test 4 (23:24) — BlueZ stuck (InProgress), fixed with adapter reset, ESP32 not visible after
### Test 3 (23:18) — Device found by UUID (3 hits, -81 dBm), connection failed with br-connection-canceled
### Test 2 (23:12) — MAC found, name cached as "EasyPlay", cache cleared
### Test 1 (23:00) — Initial test, fixed bleak 2.x .rssi compat bug
