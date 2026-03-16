# Pi Test Report: Step 1 BLE Connection Test

## Test 16 — 2026-03-16 22:50 UTC (revert adapter reset to 3s — fix regression)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: PARTIAL FIX — Connection works but takes multiple attempts

#### Discovery: Fast (early exit scan working well)
- Device found in **<2 seconds** on every scan (early exit triggered)
- Matched by **MAC** on all scans (not UUID — see note below)
- RSSI: -87 to -91 dBm (consistent)
- No "No powered adapters" errors — the 3s reset timing is sufficient

#### Connection: Succeeded on 4th attempt
- **Attempt 1** (22:50:26): Found in 1s, DISCONNECTED after 0.7s, timed out
- **Attempt 2** (22:50:41): Found in 1s, DISCONNECTED after 2s, timed out
- **Attempt 3** (22:50:57): Found in 1.3s, DISCONNECTED after 9s, timed out
- **Attempt 4** (22:51:13): Found in 1.6s, **CONNECTED** after 6s — SUCCESS
- Once connected: **32 consecutive heartbeats** (#3-#32), stable for 60s until test timeout

#### Timing
- Script start → first heartbeat: **~60 seconds** (due to 3 failed connection attempts)
- Scan-to-connect on successful attempt: ~8 seconds (1.6s scan + 6s connect)
- If all attempts succeeded on first try, estimated time would be: ~7-8 seconds

#### Important observation: MAC-only matches
All scans matched by **MAC address only** — the service UUID was **not** in the advertisement data (`UUIDs=[]`). In Tests 13-14, UUID matching was the primary method. This suggests:
- The ESP32 may not be including the UUID in its regular advertisements anymore
- Or active scanning is not triggering scan responses consistently
- MAC fallback matching (added in Test 12) is saving us here

### Answers to key questions

1. **Does the connection succeed now?** YES — the 3s reset timing fixes the "no adapter" issue from Test 15. But connections still fail ~75% of the time before succeeding.

2. **How long from script start to first heartbeat?** ~60 seconds (3 failed attempts before success). Would be ~8 seconds if first attempt succeeded.

3. **Connection stability once connected?** Excellent — 32 heartbeats over 60 seconds with no drops, just like Test 14.

### Remaining issue: Why do connections fail multiple times?

The DISCONNECTED callback fires almost immediately (0.7-9 seconds into connection), then the connect call times out. This didn't happen in Test 14. Differences from Test 14:
- `bluetoothctl remove` now happens before every full scan (Test 14 only did it on first scan)
- Connection timeout is 10s (Test 14 was 15s)
- The scan finds by MAC without UUID — maybe connecting to a device discovered without service UUID in the advertisement causes BlueZ to attempt BR/EDR first?

### Suggestion
- Try increasing `CONNECT_TIMEOUT` back to 15s
- Consider only running `bluetoothctl remove` on the very first scan, not on retries
- Investigate why UUID is no longer in advertisements (check ESP32 scan response config)

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 16 (22:50) | **PARTIAL FIX** — connects on 4th attempt, stable once connected. MAC-only matches. |
| 15 (22:42) | REGRESSION — fast discovery but all connections timeout. Adapter reset too fast. |
| 14 (22:30) | **FULL SUCCESS** — stable 3m41s connection, 110 heartbeats, zero drops |
| 13 (22:24) | Reliable discovery (16 hits), 3 connections, heartbeats received, disconnects at ~25s |
| 12 (00:50) | Not found — ESP32 stopped advertising |
| 11 (00:40) | Not found — intermittent |
| 10 (00:34) | Found unfiltered, filter was unreliable |
| 9 (00:29) | Found 3/5 scans, connection timeout |
| 6-8 | Not found — broken adv config |
| 3-5 | Found — connection issues (BR/EDR, cache, timeout) |
| 1-2 | Initial tests, bleak bug fixed |
