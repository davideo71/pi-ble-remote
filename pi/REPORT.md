# Pi Test Report: Step 1 BLE Connection Test

## Test 4 — 2026-03-15 23:24 UTC (after TX power fix)

**Duration:** ~50 seconds (4 scan cycles)

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: DEVICE NOT FOUND — BlueZ was stuck, then ESP32 not visible

**First attempt:** BlueZ was still stuck from Test 3's failed connection — every scan immediately failed with `[org.bluez.Error.InProgress] Operation already in progress`. This persisted across all 17 attempts in ~50 seconds.

**Fix applied:** Reset Bluetooth adapter:
```bash
sudo bluetoothctl power off && sleep 1 && sudo bluetoothctl power on
```

**Second attempt (after adapter reset):** Scanner worked again (no more InProgress errors), scanned 4 full cycles (48, 52, 52, ~40 devices), but **MAC `38:44:BE:45:AD:86` did not appear in any scan**. No `** MATCH **` lines.

### Analysis

1. **BlueZ stuck state** from Test 3 required an adapter power cycle to fix — the receiver script should handle this automatically
2. **ESP32 not advertising** after adapter reset — the ESP32 may need a power cycle too, or the TX power change may have caused it to crash/hang
3. **No RSSI improvement observed** — couldn't test the TX power fix since the device wasn't visible
4. **Suggestion:** Check ESP32 serial monitor to confirm it's running and advertising. May need power cycle on ESP32 side.

### Full Output (second attempt, after adapter reset)

```
============================================
  BLE Remote Receiver - Raspberry Pi
  Step 1: Connection + Heartbeat Debug
============================================
[23:26:04.5565] [INFO ] Target device: BLE-Remote
[23:26:04.5566] [INFO ] Service UUID:  4e520001-7354-4288-9a71-81a9bf56c4a8
[23:26:04.5566] [INFO ] Char UUID:     4e520002-7354-4288-9a71-81a9bf56c4a8
--------------------------------------------

[23:26:04.5566] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:26:14.6263] [INFO ] Scan complete: 48 devices seen
[23:26:14.6264] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[23:26:14.6265] [WARN ] Retrying in 3s...
[23:26:17.6296] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:26:27.6616] [INFO ] Scan complete: 52 devices seen
[23:26:27.6617] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[23:26:27.6618] [WARN ] Retrying in 4s...
[23:26:32.1664] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:26:42.2072] [INFO ] Scan complete: 52 devices seen
[23:26:42.2073] [WARN ] Device not found (tried service UUID and name 'BLE-Remote')
[23:26:42.2074] [WARN ] Retrying in 7s...
[23:26:48.9622] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
(timed out during 4th scan)
```

No devices with our service UUID or name "BLE-Remote" (or "EasyPlay") appeared.

### Full Output (first attempt, BlueZ stuck)

```
[23:24:50.4783] [INFO ] Scanning for service UUID 4e520001-7354-4288-9a71-81a9bf56c4a8 (timeout=10.0s)...
[23:24:50.5145] [ERROR] BLE error: [org.bluez.Error.InProgress] Operation already in progress
(repeated 17 times over ~50 seconds)
```

---

## Test 3 — 2026-03-15 23:18 UTC (UUID-based scanning)

### Result: DEVICE FOUND by UUID, connection failed

UUID scan matched `38:44:BE:45:AD:86` (3 hits, RSSI -81 to -85 dBm). Connection failed with `br-connection-canceled`, then BlueZ got stuck with "InProgress" errors.

---

## Test 2 — 2026-03-15 23:12 UTC (after ESP32 flash erase + reflash)

### Result: ESP32 MAC found, name cached as "EasyPlay"

MAC visible but BlueZ cached old name. Cleared cache. Signal weak (-86 to -89 dBm).

---

## Test 1 — 2026-03-15 23:00 UTC (initial test)

### Result: BLE-Remote NOT FOUND

Fixed bleak 2.x `.rssi` compat bug. Pi BLE stack working (50+ devices per scan).
