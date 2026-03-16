# Pi Test Report: Step 1 BLE Connection Test

## Test 20 — 2026-03-16 23:22 UTC (pre-emptive adapter reset on reconnect)

**Duration:** ~150 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: ROUGH RUN — Connected after 6 attempts, direct reconnect NOT tested

#### Connection attempts (cold start)
1. **(23:22:26)** Found in 0.3s by UUID. Disconnected during service discovery after 13s. Error: "failed to discover services, device disconnected"
2. **(23:22:44)** Found in 0.9s by UUID. Connection timed out after 19s.
3. **(23:23:09)** "No powered Bluetooth adapters" — adapter reset timing issue.
4. **(23:23:14)** Recovery: `bluetoothctl remove` triggered after 3 failures. Found in 5.4s. Connection timed out after 19s.
5. **(23:23:44)** "No powered adapters" again.
6. **(23:23:49)** Found in 0.4s. **CONNECTED** after 9.8s — SUCCESS.

**Script start → first heartbeat: ~100 seconds** (5 failed attempts)

#### Once connected (23:23:59 — 23:24:52+, test timeout)
- **25 heartbeats** (#5-#29), every ~2 seconds, zero gaps
- Stable for 53+ seconds — still alive at test timeout

#### Direct-MAC reconnect: NOT TESTED
The direct reconnect path (`connect_and_listen_by_address()`) was never triggered because:
- The initial connection failed before `connected_address` was set
- So the code fell back to the scan-based path on every retry
- The connection finally succeeded but didn't disconnect again during the test

#### "No powered adapters" is back
The 3s adapter reset (1s off + 2s on) produced "No powered adapters" errors **twice** in this run (attempts 3 and 5). This didn't happen in Test 18 but has appeared intermittently. The timing may be marginal — 2s after power-on might not always be enough for BlueZ to fully initialize.

### Analysis: Connection reliability has degraded

| Test | Cold start attempts | Time to heartbeat |
|------|-------------------|-------------------|
| 18 | **1** | **9s** |
| 19 | **1** | 18s |
| 20 | **6** | **100s** |

The code is essentially the same across these tests. The main variable is BLE connection reliability, which appears to fluctuate significantly. Possible factors:
- **BlueZ internal state** accumulates across runs (not fully cleared by adapter reset)
- **RF environment** changes (interference from other 2.4GHz devices)
- **ESP32 advertising state** — the periodic re-advertising may have edge cases
- **Connection timing** — BLE connection is inherently probabilistic

### Suggestions
1. **Increase adapter reset wait to 3s** (1s off + **3s** on) to eliminate "No powered adapters" errors
2. **Set `connected_address` from KNOWN_MAC at startup** so the direct-connect path is available even before the first successful connection
3. Consider that connection reliability may never be 100% on first attempt — the retry loop is the right strategy, focus on making retries fast

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 20 (23:22) | Rough run — 6 attempts to connect, direct reconnect not tested, adapter errors |
| 19 (23:16) | Cold start OK (~18s), direct reconnect works but InProgress blocks, 93s stable |
| 18 (23:04) | **PERFECT** — first-attempt connect, 9s to heartbeat, 111s stable |
| 17 (22:56) | Improved — 2nd attempt connect, 49s to heartbeat |
| 16 (22:50) | Partial fix — 4th attempt connect, 60s to heartbeat |
| 15 (22:42) | REGRESSION — all connections timeout |
| 14 (22:30) | Full success — 3m41s stable, no speed optimizations |
| 13 (22:24) | Reliable discovery, disconnects at ~25s |
| 1-12 | Early tests — discovery and connection issues |
