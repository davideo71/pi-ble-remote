# Pi Test Report: Step 1 BLE Connection Test

## Test 8 — 2026-03-16 00:18 UTC (ESP32 confirmed running via serial)

**Duration:** ~90 seconds filtered + 15s unfiltered + 15s bluetoothctl

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: ESP32 NOT VISIBLE despite being confirmed running

**Three independent scan methods, all negative:**

| Method | Duration | Devices seen | ESP32 found? |
|--------|----------|-------------|--------------|
| bleak + service_uuids filter | 90s (6 cycles) | 0 | No |
| bleak unfiltered | 15s | 10 unique | No |
| bluetoothctl scan le | 15s | ~10 | No |

### Unfiltered scan details

```
Total unique devices: 10
Named devices:
  79:3A:5B:87:09:7F name='S41 152A LE' RSSI=-84dBm UUIDs=['0000fe07-...']
  94:E6:BA:80:A1:6B name='IAe-65" The Frame' RSSI=-78dBm UUIDs=[]
```

MAC `38:44:BE:45:AD:86` did NOT appear. No device named "EasyPlay" or "BLE-Remote".

### Analysis

The ESP32 serial monitor shows:
- TX power 20 dBm
- Advertising interval 20-100ms
- BLE address `38:44:be:45:ad:86`
- Heap stable at 202712 bytes

But the Pi sees nothing. **This has been the case for Tests 6, 7, and 8 now (~30 minutes).**

**Possible explanations:**
1. **ESP32 thinks it's advertising but NimBLE isn't actually transmitting** — the `NimBLEDevice::startAdvertising()` call may have succeeded at the API level but the radio isn't actually sending packets. This can happen with NimBLE if the service/characteristic setup has an issue.
2. **RF issue** — antenna disconnected, hardware fault, or ESP32-C3 variant without external antenna
3. **NimBLE 2.x API change** — the advertising interval or power settings may not be applied correctly in the version being used
4. **Different BLE address** — the ESP32 may be using a different/random address than what the serial monitor reports

**Suggested next steps:**
- Use **nRF Connect** (phone app) to scan — this rules out Pi-side issues entirely
- Try `hcitool lescan` as root on the Pi for a lower-level scan
- Check if the ESP32 has a physical antenna issue
- Try adding `pAdvertising->setMinInterval(0x20);` / `setMaxInterval(0x40);` explicitly in NimBLE (these are in units of 0.625ms, so 0x20=20ms, 0x40=40ms)

### Previous Tests Summary

| Test | Time | Result |
|------|------|--------|
| 8 | 00:18 | Not visible (3 methods, ESP32 confirmed running) |
| 7 | 00:10 | Not visible (ESP32 was offline) |
| 6 | 00:00 | Not visible (ESP32 was offline) |
| 5 | 23:50 | Found by UUID, bluetoothctl remove broke connect |
| 4 | 23:24 | BlueZ stuck, not visible after adapter reset |
| 3 | 23:18 | **Found by UUID (3 hits, -81 dBm)**, connection failed br-connection-canceled |
| 2 | 23:12 | MAC found as "EasyPlay", cache cleared |
| 1 | 23:00 | Not found, fixed bleak .rssi bug |
