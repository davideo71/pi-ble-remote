# Pi Test Report: Step 1 BLE Connection Test

## Test 13 — 2026-03-16 22:24 UTC (reduced TX power + fast advertising + adapter reset)

**Duration:** ~120 seconds (3 connection cycles)

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: SUCCESS — Discovery reliable, connection works, heartbeats received!

#### Discovery
- ESP32 found **immediately** by UUID — **16 matches** during a single 10-second scan
- RSSI: **-87 to -91 dBm** (slightly weaker than -81 to -88 at +20 dBm, but far more consistent)
- Active scanning worked — service UUID appeared in every advertisement
- Adapter reset before scan added ~3 seconds delay (acceptable)
- Device name still shows as "EasyPlay" (BlueZ cache), not "BLE-Remote"

#### Connection 1 (22:24:40 - 22:25:06)
- Connected in ~2.3 seconds
- **Received heartbeats #2 through #13** (~2 second intervals, as expected)
- MTU: 23 (default)
- Custom service `4e520001-...` discovered with notify characteristic
- **Disconnected after ~26 seconds** (unexpected)

#### Reconnection 1 → Connection 2 (22:25:22 - 22:25:50)
- BlueZ threw "InProgress" error after disconnect — adapter reset recovered it
- Reconnected via cached MAC address (fast path, ~2 seconds)
- Connected and subscribed successfully
- **No heartbeat notifications received** despite being connected ~27 seconds
- Status checks showed connection was alive (is_connected=True)
- **Disconnected after ~27 seconds**

#### Reconnection 2 → Connection 3 (22:26:07 - timeout at 22:26:24)
- Same InProgress → adapter reset → reconnect pattern
- **Received heartbeats #2 through #9** (counter reset on each new connection)
- Connection still alive when test timed out at 120 seconds

### Key findings

| Metric | Test 9 (best before) | Test 13 |
|--------|---------------------|---------|
| Discovery | 3/5 scans | 16/16 matches in 1 scan |
| RSSI | -81 to -88 dBm | -87 to -91 dBm |
| Connection | Timeout (failed) | Success (3 connects) |
| Heartbeats | None | Received (2x sessions) |
| Auto-reconnect | N/A | Works (with adapter reset) |

### Issues to investigate

1. **Connections drop after ~25-27 seconds** — all three connections terminated at roughly the same interval. This could be:
   - ESP32 supervision timeout set too low
   - NimBLE connection parameter mismatch
   - BlueZ connection parameter negotiation failure

2. **Connection 2 had no heartbeat notifications** — subscribed successfully but no data arrived. The ESP32 heartbeat counter likely reset and the notification may not have been re-enabled on the ESP32 side.

3. **BlueZ "InProgress" after every disconnect** — consistent pattern. The adapter reset workaround is reliable but adds ~6 seconds to reconnection.

4. **Device name "EasyPlay"** — BlueZ is caching the old name. The ESP32 firmware should set the name to "BLE-Remote" but BlueZ ignores the update. Low priority.

### Answer to key question

**Does the ESP32 show up consistently?** YES — the combination of lower TX power (+9 dBm), fast advertising (20-60ms), and adapter reset before scan made discovery **completely reliable**. 16 UUID matches in a single 10-second scan is excellent.

### Suggested next steps

1. **Fix the ~25s disconnect** — check ESP32 `NimBLEServer::onDisconnect()` logs and connection supervision timeout setting. Consider setting `BLE_GAP_INITIAL_SUPERVISION_TIMEOUT` higher.
2. **Ensure notifications survive reconnect** — the heartbeat task on ESP32 may need to re-check notification subscription state after a new connection.
3. **Consider longer connection intervals** on the ESP32 to reduce radio contention (currently likely using NimBLE defaults).

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 13 (22:24) | **SUCCESS** — reliable discovery (16 hits), 3 connections, heartbeats received, disconnects at ~25s |
| 12 (00:50) | Not found — ESP32 stopped advertising |
| 11 (00:40) | Not found — intermittent |
| 10 (00:34) | Found unfiltered, filter was unreliable |
| 9 (00:29) | Found 3/5 scans, connection timeout |
| 6-8 | Not found — broken adv config |
| 3-5 | Found — connection issues (BR/EDR, cache, timeout) |
| 1-2 | Initial tests, bleak bug fixed |
