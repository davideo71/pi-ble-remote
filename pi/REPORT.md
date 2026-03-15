# Pi Test Report: Step 1 BLE Connection Test

**Date:** 2026-03-15 23:00 UTC
**Duration:** ~35 seconds (3 full scan cycles)

## Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

## Result: BLE-Remote NOT FOUND

The scanner ran 3 full 10-second scan cycles. It detected 51, 37, and ~30+ BLE devices respectively, but **"BLE-Remote" was not among them**.

Named devices seen: `EasyPlay`, `S41 152A LE`, `IAe-65" The Frame` — no `BLE-Remote`.

## Full Output

```
============================================
  BLE Remote Receiver - Raspberry Pi
  Step 1: Connection + Heartbeat Debug
============================================
[23:00:05.2561] [INFO ] Target device: BLE-Remote
[23:00:05.2561] [INFO ] Service UUID:  4e520001-7354-4288-9a71-81a9bf56c4a8
[23:00:05.2562] [INFO ] Char UUID:     4e520002-7354-4288-9a71-81a9bf56c4a8
--------------------------------------------

[23:00:05.2562] [INFO ] Scanning for device named 'BLE-Remote' (timeout=10.0s)...
[23:00:05.5457] [DEBUG]   SCAN: 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-84dBm
[23:00:05.5464] [DEBUG]   SCAN: D9:D4:E7:A0:C2:B5 name=None RSSI=-79dBm
[23:00:05.6176] [DEBUG]   SCAN: 68:07:B8:2C:F5:51 name='S41 152A LE' RSSI=-88dBm
[23:00:05.7176] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-89dBm
[23:00:05.7209] [DEBUG]   SCAN: 52:78:5C:78:0E:94 name=None RSSI=-88dBm
[23:00:05.7241] [DEBUG]   SCAN: 5F:B0:8C:2A:F1:AA name=None RSSI=-84dBm
[23:00:06.0226] [DEBUG]   SCAN: 68:07:B8:2C:F5:51 name='S41 152A LE' RSSI=-88dBm
[23:00:06.0230] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-80dBm
[23:00:06.8299] [DEBUG]   SCAN: 5F:B0:8C:2A:F1:AA name=None RSSI=-84dBm
[23:00:07.2353] [DEBUG]   SCAN: 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-84dBm
[23:00:15.3163] [INFO ] Scan complete: 51 devices seen
[23:00:15.3164] [WARN ] Device 'BLE-Remote' not found in scan results
[23:00:15.3165] [WARN ] Retrying in 3s...
[23:00:18.3197] [INFO ] Scanning for device named 'BLE-Remote' (timeout=10.0s)...
[23:00:18.4476] [DEBUG]   SCAN: 51:B6:93:25:17:44 name=None RSSI=-85dBm
[23:00:18.7171] [DEBUG]   SCAN: 51:B6:93:25:17:44 name=None RSSI=-85dBm
[23:00:18.7532] [DEBUG]   SCAN: 52:78:5C:78:0E:94 name=None RSSI=-88dBm
[23:00:18.9594] [DEBUG]   SCAN: 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-84dBm
[23:00:19.5634] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-84dBm
[23:00:20.0673] [DEBUG]   SCAN: 52:78:5C:78:0E:94 name=None RSSI=-88dBm
[23:00:20.2712] [DEBUG]   SCAN: 5F:B0:8C:2A:F1:AA name=None RSSI=-84dBm
[23:00:20.3372] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-87dBm
[23:00:20.6741] [DEBUG]   SCAN: 51:B6:93:25:17:44 name=None RSSI=-85dBm
[23:00:20.6770] [DEBUG]   SCAN: 68:07:B8:2C:F5:51 name='S41 152A LE' RSSI=-88dBm
[23:00:28.3512] [INFO ] Scan complete: 37 devices seen
[23:00:28.3513] [WARN ] Device 'BLE-Remote' not found in scan results
[23:00:28.3514] [WARN ] Retrying in 4s...
[23:00:32.8561] [INFO ] Scanning for device named 'BLE-Remote' (timeout=10.0s)...
[23:00:33.0498] [DEBUG]   SCAN: A8:51:AB:8F:DE:49 name=None RSSI=-88dBm
[23:00:33.0507] [DEBUG]   SCAN: 51:B6:93:25:17:44 name=None RSSI=-90dBm
[23:00:33.1541] [DEBUG]   SCAN: 68:07:B8:2C:F5:51 name='S41 152A LE' RSSI=-80dBm
[23:00:33.1557] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-78dBm
[23:00:33.1859] [DEBUG]   SCAN: 52:78:5C:78:0E:94 name=None RSSI=-89dBm
[23:00:33.4558] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-82dBm
[23:00:33.7657] [DEBUG]   SCAN: 68:07:B8:2C:F5:51 name='S41 152A LE' RSSI=-88dBm
[23:00:33.9007] [DEBUG]   SCAN: 68:07:B8:2C:F5:51 name='S41 152A LE' RSSI=-84dBm
[23:00:34.8061] [DEBUG]   SCAN: 68:07:B8:2C:F5:51 name='S41 152A LE' RSSI=-88dBm
[23:00:34.9770] [DEBUG]   SCAN: 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-84dBm
```

## Errors and Warnings

- **No errors** — the scanner ran cleanly after a bleak 2.x compatibility fix (`.rssi` moved from `BLEDevice` to `AdvertisementData`)
- **3x WARN:** `Device 'BLE-Remote' not found in scan results`

## Notes

- The Pi's BLE stack is working — it sees 50+ devices per scan cycle
- The ESP32-C3 may not be advertising, may be out of range, or the device name may not match exactly
- The bleak `.rssi` deprecation was fixed in `ble_receiver.py` before this test (3 occurrences)
- Reconnection backoff is working correctly (3s → 4s)
