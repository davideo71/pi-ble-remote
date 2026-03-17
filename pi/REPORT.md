# Pi Test Report: Step 2 — Button Handling

## Test 36 — 2026-03-17 11:47 UTC (fresh reboot + BT restart + improved recovery)

**Duration:** ~2 minutes (timeout)
**Uptime at start:** 2 minutes (freshly rebooted)
**Firmware:** Same as 35b/35c — device advertises as "EasyPlay" (not "BLE-Remote")

### Result: FAIL — device found but all connections time out

#### Bug fix applied
`ble_receiver.py` had an `UnboundLocalError` on `connected_address` at line 336. The variable was never initialized in `main()` and the `global` declaration was missing. Fixed by adding `global connected_address` and initializing to `None`.

#### Connection attempts
- **Attempt 1:** Found "EasyPlay" at -90 dBm after scanning 3 devices. Disconnected at 5s, timed out at 15s.
- **Attempt 2:** Found "EasyPlay" at -86 dBm after 9 devices. Timed out at 15s.
- **Attempt 3:** Scan timeout — 37 devices seen, target not found. Recovery triggered (BT service restart).
- **Attempt 4:** Found "EasyPlay" at -90 dBm after 14 devices. Timed out at 15s.
- **Attempt 5:** Found "EasyPlay" at -90 dBm after 7 devices. Timed out at 15s (test ended at timeout).

#### RSSI
- **-86 to -90 dBm** — weak but consistently found during scans

#### Key observations
1. **Pi was freshly rebooted** — clean BlueZ state, so degradation is not the issue this time
2. **Device name is "EasyPlay"** not "BLE-Remote" — still matched via KNOWN_MAC fallback
3. **UUIDs=[]** — device not advertising the expected service UUID
4. **Scanning works, connecting doesn't** — same pattern as Test 35c
5. RSSI is comparable to Test 35 (-89 dBm, which connected) but connection never establishes

#### RSSI comparison
| Test | Pi State | RSSI | Result |
|------|----------|------|--------|
| Test 35 (close) | Fresh reboot | -89 dBm | PASS — first attempt, 89 heartbeats |
| Test 35b (far) | Same session | not seen | FAIL — out of range |
| Test 35c (far, no WiFi) | Same session | -88 to -91 dBm | FAIL — found but can't connect |
| **Test 36** | **Fresh reboot** | **-86 to -90 dBm** | **FAIL — found but can't connect** |

#### Conclusion
The ESP32 is visible in scans (-86 to -90 dBm) but connection never establishes. Since the Pi was freshly rebooted, BlueZ state degradation is ruled out. The issue is likely:
1. **ESP32 firmware changed** — now advertising as "EasyPlay" with no matching service UUID, which means GATT service discovery will fail even if the L2CAP connection succeeds
2. **Signal is marginal** — -90 dBm is borderline for reliable GATT connections
3. The ESP32 may need to be **re-flashed with the correct firmware** (BLE-Remote with the expected service UUID)

---

## Test 35c — 2026-03-17 11:34 UTC (range test retry — WiFi disabled)

**Duration:** ~5 minutes
**Firmware:** New firmware — device now advertises as "EasyPlay" (was "BLE-Remote")
**Pi state:** Same session, WiFi disabled via rfkill, onboard BT already disabled. USB dongle (CSR8510) only.

### Result: FAIL — device found but every connection times out

#### Details
- **7 connection attempts**, all timed out after 15s
- Device found by MAC address match (name changed to "EasyPlay", receiver expects "BLE-Remote")
- RSSI: **-88 to -91 dBm** — right at the noise floor
- Disabling WiFi made no difference vs Test 35b

#### Key observations
1. **Device name changed** — firmware now advertises as "EasyPlay" instead of "BLE-Remote". Receiver matches by MAC fallback but UUID match also fails (UUIDs=[] on most scans)
2. **Scanning works, connecting doesn't** — at -91 dBm the signal is strong enough for discovery but too weak for the multi-packet GATT connection handshake
3. **WiFi interference is not the issue** — same failure pattern with WiFi off

#### RSSI comparison
| Test | RSSI | Result |
|------|------|--------|
| Test 35 (close) | -89 dBm | PASS — first attempt, 89 heartbeats |
| Test 35b (far) | not seen | FAIL — completely out of range |
| Test 35c (far, no WiFi) | -88 to -91 dBm | FAIL — found but can't connect |

#### Conclusion
ESP32 is too far away. Needs to be moved closer. The -89 dBm from Test 35 was already marginal but worked; -91 dBm does not. Also: firmware needs to either keep advertising as "BLE-Remote" or `ble_receiver.py` needs updating to match "EasyPlay".

---

## Test 35b — 2026-03-17 11:17 UTC (range test — ESP32 moved further away)

**Duration:** ~4 minutes (15 scan attempts)
**Firmware:** Same as Test 35 (interrupt-driven buttons)
**Pi state:** Same session as Test 35 (uptime ~18 min)

### Result: FAIL — device never found

#### Details
- **15 scan attempts** over ~4 minutes, BLE-Remote never appeared in any scan
- Other BLE devices visible (S41 152A LE at -78 to -91 dBm, various unknowns at -78 to -90 dBm)
- ESP32 address 38:44:BE:45:AD:86 never seen — not even weak signal
- Recovery cache clears triggered 3 times, no help

#### RSSI comparison
| Test | RSSI | Result |
|------|------|--------|
| Test 35 (worked) | -89 dBm | PASS — first attempt |
| Test 35b (failed) | not seen | FAIL — completely out of range |

#### Conclusion
The ESP32 is too far away at its new position. At -89 dBm (Test 35) we were already near the limit. The new location is beyond BLE range. **Move the ESP32 closer** — it needs to be within ~5-8 meters with line of sight for reliable operation.

---

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
