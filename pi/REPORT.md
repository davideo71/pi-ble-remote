# Pi Test Report: Step 1 BLE Connection Test

## Test 18 — 2026-03-16 23:04 UTC (removed bluetoothctl remove from normal scan flow)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: PERFECT — First attempt connection, 9 seconds to first heartbeat

#### Discovery
- Device found as **device #2** in scan — early exit after **0.4 seconds**
- Matched by **UUID** (service UUID back in advertisement data!)
- Device name now shows as **"BLE-Remote"** (correct name, not "EasyPlay")
- RSSI: -91 dBm
- All 3 service UUIDs visible in advertisement: GAP, GATT, and custom service

#### Connection: FIRST ATTEMPT SUCCESS
- Scan complete at 23:04:24.4
- Connected at 23:04:27.9 — **3.5 seconds** to connect
- Subscribed to notifications at 23:04:29.0
- **First heartbeat at 23:04:29.6**

#### Timing: Script start to first heartbeat
- Script start: 23:04:20.8
- Adapter reset: 3.2 seconds
- Scan: 0.4 seconds (early exit)
- Connection: 3.5 seconds
- Subscription: 1.1 seconds
- **Total: 8.8 seconds from script start to first heartbeat**

#### Stability
- **56 consecutive heartbeats** (#2-#57), every ~2 seconds, **zero gaps**
- Connection held for **111 seconds** — still alive when test timed out
- No disconnects, no errors, no adapter resets needed after initial

### The fix: removing `bluetoothctl remove` was the key

| Metric | Test 16 | Test 17 | Test 18 |
|--------|---------|---------|---------|
| Cache removal | Every scan | First only | **None (recovery only)** |
| Attempts to connect | 4 | 2 | **1** |
| Time to first heartbeat | ~60s | ~49s | **~9s** |
| UUID in advertisements | No | No | **Yes** |
| Device name correct | No (EasyPlay) | No (EasyPlay) | **Yes (BLE-Remote)** |
| Heartbeats received | 32 | 36 | **56** |

Key insight: `bluetoothctl remove` was destroying BlueZ's cached knowledge of the device, including its correct name and service UUID data. Without the remove, BlueZ retains the device info from prior connections, enabling faster and more reliable connections.

### Answer to key question

**Does the FIRST connection attempt succeed?** YES! Connected on the very first attempt in 3.5 seconds. The `bluetoothctl remove` was the root cause of the multi-attempt failures in Tests 15-17.

### Step 1 status: COMPLETE (for real this time)

The optimized connection flow now achieves:
- **~9 seconds** from cold start to receiving data
- **First-attempt** connection success
- **Stable** connections (111+ seconds, no drops)
- **Fast discovery** via early exit scan (<1 second)
- **Recovery path** available (bluetoothctl remove after 3+ failures)

**Ready for Step 2: GPIO button handling.**

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 18 (23:04) | **PERFECT** — first-attempt connect, 9s to heartbeat, 56 heartbeats, 111s stable |
| 17 (22:56) | Improved — 2nd attempt connect, 49s to heartbeat |
| 16 (22:50) | Partial fix — 4th attempt connect, 60s to heartbeat |
| 15 (22:42) | REGRESSION — all connections timeout |
| 14 (22:30) | Full success — 3m41s stable, but no speed optimizations |
| 13 (22:24) | Reliable discovery, disconnects at ~25s |
| 12 (00:50) | Not found — ESP32 stopped advertising |
| 9-11 | Intermittent discovery, connection issues |
| 6-8 | Not found — broken adv config |
| 1-5 | Early tests, bleak bug fixed, connection issues |
