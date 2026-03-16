# Pi Test Report: Step 2 — Button Handling

## Test 28b — 2026-03-17 00:36 UTC (scan first + service restart + simulated buttons)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: PROGRESS — ESP32 found by scan, but ALL connections fail

#### Good news: ESP32 IS advertising
- Scan-based discovery works — device found **4 times** across scans
- First match by **UUID** (scan 3), then by **MAC** (scans 4-6)
- RSSI: -85 to -89 dBm (normal range)
- The scan-first approach and service restart brought the ESP32 back into view

#### Bad news: Every connection drops during setup
- **Attempt 1**: Found by UUID → connect → `br-connection-canceled` after 5.3s
- **Attempt 2**: Found by MAC → connect → DISCONNECTED after 5.2s → timeout
- **Attempt 3**: Found by MAC → connect → "failed to discover services, device disconnected" after 7.3s
- **Attempt 4**: Found by MAC → connect → DISCONNECTED after 19.5s → timeout

Every connection attempt reaches the ESP32 (DISCONNECTED callback fires = a connection was partially established) but drops during GATT service discovery. This is the **exact same pattern as Test 25**.

#### Error: `br-connection-canceled`
This error appeared on the first connection attempt. It means BlueZ tried a **BR/EDR (classic Bluetooth) connection** instead of BLE. This happens when BlueZ doesn't know the device is LE-only. After `bluetoothctl remove`, BlueZ loses the LE flag and may default to BR/EDR on the next connection.

### Analysis

The connection failures are **consistent with the button firmware on the ESP32**:
- Tests 21-23 (pre-button firmware): connections work perfectly
- Tests 24-28 (button firmware): every connection drops during service discovery

The ESP32 is advertising (we can see it) but **drops the connection during GATT operations**. This strongly suggests:

1. **The simulated button code is interfering with BLE GATT operations** — sending notifications during service discovery may confuse the NimBLE stack
2. **The 500ms button simulation interval may be too aggressive** — the ESP32 may be trying to send notifications before the client has finished subscribing
3. **The ESP32 may need to wait for a client to subscribe** before sending button notifications

### Suggestion
1. **On ESP32: only send button notifications AFTER a client subscribes** (check `pCharacteristic->getSubscribedCount() > 0` before notify)
2. **On ESP32: delay button simulation start by 5-10 seconds** after a connection to let GATT discovery complete
3. **Or: disable simulated buttons entirely** and test with real button hardware (which only sends on physical press)

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 28b (00:36) | ESP32 found by scan, but all connections drop during GATT discovery |
| 28 (00:32) | FAILED — "Device not found" (pre-scan-first code) |
| 27-26 | FAILED — "Device not found" |
| 25 | FAILED — ESP32 drops connections (same pattern as 28b) |
| 24 | FAILED — InProgress errors |
| 23 | Last successful connection (heartbeats, no buttons) |
| 22-21 | Success — direct connect by MAC |
| 18 | Best scan+connect, 9s |
| 14 | First stable connection (3m41s) |
