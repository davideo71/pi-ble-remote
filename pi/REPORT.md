# Pi Test Report: Step 1 BLE Connection Test

## Test 17 — 2026-03-16 22:56 UTC (15s timeout + first-scan-only cache removal)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: IMPROVED — Connected on 2nd attempt, stable for 70 seconds

#### Discovery
- Early exit scan still fast — device found in <1 second (attempt 1) and ~4 seconds (attempt 2)
- Matched by **MAC only** again (UUIDs=[] in advertisement data)
- RSSI: -87 to -91 dBm

#### Connection attempts
- **Attempt 1** (22:56:47): "No powered adapters" error — adapter reset timing issue on first boot
- **Attempt 2** (22:56:47 retry): Found instantly, DISCONNECTED after 6.6s during connect, timed out at 15s
- **Attempt 3** (22:57:11): Found in 4s, **CONNECTED** after 8.5s — SUCCESS

#### Once connected (22:57:20 — 22:58:33, test timeout)
- **36 consecutive heartbeats** (#4-#39), every ~2 seconds, zero gaps
- Stable for **73 seconds** — connection still alive when test ended
- MTU: 23

#### Timing
- Script start → first heartbeat: **~49 seconds** (1 adapter error + 1 failed connect + successful connect)
- On the successful attempt: scan-to-heartbeat = **~11 seconds** (4s scan + 8.5s connect + notification setup)

### Progress across Tests 15-17

| Metric | Test 15 | Test 16 | Test 17 |
|--------|---------|---------|---------|
| Adapter reset | 1.5s (broken) | 3s (works) | 3s (works) |
| Connect timeout | 10s | 10s | **15s** |
| Cache removal | Every scan | Every scan | **First only** |
| Attempts to connect | ∞ (never) | 4 | **2** (1 error + 1 fail) |
| Time to first heartbeat | N/A | ~60s | **~49s** |
| Stability once connected | N/A | 60s stable | **73s stable** |

### Answer to key question

**Does the first connection attempt succeed?** Not yet — the first real attempt still fails (DISCONNECTED during connect handshake). But it's improving: Test 16 needed 4 attempts, Test 17 needed 2.

The remaining issue is that the first connection after a fresh `bluetoothctl remove` + adapter reset seems to fail consistently. The second attempt (without remove) succeeds. This suggests:
1. **The `bluetoothctl remove` is the problem** — it forces BlueZ to re-discover the device from scratch, and the first connection after fresh discovery often fails
2. **Possible fix**: skip `bluetoothctl remove` entirely on startup — only use it as a recovery step if connections fail multiple times

### Suggestion
Try removing the `bluetoothctl remove` from the initial scan path entirely. Only use it as a fallback after 2-3 failed connection attempts. The adapter reset alone should be sufficient to clear stale state.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 17 (22:56) | **IMPROVED** — connects on 2nd attempt (~49s), stable 73s. First attempt still fails. |
| 16 (22:50) | Partial fix — connects on 4th attempt (~60s), stable 60s |
| 15 (22:42) | REGRESSION — fast discovery but all connections timeout |
| 14 (22:30) | **FULL SUCCESS** — stable 3m41s connection, 110 heartbeats, zero drops |
| 13 (22:24) | Reliable discovery, 3 connections, disconnects at ~25s |
| 12 (00:50) | Not found — ESP32 stopped advertising |
| 9-11 | Intermittent discovery, connection issues |
| 6-8 | Not found — broken adv config |
| 1-5 | Early tests, bleak bug fixed, connection issues |
