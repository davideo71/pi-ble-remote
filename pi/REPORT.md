# Pi Test Report: Step 1 BLE Connection Test

## Test 5 — 2026-03-15 23:50 UTC (BR/EDR fix + cache removal + service UUID filter)

**Duration:** ~65 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: DEVICE FOUND, connection failed — `bluetoothctl remove` timing issue

**What worked:**
- Bluetooth service restart cleared previous stuck state
- Service UUID filter works perfectly — scanner only reports our device (1 device seen vs 50+ without filter)
- Device found on scan cycle 3 at RSSI -81 dBm
- No more `br-connection-canceled` or `InProgress` errors

**What failed:**
The `bluetoothctl remove` is called BETWEEN scan and connect. This removes the BlueZ device entry that bleak just discovered, so the connect immediately fails:

```
[23:50:41.2879] [INFO ] Found target by service UUID: EasyPlay (38:44:BE:45:AD:86)
[23:50:41.2880] [INFO ] Connecting to 38:44:BE:45:AD:86 (timeout=15.0s)...
[23:50:41.2880] [INFO ] Removing 38:44:BE:45:AD:86 from BlueZ cache...
[23:50:41.3080] [INFO ]   Removed from BlueZ cache
[23:50:41.8137] [ERROR] BLE error: device 'dev_38_44_BE_45_AD_86' not found
```

**Root cause:** `remove_bluez_device()` is called in `connect_and_listen()` right before `BleakClient()`. This destroys the D-Bus device object that bleak needs to initiate the connection. The BLEDevice Python object still exists but the underlying BlueZ device is gone.

### Suggested fix

Move the `bluetoothctl remove` to BEFORE the scan (not between scan and connect). This way:
1. Remove stale cache entry (clears any old BR/EDR association)
2. Scan discovers the device fresh (creates new LE device entry in BlueZ)
3. Connect uses the fresh device entry

Alternatively, skip the `bluetoothctl remove` entirely since the `service_uuids` filter already ensures BlueZ treats it as an LE device.

### Other observations

- **Name still "EasyPlay"** — the ESP32 is still advertising the old name despite flash erase. Check the NimBLE `init()` call in firmware.
- **ESP32 appears intermittently** — 0 devices on scan cycles 1, 2, 4, 5; found on cycle 3 only
- **RSSI -81 dBm** — better than before (-86 to -89), consistent with TX power increase

### Full Output

```
============================================
  BLE Remote Receiver - Raspberry Pi
  Step 1: Connection + Heartbeat Debug
============================================
[23:50:03.6753] [INFO ] Target device: BLE-Remote
[23:50:03.6753] [INFO ] Service UUID:  4e520001-7354-4288-9a71-81a9bf56c4a8
[23:50:03.6754] [INFO ] Char UUID:     4e520002-7354-4288-9a71-81a9bf56c4a8
--------------------------------------------

[23:50:03.6754] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:50:13.7318] [INFO ] Scan complete: 0 devices seen
[23:50:13.7320] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[23:50:13.7320] [WARN ] Retrying in 3s...
[23:50:16.7351] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:50:26.7600] [INFO ] Scan complete: 0 devices seen
[23:50:26.7600] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[23:50:26.7600] [WARN ] Retrying in 4s...
[23:50:31.2631] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:50:35.5112] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-81dBm UUIDs=['4e520001-7354-4288-9a71-81a9bf56c4a8']
[23:50:41.2879] [INFO ] Scan complete: 1 devices seen
[23:50:41.2879] [INFO ] Found target by service UUID: EasyPlay (38:44:BE:45:AD:86)
[23:50:41.2880] [INFO ] Connecting to 38:44:BE:45:AD:86 (timeout=15.0s)...
[23:50:41.2880] [INFO ] Removing 38:44:BE:45:AD:86 from BlueZ cache...
[23:50:41.3080] [INFO ]   Removed from BlueZ cache
[23:50:41.8137] [ERROR] BLE error: device 'dev_38_44_BE_45_AD_86' not found
[23:50:41.8138] [INFO ] Reconnecting in 3s...
[23:50:44.8151] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:50:54.8420] [INFO ] Scan complete: 0 devices seen
[23:50:54.8421] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[23:50:54.8421] [WARN ] Retrying in 3s...
[23:50:57.8432] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:51:07.8681] [INFO ] Scan complete: 0 devices seen
[23:51:07.8682] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[23:51:07.8682] [WARN ] Retrying in 4s...
```

---

## Previous Tests

### Test 4 (23:24) — BlueZ stuck, ESP32 not visible after adapter reset
### Test 3 (23:18) — Device found by UUID (3 hits), connection failed with br-connection-canceled, BlueZ stuck
### Test 2 (23:12) — MAC found, name cached as "EasyPlay", cache cleared
### Test 1 (23:00) — Initial test, fixed bleak 2.x .rssi compat bug
