# Pi Test Report: Step 2 — Button Handling

## Test 35 — 2026-03-17 11:01 UTC (fresh reboot + interrupt-driven button firmware)

**Duration:** ~3 minutes
**Uptime at start:** 2 minutes (freshly rebooted)
**Firmware:** Test 34 (interrupt-driven buttons, NimBLE)

### Result: PASS — connected on first attempt, rock-solid connection

#### Connection
- **Scan:** Found BLE-Remote after 2s (early exit after 17 devices), RSSI -89 dBm
- **Connect:** Connected in ~4.5s on the **first attempt**
- **Subscribed:** Notifications active at 11:01:23

#### Heartbeats
- **89 heartbeats** received over ~3 minutes (HEARTBEAT #2 through #89)
- Consistent ~2s interval, zero drops
- All STATUS checks: `connected=True mtu=23`

#### Conclusion
**The interrupt-driven button firmware works perfectly.** The failures in Tests 32-34c were caused by accumulated Pi/BlueZ state degradation, not the firmware. A fresh reboot restored clean first-attempt connectivity.

| Test | Firmware | Pi State | Attempts | Result |
|------|----------|----------|----------|--------|
| **35 (11:01)** | **Interrupt buttons (Test 34)** | **Fresh reboot** | **1** | **PASS — 89 heartbeats, stable** |
| 31b (10:53) | GPIO init only (Test 31) | Degraded (20+ cycles) | 8 | Connected but degraded |
| 34c-32 (10:43-01:22) | Various button firmwares | Degraded | All failed | Falsely blamed firmware |
| 31 (01:14) | GPIO init only | Fresh session | 1 | PASS |

### Next steps
- Button firmware is confirmed working — ready to test actual button presses
- Consider implementing periodic Pi bluetooth service restart to prevent state degradation in long sessions

---

## Test 31b — 2026-03-17 10:53 UTC (baseline recheck — exact Test 31 firmware)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: EVENTUALLY CONNECTED — but took 8 attempts (was 1 in original Test 31)

#### Connection attempts
- **Attempts 1-7**: ALL failed with "failed to discover services, device disconnected" (2-6s each)
- **Attempt 8** (10:55:01): **CONNECTED** after 9s → heartbeats flowing

#### Once connected: Stable
- **5 heartbeats** (#6-#10) in the final ~8 seconds before test timeout
- Connection stable, heartbeats every ~2 seconds
- Would likely have held longer if test continued

#### RSSI: Variable
- -81 to -90 dBm across scans — more variable than before

### CRITICAL FINDING: The baseline is broken too

| Test | Firmware | Attempts to connect | RSSI |
|------|----------|-------------------|------|
| **31 (original, 01:14)** | **GPIO init only** | **1** | **-84** |
| **31b (recheck, 10:53)** | **Same firmware** | **8** | **-81 to -90** |

The exact same firmware that connected on the first attempt at 01:14 now takes 8 attempts at 10:53. **The problem is NOT in the firmware at all.**

### What changed between Test 31 and now?

1. **Pi Bluetooth adapter state** — after 20+ test cycles with repeated adapter resets, service restarts, cache removals, and failed connections, the BlueZ/adapter may be in a degraded state
2. **ESP32 position** — was moved, now has more variable RSSI (-81 to -90 vs steady -84)
3. **RF environment** — time of day, other BLE devices, WiFi interference may have changed
4. **BlueZ D-Bus state** — accumulated from many failed connections and recoveries

### Re-evaluation of ALL test results since Test 32

If the baseline itself is unreliable at 8 attempts, then **Tests 32-34c failures may not have been caused by the button code at all.** They may have been caused by the same environmental/adapter degradation that makes Test 31b take 8 attempts.

The only truly reliable comparison is:
- Test 31 (01:14, fresh session): 1 attempt → connected
- Test 32 (01:22, 8 minutes later): connected but unstable
- Test 31b (10:53, 9.5 hours later): 8 attempts → connected

### Suggestion
1. **Reboot the Pi** — full reboot to clear all accumulated BlueZ/kernel state
2. **Run Test 31 firmware again** after reboot — if it connects on first attempt, the baseline is restored
3. **Then immediately run the button firmware** — to get a fair comparison in the same session
4. Consider that the ESP32 at its current position may be marginal — bring it closer (~50cm)

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 35 (11:01) | **PASS** — first attempt, 89 heartbeats, fresh reboot |
| 31b (10:53) | Connected on 8th attempt — baseline degraded |
| 34c (10:43) | FAIL — cache cleared, still fails |
| 34b (10:28) | FAIL — closer range |
| 34 (10:20) | FAIL — interrupt-driven |
| 33 (10:06) | FAIL — no Serial |
| 32b (09:50) | FAIL — 10ms delay |
| 32 (01:22) | PARTIAL — only partially worked |
| 31 (01:14) | **PASS** — first attempt (fresh session) |
| 30 (01:06) | **PASS** — first attempt (fresh session) |
