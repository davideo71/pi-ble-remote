# Pi Test Report: Step 1 BLE Connection Test

## Test 7 — 2026-03-16 00:10 UTC (faster advertising interval)

**Duration:** ~90 seconds (6 scan cycles) + unfiltered verification scans

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: ESP32 NOT VISIBLE — confirmed offline with multiple methods

**0 devices** across 6 filtered scan cycles (90 seconds).

Verified with two additional unfiltered scans:
1. `bluetoothctl --timeout 12 scan le` — MAC `38:44:BE:45:AD:86` not present
2. `BleakScanner.discover(timeout=10)` without `service_uuids` filter — only 8 devices found, ESP32 not among them

**The ESP32 is definitely not advertising.** This is not a filter issue — it's absent from all scan methods.

### Analysis

- The faster advertising interval (20-100ms) couldn't be tested since the ESP32 isn't advertising at all
- The reduced total device count (8 vs 50+ in earlier tests) suggests the BLE environment is quieter at this time, making ESP32 absence more definitive
- **Possible causes:**
  - ESP32 crashed during/after reflash
  - Power issue (batteries depleted?)
  - Firmware bug causing advertising to not start
  - Check ESP32 serial monitor for boot messages / errors
- **Suggestion:** Check if the ESP32's serial monitor shows it booted and started advertising. Try pressing the reset button.

### Full Output (filtered scan)

```
============================================
  BLE Remote Receiver - Raspberry Pi
  Step 1: Connection + Heartbeat Debug
============================================
[00:10:10.9332] [INFO ] Target device: BLE-Remote
[00:10:10.9333] [INFO ] Service UUID:  4e520001-7354-4288-9a71-81a9bf56c4a8
[00:10:10.9333] [INFO ] Char UUID:     4e520002-7354-4288-9a71-81a9bf56c4a8
--------------------------------------------

[00:10:10.9333] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:10:20.9880] [INFO ] Scan complete: 0 devices seen
[00:10:20.9881] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
(repeated 5 more times, 0 devices each cycle, backoff 3s → 4s → 7s → 10s → 15s)
```

### Unfiltered verification

```python
# BleakScanner.discover(timeout=10) without service_uuids filter
# Result: ESP32 MAC not found among 8 devices
```

```bash
# bluetoothctl --timeout 12 scan le | grep "38:44:BE:45:AD:86"
# (no output)
```

---

## Previous Tests

### Test 6 (00:00) — ESP32 not visible, 0 devices (UUID filter), confirmed offline via bluetoothctl
### Test 5 (23:50) — Device found by UUID, bluetoothctl remove between scan/connect broke connection
### Test 4 (23:24) — BlueZ stuck, ESP32 not visible after adapter reset
### Test 3 (23:18) — Device found by UUID (3 hits, -81 dBm), connection failed with br-connection-canceled
### Test 2 (23:12) — MAC found, name cached as "EasyPlay"
### Test 1 (23:00) — Initial test, fixed bleak 2.x .rssi compat bug
