# Pi Test Report: Step 1 BLE Connection Test

## Test 21 — 2026-03-16 23:27 UTC (pre-seeded MAC + 3s adapter on-wait)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: SUCCESS — First-attempt direct connect, 13s to heartbeat, no scan needed

#### Cold start: Direct connect by MAC (no scanning!)
- Script start: 23:27:57.5
- Adapter reset: 4.2 seconds (1s off + 3s on)
- Direct connect by MAC: 7.3 seconds
- Subscribe: 1.0 second
- **First heartbeat: 23:28:10.8**
- **Total: 13.3 seconds from script start to first heartbeat**

#### No errors
- **Zero "No powered adapters" errors** — the 3s on-wait is sufficient
- **Zero "InProgress" errors**
- **First-attempt connection success**
- No scanning was performed at all — went straight to direct connect

#### Stability: 105 seconds, 54 heartbeats
- **54 consecutive heartbeats** (#2-#55), every ~2 seconds, zero gaps
- Connection held for **105+ seconds** — still alive at test timeout
- No disconnects

### Answers to key questions

1. **Does pre-seeding MAC allow direct-connect on first startup?** YES — the script went straight from startup → adapter reset → connect by MAC. No scanning at all. The log shows `"Direct connect to known address ... (no scan)..."` as the very first action.

2. **Is 3s on-wait enough?** YES — zero adapter errors in this run. The previous 2s wait caused intermittent "No powered adapters" in Tests 15, 17, and 20.

### Comparison: Connection optimization journey

| Test | Strategy | Attempts | Time to heartbeat |
|------|----------|----------|-------------------|
| 14 | Scan (10s full) + connect | 1 | ~20s |
| 18 | Scan (early exit) + connect | 1 | **9s** |
| 19 | Scan (early exit) + connect | 1 | 18s |
| 20 | Scan + connect (unstable) | 6 | 100s |
| **21** | **Direct connect by MAC (no scan)** | **1** | **13s** |

The 13s time is: 4.2s (adapter reset) + 7.3s (connect) + 1s (subscribe) + 0.8s (first notification). The reset is fixed overhead. The connect time varies (3-12s across tests). Best-case would be ~8s.

### Step 1 optimized connection: COMPLETE

Final connection architecture:
- **Cold start**: adapter reset → direct connect by pre-seeded MAC → subscribe (~8-18s)
- **Reconnect after disconnect**: adapter reset → direct connect by MAC → subscribe (~8-18s)
- **Recovery after 3+ failures**: `bluetoothctl remove` → adapter reset → scan with early exit → connect

**Ready for Step 2: GPIO button handling.**

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 21 (23:27) | **SUCCESS** — direct connect, 13s to heartbeat, 54 heartbeats, 105s stable |
| 20 (23:22) | Rough — 6 attempts, adapter errors |
| 19 (23:16) | Direct reconnect works but InProgress blocks it |
| 18 (23:04) | Perfect scan+connect, 9s to heartbeat |
| 15-17 | Optimization attempts with regressions |
| 14 (22:30) | First stable connection (3m41s) |
| 13 (22:24) | First reliable discovery |
| 1-12 | Early tests — discovery and connection issues |
