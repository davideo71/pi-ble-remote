# Pi Test Report: Step 1 BLE Connection Test

## Test 9 — 2026-03-16 00:29 UTC (minimal advertising config reverted)

**Duration:** ~90 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: DEVICE CONSISTENTLY VISIBLE, connection times out

**Scanning:** ESP32 is reliably visible again! Found on 3 out of 5 scan cycles (cycles 2, 4, 5). The minimal advertising config fixed the visibility issue from Tests 6-8.

**Connection:** Both connection attempts **timed out** after 15 seconds each. This is different from Test 3's `br-connection-canceled` — now it's a genuine timeout, meaning BlueZ is attempting an LE connection but the ESP32 isn't completing the handshake.

**Adapter auto-reset:** Working correctly — recovered from "InProgress" errors automatically after each failed connection.

### Timeline

```
[00:29:43] Scan 1: 0 devices (missed)
[00:29:56] Scan 2: ** MATCH ** 38:44:BE:45:AD:86 RSSI=-88dBm
[00:30:06] Connect attempt 1 → TIMEOUT after 15s
[00:30:24] Scan 3: InProgress error → adapter auto-reset
[00:30:30] Scan 4: ** MATCH ** (2 hits) RSSI=-88dBm
[00:30:40] Connect attempt 2 → TIMEOUT after 15s
[00:30:58] Scan 5: InProgress error → adapter auto-reset
[00:31:05] Scan 6: ** MATCH ** RSSI=-88dBm (timed out before connect)
```

### Analysis

1. **Visibility fixed** — the minimal advertising config works. The broken setMinInterval/setMaxInterval/enableScanResponse from Tests 6-8 was the problem.
2. **Connection timeout** — BlueZ tries to connect for 15s and gives up. Possible causes:
   - **ESP32 not responding to connection requests** — it may be advertising but not listening for connection indications
   - **RSSI -88 dBm is borderline** — just barely visible, may be too weak for reliable connection establishment
   - **BlueZ cache/state issue** — the `remove_bluez_device()` call before connect might still be interfering (check if it's still in the code)
   - **NimBLE server not ready** — the GATT server might not be properly initialized despite advertising
3. **Name still "EasyPlay"** — consistent, cosmetic issue
4. **Adapter auto-reset working** — good, handles the stuck state automatically

### Suggested next steps

- **Check if `remove_bluez_device()` is still called before connect** — this was identified as problematic in Test 5
- **Try connecting without cache removal** — just scan and connect directly
- **Move devices closer** — -88 dBm is marginal
- **Check ESP32 server callbacks** — is `onConnect` being triggered? If not, the ESP32 may not be accepting connections
- **Try `bluetoothctl connect 38:44:BE:45:AD:86`** manually to see what BlueZ reports

### Full Output

```
[00:29:43.6337] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:29:53.6826] [INFO ] Scan complete: 0 devices seen
[00:29:53.6826] [WARN ] Device not found
[00:29:56.6858] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:30:03.8700] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-88dBm UUIDs=['4e520001-7354-4288-9a71-81a9bf56c4a8']
[00:30:06.7091] [INFO ] Found target by service UUID: EasyPlay (38:44:BE:45:AD:86)
[00:30:06.7092] [INFO ] Connecting to 38:44:BE:45:AD:86 (timeout=15.0s)...
[00:30:21.7347] [ERROR] Connection timed out
[00:30:24.7485] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[00:30:24.7486] [WARN ] BlueZ stuck — resetting Bluetooth adapter...
[00:30:27.9039] [INFO ] Bluetooth adapter reset complete
[00:30:30.9071] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:30:35.5574] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-88dBm
[00:30:37.0123] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-88dBm
[00:30:40.9273] [INFO ] Found target by service UUID: EasyPlay (38:44:BE:45:AD:86)
[00:30:40.9274] [INFO ] Connecting to 38:44:BE:45:AD:86 (timeout=15.0s)...
[00:30:55.9424] [ERROR] Connection timed out
[00:30:58.9581] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
[00:30:58.9582] [WARN ] BlueZ stuck — resetting Bluetooth adapter...
[00:31:02.1109] [INFO ] Bluetooth adapter reset complete
[00:31:05.1142] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[00:31:11.1184] [DEBUG]   SCAN: ** MATCH ** 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-88dBm
```

---

## Previous Tests Summary

| Test | Time | Result |
|------|------|--------|
| 9 | 00:29 | **Visible (3/5 scans, -88 dBm), connection TIMEOUT** |
| 8 | 00:18 | Not visible (ESP32 running but broken adv config) |
| 7 | 00:10 | Not visible (broken adv config) |
| 6 | 00:00 | Not visible (broken adv config) |
| 5 | 23:50 | Found, bluetoothctl remove broke connect |
| 4 | 23:24 | BlueZ stuck, not visible after reset |
| 3 | 23:18 | Found (3 hits, -81 dBm), br-connection-canceled |
| 2 | 23:12 | MAC found as "EasyPlay" |
| 1 | 23:00 | Not found, fixed bleak .rssi bug |
