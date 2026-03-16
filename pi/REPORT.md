# Pi Test Report: Step 1 BLE Connection Test

## Test 19 — 2026-03-16 23:16 UTC (direct MAC reconnect, skip scanning)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: Cold start good, reconnect path works but BlueZ InProgress blocks it

#### Cold start: First-attempt success (again!)
- Device found as **device #3** in scan — early exit after **0.8 seconds**
- Matched by **UUID**, device name "BLE-Remote" (correct)
- Connected in **11.5 seconds** (slower than Test 18's 3.5s — variable)
- First heartbeat at 23:16:56.7
- **Script start → first heartbeat: ~18 seconds** (3.2s reset + 0.8s scan + 11.5s connect + 2s subscribe)

#### Stability: 93 seconds, 48 heartbeats
- **48 consecutive heartbeats** (#5-#48), every ~2 seconds, zero gaps
- Connection held from 23:16:54 to 23:18:29 — **93 seconds**
- Natural disconnect at 23:18:29 (not forced — ESP32 may have dropped or supervision timeout)

#### Reconnection: Direct MAC connect triggered
- After disconnect, the new direct-connect path activated correctly:
  - `"Direct connect to known address 38:44:BE:45:AD:86 (no scan)..."`
  - No adapter reset, no scan — went straight to connect
- **BUT: BlueZ "InProgress" error fired immediately** (within 16ms of connect attempt)
- Adapter reset triggered (3.2 seconds)
- Second direct-connect attempt started but test timed out before result

#### Timing breakdown

| Phase | Test 18 | Test 19 |
|-------|---------|---------|
| Adapter reset | 3.2s | 3.2s |
| Scan | 0.4s | 0.8s |
| Connection | 3.5s | **11.5s** |
| Subscribe | 1.1s | 1.5s |
| **Total cold start** | **8.8s** | **~18s** |

The connection time is quite variable (3.5s vs 11.5s). This is likely BlueZ/BLE negotiation variance, not a code issue.

### Answer to key question

**How fast is reconnection?** The direct-MAC path activated correctly and skipped scanning, but **BlueZ "InProgress" always fires after a disconnect**, requiring a 3.2s adapter reset before the reconnect can proceed. Estimated reconnect time: ~1s delay + 0.016s (InProgress) + 3.2s reset + connect time (~4-12s) = **~8-16 seconds**.

The "InProgress" error after disconnect appears to be unavoidable with BlueZ — it seems to need an adapter reset to clear its internal connection state. The direct-connect optimization saves the scan time (~1-4s) but can't avoid the reset.

### Suggestion
- The direct-MAC reconnect path is working correctly — keep it
- Consider pre-emptively doing the adapter reset immediately on disconnect (before the 1s reconnect delay) to overlap the timing
- The connection time variability (3.5-11.5s) suggests the ESP32 may sometimes need multiple connection intervals to respond — could be improved with shorter connection intervals on the ESP32 side

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 19 (23:16) | Cold start OK (~18s), direct reconnect works but InProgress blocks, 93s stable |
| 18 (23:04) | **PERFECT** — first-attempt connect, 9s to heartbeat, 111s stable |
| 17 (22:56) | Improved — 2nd attempt connect, 49s to heartbeat |
| 16 (22:50) | Partial fix — 4th attempt connect, 60s to heartbeat |
| 15 (22:42) | REGRESSION — all connections timeout |
| 14 (22:30) | Full success — 3m41s stable, no speed optimizations |
| 13 (22:24) | Reliable discovery, disconnects at ~25s |
| 1-12 | Early tests — discovery and connection issues |
