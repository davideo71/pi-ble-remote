# Pi Test Report: Step 1 BLE Connection Test

## Test 15 — 2026-03-16 22:42 UTC (optimized connection and reconnection speed)

**Duration:** 2 runs, ~60 seconds each

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: REGRESSION — Discovery fast, but ALL connections timeout

#### What works: Discovery + early exit scan
- Early exit scan works perfectly — device found in **<1 second** on first match
- First scan in Run 1: found as **device #1** (instant!)
- Subsequent scans: found in 1-8 devices, typically under 2 seconds
- RSSI: steady -87 dBm (consistent with Test 13-14)
- UUID matching reliable, MAC fallback also works

#### What's broken: Connections timeout every time
- **Run 1:** 3 connection attempts, ALL timed out after 10-14 seconds
- **Run 2:** 3+ connection attempts, ALL timed out
- 0 successful connections across both runs
- Pattern: device found → connect attempt → ~10s timeout → DISCONNECTED callback fires → repeat

This is a **regression from Test 14** where connections worked perfectly (3m41s stable).

#### Additional issue: Adapter reset too fast
- The 1.5s adapter reset (0.5s off + 1.0s on) is not always enough
- Got "No powered Bluetooth adapters found" errors when scanning immediately after reset
- The old 3s timing (1s off + 2s on) was more reliable

### Analysis

The ESP32 firmware is **unchanged** from Test 14. Only Pi-side code changed. Possible causes:

1. **Faster adapter reset (1.5s) may leave BlueZ in a half-initialized state** — the adapter reports as powered but the LE subsystem isn't fully ready, causing connection to fail silently.

2. **`bluetoothctl remove` before every scan may be too aggressive** — in Test 14, the remove only happened on first scan (no cached address). Now it happens every time the full scan path runs, which is after every failed connection.

3. **The quick scan path (3s address-based, no adapter reset) never succeeded** — always fell through to full scan. Possibly because BlueZ needs the reset to see the device after a disconnect/timeout.

### Answers to key questions

1. **How long from script start to first heartbeat?** N/A — connections never succeeded.
2. **How long from restart to first heartbeat?** N/A — same issue.
3. **Does the connection stay stable?** N/A — couldn't connect.

### Suggestion

Revert the adapter reset timing to 3s (1s off + 2s on) — the 1.5s timing causes "no adapter" errors. Keep the early exit scan (it's a clear win for discovery speed). The connection timeout issue may also be related to insufficient reset time before the connect attempt.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 15 (22:42) | **REGRESSION** — fast discovery but all connections timeout. Adapter reset too fast. |
| 14 (22:30) | **FULL SUCCESS** — stable 3m41s connection, 110 heartbeats, zero drops |
| 13 (22:24) | Reliable discovery (16 hits), 3 connections, heartbeats received, disconnects at ~25s |
| 12 (00:50) | Not found — ESP32 stopped advertising |
| 11 (00:40) | Not found — intermittent |
| 10 (00:34) | Found unfiltered, filter was unreliable |
| 9 (00:29) | Found 3/5 scans, connection timeout |
| 6-8 | Not found — broken adv config |
| 3-5 | Found — connection issues (BR/EDR, cache, timeout) |
| 1-2 | Initial tests, bleak bug fixed |
