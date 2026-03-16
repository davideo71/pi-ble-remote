# Pi Test Report: Step 1 BLE Connection Test

## Test 22 — 2026-03-16 23:37 UTC (light adapter reset: disconnect + HCI reset)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: SUCCESS — Light reset works, 12.6s to heartbeat

#### Cold start timing
- Script start: 23:37:23.4
- Light reset (disconnect + HCI reset): **3.8 seconds** (logged as ~1.3s but actual elapsed was 3.8s including subprocess overhead)
- Direct connect by MAC: **5.5 seconds**
- Subscribe: **2.3 seconds**
- **First heartbeat: 23:37:36.0**
- **Total: 12.6 seconds from script start to first heartbeat**

#### Light reset: works without errors
- `bluetoothctl disconnect` + `hciconfig hci0 reset` succeeded (no permission errors)
- No fallback to full power cycle needed
- No "InProgress" errors
- No "No powered adapters" errors

#### Stability: 107 seconds, 54 heartbeats
- **54 consecutive heartbeats** (#3-#56), every ~2 seconds, zero gaps
- Connection held for **107+ seconds** — still alive at test timeout
- No disconnects

### Timing comparison

| Test | Reset method | Reset time | Connect time | Total to heartbeat |
|------|-------------|------------|--------------|-------------------|
| 14 | Full power cycle (3s) + scan (10s) | ~13s | 5s | ~20s |
| 18 | Full power cycle (3s) + scan (early exit) | ~3.6s | 3.5s | **9s** |
| 21 | Full power cycle (4s) + direct MAC | ~4.2s | 7.3s | 13s |
| **22** | **Light reset (disconnect + HCI)** + direct MAC | **~3.8s** | **5.5s** | **12.6s** |

The light reset saved ~0.4s vs the full power cycle from Test 21. The connect time varies between runs (3.5-11.5s across tests), so the total time is dominated by BLE connection variance rather than reset strategy.

### Answer to key question

**Does the light reset clear InProgress errors?** Yes — no InProgress errors occurred. However, we didn't test reconnection after disconnect (the connection never dropped). The real test of InProgress handling will be when a disconnect happens mid-session.

**How much faster is startup?** ~0.4s faster than full power cycle. The improvement is modest because: (1) the HCI reset still takes time, (2) the connect time varies more than the reset savings. The main benefit is reliability — avoiding the "No powered adapters" risk of the full power cycle.

### Note on `hciconfig`
`hciconfig hci0 reset` ran without needing sudo. This may be because the user has permissions via the `bluetooth` group, or because the Pi's default config allows it. On other systems, this might require elevated privileges.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 22 (23:37) | **SUCCESS** — light reset, 12.6s to heartbeat, 54 heartbeats, 107s stable |
| 21 (23:27) | Success — direct connect, 13s to heartbeat, 105s stable |
| 20 (23:22) | Rough — 6 attempts, adapter errors |
| 19 (23:16) | Direct reconnect works but InProgress blocks |
| 18 (23:04) | Perfect scan+connect, 9s to heartbeat |
| 15-17 | Optimization attempts with regressions |
| 14 (22:30) | First stable connection (3m41s) |
| 13 (22:24) | First reliable discovery |
| 1-12 | Early tests — discovery and connection issues |
