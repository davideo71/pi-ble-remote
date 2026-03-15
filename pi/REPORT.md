# Pi Test Report: Step 1 BLE Connection Test

## Test 3 — 2026-03-15 23:18 UTC (UUID-based scanning)

**Duration:** ~50 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: DEVICE FOUND by UUID, connection failed

**Scanning by service UUID works!** The device was found on scan cycle 2:

```
[23:19:06.5187] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-85dBm UUIDs=['4e520001-7354-4288-9a71-81a9bf56c4a8']
[23:19:10.8658] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-81dBm UUIDs=['4e520001-7354-4288-9a71-81a9bf56c4a8']
[23:19:13.8720] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-81dBm UUIDs=['4e520001-7354-4288-9a71-81a9bf56c4a8']
[23:19:16.3196] [INFO ] Found target by service UUID: EasyPlay (38:44:BE:45:AD:86)
```

**Connection attempt failed immediately:**

```
[23:19:16.3198] [INFO ] Connecting to 38:44:BE:45:AD:86 (timeout=15.0s)...
[23:19:19.8232] [WARN ] DISCONNECTED from 38:44:BE:45:AD:86
[23:19:19.8235] [ERROR] BLE error: [org.bluez.Error.Failed] br-connection-canceled
```

**BlueZ then got stuck** — all subsequent scan attempts failed with:
```
[ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
```

This repeated for 8+ cycles until the script was killed.

### Observations

1. **UUID-based scanning works** — the service UUID `4e520001-...` is present in advertisements, matching is reliable
2. **Device name still shows as "EasyPlay"** — BlueZ is getting the name from the device's scan response, not from cache (we cleared cache before this test). The ESP32 may still be advertising the old name.
3. **RSSI improved** — -81 to -85 dBm (previously -86 to -89), still not great
4. **`br-connection-canceled`** — this is a BR/EDR (classic Bluetooth) connection error, not BLE. BlueZ may be trying a classic connection instead of LE. This could be a bleak/BlueZ issue with device type detection.
5. **"Operation already in progress"** — after the failed connection, BlueZ's internal state got stuck. The script needs to handle this (possibly restart the Bluetooth adapter or add a longer delay).

### Possible fixes

- **Connection type:** Force LE transport in BleakClient (bleak may not be doing this correctly for this device)
- **Stuck state recovery:** Add error handling to reset BlueZ adapter when "InProgress" errors occur (e.g., `bluetoothctl power off && power on`)
- **Name issue:** Check ESP32 serial monitor — it may still be advertising as "EasyPlay" despite the flash erase

### Full Output

