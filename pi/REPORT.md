# Pi Test Report: Step 1 BLE Connection Test

## Test 11 — 2026-03-16 00:40 UTC (removed service_uuids filter)

**Duration:** ~90s receiver + 15s + 20s unfiltered scans

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: ESP32 NOT VISIBLE — intermittency problem

**Receiver (unfiltered, 5 scan cycles, 90s):** 46-51 devices per cycle, no `** MATCH **` lines. The ESP32 MAC `38:44:BE:45:AD:86` did not appear at all, and no service UUID match triggered.

**Additional unfiltered scans (15s + 20s):** ESP32 not found. 88 detection callbacks in the 20s scan, none for our MAC or "EasyPlay".

### Key observation: service UUID may not be in advertisement data

The receiver was scanning unfiltered and matching by UUID in the callback (`advertisement_data.service_uuids`). Even if the device WAS seen (which it wasn't this time), the service UUID might not be in the advertisement data — it could be in the scan response only, or not included at all with the minimal advertising config.

**Previous evidence:**
- Test 10 (5 minutes earlier): unfiltered `BleakScanner.discover()` found it as "EasyPlay"
- Test 9 (15 minutes earlier): filtered scan found it with UUID 3 times
- Now: completely absent

### The intermittency pattern

| Test | Time gap | ESP32 visible? |
|------|----------|---------------|
| 3 | 23:18 | Yes (3 hits) |
| 5 | 23:50 | Yes (1 hit) |
| 9 | 00:29 | Yes (3 hits) |
| 10 unfiltered | 00:37 | Yes (1 hit) |
| 11 | 00:40 | No |

The ESP32 seems to advertise in bursts with long gaps. With the minimal config, the default NimBLE advertising interval is ~1280ms (1.28 seconds). The device should appear multiple times in a 10-second scan window.

### Possible causes

1. **ESP32 stops advertising after a while** — NimBLE might stop advertising after a timeout or connection attempt
2. **NimBLE advertising needs `setScanResponse(true)`** — without scan response enabled, the service UUID may only appear intermittently in advertising packets
3. **ESP32 crash/hang** — it may be crashing silently and only advertising briefly after reboot
4. **Power issue** — unstable power causing intermittent radio operation

### Suggestion

- **Check ESP32 serial output RIGHT NOW** — is it still printing status messages? If the serial output stopped, it crashed.
- **Add `setScanResponse(true)` back** — but without the interval settings that broke it in Tests 6-8. The scan response is needed to reliably include the service UUID.
- **Consider matching by name AND MAC as fallback** — don't rely solely on service UUID in advertisement data

---

## Previous Tests Summary

| Test | Time | Result |
|------|------|--------|
| 11 | 00:40 | Not found (unfiltered, 5 cycles + 2 extra scans) |
| 10 | 00:34 | Filtered: not found. Unfiltered: FOUND. |
| 9 | 00:29 | Visible (3/5 scans), connection timeout |
| 8 | 00:18 | Not visible (broken adv config) |
| 7 | 00:10 | Not visible (broken adv config) |
| 6 | 00:00 | Not visible (broken adv config) |
| 5 | 23:50 | Found, bluetoothctl remove broke connect |
| 3 | 23:18 | Found (3 hits), br-connection-canceled |
| 2 | 23:12 | MAC found as "EasyPlay" |
| 1 | 23:00 | Fixed bleak .rssi bug |
