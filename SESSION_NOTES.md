# Session Notes — March 17, 2026

## Quick Summary for Tomorrow's Claude

You're building a BLE remote (ESP32-C3 → Raspberry Pi). Tonight we isolated why button firmware broke BLE connections. **The root cause is the single-core ESP32-C3 being starved by tight polling loops.** Next step: re-test with 10ms loop delay, then add BLE button notifications.

---

## What Was Proven (Tests 30-32)

We did incremental isolation starting from a known-working heartbeat-only firmware:

| Test | What was added to firmware | Loop delay | BLE Result |
|------|--------------------------|-----------|------------|
| 30 | Heartbeat only (no GPIO) | 10ms | **PASS** — 37 heartbeats, 73s+ stable |
| 31 | + `pinMode(0-4, INPUT_PULLUP)` in setup | 10ms | **PASS** — 48 heartbeats, 78s+ stable |
| 32 | + `digitalRead` polling + debounce (serial log only, no BLE notify) | **5ms** | **PARTIAL** — connected but irregular heartbeats (gaps+bursts), dropped at 48s |
| 32b | Same as 32 but 10ms delay | 10ms | **UNTESTED** — ESP32 went offline (needs power cycle) |

### Root Cause
The ESP32-C3 is **single-core**. NimBLE runs as a FreeRTOS task. The Arduino `delay()` yields to FreeRTOS, giving NimBLE CPU time. At `delay(5)` (200Hz), the loop runs too fast and starves NimBLE — heartbeat notifications queue up and burst, and the connection eventually drops. The original broken firmware (Tests 24-29) was even worse: 5ms polling + BLE button notifications + simulated presses all at once.

### What was NOT the problem
- GPIO init (`pinMode`) — completely fine
- The Pi-side code — unchanged and working
- ESP32 hardware — confirmed working in Test 30

---

## Current State of Hardware

- **ESP32-C3**: Flashed with Test 32b firmware (button reading + 10ms delay). Currently **offline** — needs USB power cycle. It's off the protoboard (no buttons wired). Was moved further from Pi by user.
- **Raspberry Pi**: Running Pi-Claude, polling git for TASK.md updates. `ble_receiver.py` has: scan-first on cold start, `systemctl restart bluetooth` on startup, full adapter reset between retries.
- **Mac**: Dev machine, ESP32 connected via USB for flashing.

---

## What To Do Tomorrow

### Step 1: Re-run Test 32b
1. **Power cycle the ESP32** (unplug/replug USB cable)
2. Optionally check serial monitor to confirm it's advertising
3. Pi-Claude should auto-run the test (TASK.md is already set for Test 32b)
4. `git pull` and check `pi/REPORT.md` for results

### Step 2: If Test 32b PASSES (stable connection with button reading at 10ms)
Create **Test 33**: Add BLE button notifications
- Keep 10ms loop delay
- Add `sendButton()` function back — sends single ASCII char as BLE notify
- Add grace period (5s after connect before sending any notifications)
- Add simulated button presses (ESP32 is off protoboard, no physical buttons)
- This tests whether BLE notifications on top of button reading work at 10ms

### Step 3: If Test 33 PASSES
- We have working button firmware! Remove simulated presses.
- Put ESP32 back on protoboard, wire buttons to GPIO 0-4 → GND
- Test with physical button presses

### Step 2 (alt): If Test 32b FAILS (still unstable at 10ms)
- Try `delay(20)` (50Hz) — still plenty fast for 50ms debounce
- Or switch to interrupt-driven buttons (attach ISR to GPIO pins, only process events when buttons change, not every loop iteration)

---

## Key Files

| File | Description |
|------|-------------|
| `esp32/ble_remote/ble_remote.ino` | ESP32 firmware — currently Test 32b (button read, serial only, 10ms delay) |
| `pi/ble_receiver.py` | Pi BLE receiver — unchanged, working well |
| `pi/TASK.md` | Instructions for Pi-Claude — currently set for Test 32b |
| `pi/REPORT.md` | Pi-Claude test results — last entry is Test 32b (ESP32 offline) |

---

## BLE Protocol Reminder

- ESP32 sends single ASCII chars as BLE notifications
- Uppercase = KEYDOWN, lowercase = KEYUP: L/l (Left), R/r (Right), U/u (Up), D/d (Down), O/o (On/Off)
- Heartbeat: 4-byte uint32 counter, sent every 2 seconds
- Pi subscribes to notifications on characteristic `4e520002-...`

---

## Pi-Claude Instructions

Pi-Claude runs on the Raspberry Pi and polls `git pull` for new TASK.md files. When it finds an update:
1. Pulls latest code
2. Runs `python3 pi/ble_receiver.py` for the duration specified in TASK.md
3. Writes results to `pi/REPORT.md`
4. Commits and pushes

To trigger a new test: update `pi/TASK.md`, commit, and push. Pi-Claude will pick it up.

---

## Git Repo
`https://github.com/davideo71/pi-ble-remote` (public)
