# Pi Test Report: Step 1 BLE Connection Test

## Test 14 — 2026-03-16 22:30 UTC (relaxed connection parameters — supervision timeout fix)

**Duration:** ~4 minutes (240 second timeout)

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FULL SUCCESS — Stable connection for 3m41s, 110 heartbeats, zero drops

#### Discovery
- ESP32 found by UUID — **6 matches** during 10-second scan
- RSSI: **steady -87 dBm** (very consistent, no variation)
- Active scanning + adapter reset working as expected

#### Connection (22:30:37 — 22:34:18, entire test duration)
- Connected in ~5 seconds
- **Single connection held for 3 minutes 41 seconds** — no disconnects
- **110 consecutive heartbeats** (#3 through #112), every ~2 seconds, **zero gaps**
- MTU: 23 (default)
- Status checks every 5 seconds all reported connected=True
- Connection was still alive when the 240-second timeout killed the test

### Answers to key questions

1. **Do connections survive past the 25-27 second mark?** YES — the connection ran for **3 min 41 sec** with no sign of dropping. The relaxed supervision timeout (6s vs default ~2s) completely fixed the disconnect issue.

2. **Do heartbeats arrive consistently on every connection?** YES — 110 consecutive heartbeats with no gaps. The "Connection 2 no heartbeats" issue from Test 13 did not recur.

3. **Does auto-reconnect still work?** Not tested — the connection never dropped, so auto-reconnect was not triggered. This is a good problem to have.

### Comparison across tests

| Metric | Test 9 | Test 13 | Test 14 |
|--------|--------|---------|---------|
| Discovery | 3/5 scans | 16 matches | 6 matches |
| RSSI | -81 to -88 | -87 to -91 | -87 steady |
| Connection | Timeout | 3x ~25s drops | **Stable 3m41s** |
| Heartbeats | None | 2/3 sessions | **110 consecutive** |
| Disconnects | N/A | 3 in 2 min | **0 in 4 min** |

### Step 1 status: COMPLETE

The BLE connection between ESP32-C3 and Raspberry Pi is now **reliable and stable**:
- Discovery is consistent (UUID-based, active scanning)
- Connection survives well beyond the initial 25-second issue
- Heartbeat notifications arrive every ~2 seconds without gaps
- The auto-reconnect infrastructure exists but wasn't needed

**Ready for Step 2:** GPIO button handling on the ESP32 + button-to-action mapping on the Pi.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 14 (22:30) | **FULL SUCCESS** — stable 3m41s connection, 110 heartbeats, zero drops |
| 13 (22:24) | Reliable discovery (16 hits), 3 connections, heartbeats received, disconnects at ~25s |
| 12 (00:50) | Not found — ESP32 stopped advertising |
| 11 (00:40) | Not found — intermittent |
| 10 (00:34) | Found unfiltered, filter was unreliable |
| 9 (00:29) | Found 3/5 scans, connection timeout |
| 6-8 | Not found — broken adv config |
| 3-5 | Found — connection issues (BR/EDR, cache, timeout) |
| 1-2 | Initial tests, bleak bug fixed |
