# Pi Test Report: Step 1 BLE Connection Test

## Test 2 — 2026-03-15 23:12 UTC (after ESP32 flash erase + reflash)

**Duration:** ~35 seconds (3 scan cycles), plus bluetoothctl scan and cache investigation

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: ESP32 MAC found, but name still cached as "EasyPlay"

**Key finding:** MAC `38:44:BE:45:AD:86` appeared in scan cycle 1 but BlueZ reported the **cached name "EasyPlay"** instead of "BLE-Remote".

`bluetoothctl info` confirmed the cache:
```
Device 38:44:BE:45:AD:86 (public)
    Name: EasyPlay
    Alias: EasyPlay
    UUID: Vendor specific (4e520001-7354-4288-9a71-81a9bf56c4a8)
```

The correct service UUID `4e520001...` confirms this is our ESP32-C3.

### Actions taken

1. Cleared BlueZ cache: `bluetoothctl remove 38:44:BE:45:AD:86`
2. Re-ran scanner — MAC `38:44:BE:45:AD:86` did not appear in the next 3 scan cycles (intermittent visibility, RSSI was -86 to -89 dBm when seen)

### bluetoothctl raw scan output (excerpt)

```
Device 38:44:BE:45:AD:86 RSSI: 0xffffffa7 (-89)
Device 38:44:BE:45:AD:86 RSSI: 0xffffffaa (-86)
Device 38:44:BE:45:AD:86 RSSI: 0xffffffaa (-86)
```

The device was seen at -86 to -89 dBm — quite weak, which may explain intermittent visibility.

### ble_receiver.py output (scan cycle 1 — MAC visible)

```
============================================
  BLE Remote Receiver - Raspberry Pi
  Step 1: Connection + Heartbeat Debug
============================================
[23:12:45.8991] [INFO ] Target device: BLE-Remote
[23:12:45.8992] [INFO ] Service UUID:  4e520001-7354-4288-9a71-81a9bf56c4a8
[23:12:45.8992] [INFO ] Char UUID:     4e520002-7354-4288-9a71-81a9bf56c4a8
--------------------------------------------

[23:12:45.8992] [INFO ] Scanning for device named 'BLE-Remote' (timeout=10.0s)...
[23:12:46.2300] [DEBUG]   SCAN: 38:44:BE:45:AD:86 name='EasyPlay' RSSI=-89dBm
[23:12:55.9647] [INFO ] Scan complete: 43 devices seen
[23:12:55.9648] [WARN ] Device 'BLE-Remote' not found in scan results
```

### ble_receiver.py output (after cache clear — MAC not visible)

```
[23:13:58.4013] [INFO ] Scanning for device named 'BLE-Remote' (timeout=10.0s)...
[23:14:08.4632] [INFO ] Scan complete: 52 devices seen
[23:14:08.4633] [WARN ] Device 'BLE-Remote' not found in scan results
[23:14:21.5108] [INFO ] Scan complete: 48 devices seen
[23:14:21.5109] [WARN ] Device 'BLE-Remote' not found in scan results
```

MAC `38:44:BE:45:AD:86` did not appear in any of the 3 post-cache-clear cycles.

### Errors and Warnings

- **No errors** — scanner runs cleanly
- BlueZ was caching stale device name "EasyPlay" from before the flash erase
- Cache was cleared with `bluetoothctl remove`

### Analysis

1. **The ESP32-C3 IS advertising** — we see MAC `38:44:BE:45:AD:86` with the correct service UUID
2. **BlueZ was caching the old name "EasyPlay"** — even after flash erase + reflash on the ESP32, the Pi's BlueZ retained the old name
3. **Signal is weak** (-86 to -89 dBm) — the device appears intermittently
4. **After cache clear**, the device wasn't seen in 3 cycles — could be timing/signal, or the ESP32 may need a power cycle too
5. **Next steps:** Verify on the ESP32 serial monitor that it's actually advertising as "BLE-Remote". Consider moving devices closer together for testing. The receiver could also fall back to scanning by service UUID instead of device name for more reliable discovery.

---

## Test 1 — 2026-03-15 23:00 UTC (initial test)

**Duration:** ~35 seconds (3 full scan cycles)

### Result: BLE-Remote NOT FOUND

The scanner ran 3 full 10-second scan cycles. It detected 51, 37, and ~30+ BLE devices respectively, but "BLE-Remote" was not found. MAC `38:44:BE:45:AD:86` appeared as "EasyPlay" (stale cached name).

### Notes

- Fixed bleak 2.x compat bug (`.rssi` moved from `BLEDevice` to `AdvertisementData`) — 3 occurrences in `ble_receiver.py`
- Pi BLE stack working — sees 50+ devices per scan cycle
- Reconnection backoff working correctly (3s -> 4s)
