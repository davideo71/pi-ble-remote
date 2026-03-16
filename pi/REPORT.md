# Pi Test Report: Step 2 — Button Handling

## Test 29 — 2026-03-17 00:42 UTC (ESP32: 5s grace period before notifications)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — 5s grace period did NOT fix connection drops

#### Discovery: Working well
- ESP32 found by **UUID** on 4 out of 6 scans (consistently advertising)
- RSSI: -85 to -89 dBm
- Early exit scan working: device found in 2-13 devices, 2-5 seconds

#### Connection: Still failing every time
- **Attempt 1**: Found → connect → DISCONNECTED → timeout (17s)
- **Attempt 2**: Found → connect → DISCONNECTED → timeout (19s)
- **Attempt 3**: "No powered adapters" (adapter timing)
- **Attempt 4**: Not found in scan
- **Attempt 5**: Found → connect → DISCONNECTED → timeout (19s)
- **Attempt 6**: "No powered adapters"
- **Attempt 7**: Found → connect → (test timed out)

**Zero successful connections.** The DISCONNECTED callback fires during every connection attempt, same as Tests 25 and 28b.

### Analysis: The 5s grace period is NOT the fix

The grace period prevents notifications during GATT discovery, but the connection is dropping **before** GATT discovery even completes. The DISCONNECTED event fires 5-19 seconds into the connection attempt — before any service discovery or subscription happens.

This means the problem is at the **BLE connection level**, not the GATT/notification level:

1. **The ESP32 button firmware may have a connection parameter issue** — the button GPIO setup or ISR handlers may be interfering with BLE connection parameter negotiation
2. **ESP32 may be crashing/resetting during connection** — the button simulation loop + connection event handling may exceed stack/heap
3. **The ESP32 connection supervision timeout may have been affected** by the button firmware changes

### Strong evidence: This is the button firmware

| Tests 21-23 (pre-button firmware) | Tests 24-29 (button firmware) |
|-----------------------------------|-----------------------------|
| First-attempt connections | Zero successful connections |
| 100+ second stable sessions | Every connection drops in <20s |
| Heartbeats flowing | Never reaches subscription |

### Suggestion
**Revert to the exact ESP32 firmware from Test 22** (heartbeat only, no buttons) to confirm the hardware is still OK. Then add button support incrementally:
1. First: just GPIO init (no button reading)
2. Then: button reading without BLE notifications
3. Then: button BLE notifications

This will isolate which part of the button code breaks BLE.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 29 (00:42) | **FAILED** — 5s grace period didn't help, connections drop at BLE level |
| 28b (00:36) | FAILED — ESP32 found, connections drop during GATT |
| 28 (00:32) | FAILED — "Device not found" |
| 27-26 | FAILED — "Device not found" |
| 25 | FAILED — ESP32 drops connections |
| 24 | FAILED — InProgress errors |
| 23 | Last successful connection |
| 22-21 | Success — direct connect |
| 18 | Best result (9s) |