```
============================================
  BLE Remote Receiver - Raspberry Pi
  Step 1: Connection + Heartbeat Debug
============================================
[23:18:53.2197] [INFO ] Target device: BLE-Remote
[23:18:53.2198] [INFO ] Service UUID:  4e520001-7354-4288-9a71-81a9bf56c4a8
[23:18:53.2198] [INFO ] Char UUID:     4e520002-7354-4288-9a71-81a9bf56c4a8
--------------------------------------------

[23:18:53.2198] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:18:53.2732] [DEBUG]   SCAN: 46:AB:B3:91:64:FF name='S41 152A LE' RSSI=-80dBm
[23:18:53.4773] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-80dBm
[23:18:53.6410] [DEBUG]   SCAN: 59:7E:C3:2D:16:9A name=None RSSI=-89dBm
[23:18:53.6801] [DEBUG]   SCAN: 46:AB:B3:91:64:FF name='S41 152A LE' RSSI=-80dBm
[23:18:53.7783] [DEBUG]   SCAN: 51:B6:93:25:17:44 name=None RSSI=-85dBm
[23:18:53.7794] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-89dBm
[23:18:53.8821] [DEBUG]   SCAN: 4A:3F:4E:E4:3B:AE name=None RSSI=-88dBm
[23:18:54.0460] [DEBUG]   SCAN: 51:B6:93:25:17:44 name=None RSSI=-81dBm
[23:18:54.0852] [DEBUG]   SCAN: 46:AB:B3:91:64:FF name='S41 152A LE' RSSI=-84dBm
[23:18:54.0885] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-84dBm
[23:19:03.2792] [INFO ] Scan complete: 51 devices seen
[23:19:03.2793] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[23:19:03.2794] [WARN ] Retrying in 3s...
[23:19:06.2831] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:19:06.3097] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-85dBm
[23:19:06.4786] [DEBUG]   SCAN: 59:7E:C3:2D:16:9A name=None RSSI=-84dBm
[23:19:06.5187] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-85dBm UUIDs=['4e520001-7354-4288-9a71-81a9bf56c4a8']
[23:19:07.3267] [DEBUG]   SCAN: A8:51:AB:8F:DE:49 name=None RSSI=-84dBm
[23:19:07.3277] [DEBUG]   SCAN: 4A:3F:4E:E4:3B:AE name=None RSSI=-84dBm
[23:19:07.6938] [DEBUG]   SCAN: 94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-87dBm
[23:19:07.7975] [DEBUG]   SCAN: 59:7E:C3:2D:16:9A name=None RSSI=-84dBm
[23:19:07.8286] [DEBUG]   SCAN: 51:B6:93:25:17:44 name=None RSSI=-82dBm
[23:19:08.0988] [DEBUG]   SCAN: 51:B6:93:25:17:44 name=None RSSI=-85dBm
[23:19:08.1399] [DEBUG]   SCAN: 46:AB:B3:91:64:FF name='S41 152A LE' RSSI=-88dBm
[23:19:10.8658] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-81dBm UUIDs=['4e520001-7354-4288-9a71-81a9bf56c4a8']
[23:19:13.8720] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-81dBm UUIDs=['4e520001-7354-4288-9a71-81a9bf56c4a8']
[23:19:16.3194] [INFO ] Scan complete: 53 devices seen
[23:19:16.3196] [INFO ] Found target by service UUID: EasyPlay (38:44:BE:45:AD:86)
[23:19:16.3198] [INFO ] Connecting to 38:44:BE:45:AD:86 (timeout=15.0s)...
[23:19:19.8232] [WARN ] DISCONNECTED from 38:44:BE:45:AD:86
[23:19:19.8235] [ERROR] BLE error: [org.bluez.Error.Failed] br-connection-canceled
[23:19:19.8236] [INFO ] Reconnecting in 3s...
[23:19:22.8268] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:19:22.8369] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[23:19:22.8369] [INFO ] Reconnecting in 3s...
[23:19:25.8390] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:19:25.8469] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[23:19:25.8470] [INFO ] Reconnecting in 3s...
[23:19:28.8501] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:19:28.8571] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[23:19:28.8572] [INFO ] Reconnecting in 3s...
[23:19:31.8590] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:19:31.8674] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[23:19:31.8674] [INFO ] Reconnecting in 3s...
[23:19:34.8706] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:19:34.8773] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[23:19:34.8773] [INFO ] Reconnecting in 3s...
[23:19:37.8791] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:19:37.8875] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[23:19:37.8875] [INFO ] Reconnecting in 3s...
[23:19:40.8908] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:19:40.8978] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[23:19:40.8979] [INFO ] Reconnecting in 3s...
```

---

## Test 2 — 2026-03-15 23:12 UTC (after ESP32 flash erase + reflash)

### Result: ESP32 MAC found, but name still cached as "EasyPlay"

MAC `38:44:BE:45:AD:86` appeared but BlueZ reported cached name "EasyPlay". Cleared cache with `bluetoothctl remove`. Signal weak (-86 to -89 dBm).

---

## Test 1 — 2026-03-15 23:00 UTC (initial test)

### Result: BLE-Remote NOT FOUND

Scanner ran cleanly after bleak 2.x compat fix. 50+ devices seen per cycle but no "BLE-Remote". Fixed `.rssi` deprecation in `ble_receiver.py`.
