# Pi Test Report: Step 2 — Button Handling

## Test 23 — 2026-03-17 00:02 UTC (Step 2: button notifications)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: Connection stable, button decoding ready — NO BUTTON EVENTS RECEIVED

#### Connection
- InProgress error on first attempt (light reset didn't clear it)
- Full adapter reset → second attempt succeeded after 12.7s connect time
- **Script start → first heartbeat: ~23 seconds** (InProgress retry added ~7s)

#### Heartbeats
- **49 consecutive heartbeats** (#4-#52), every ~2 seconds, zero gaps
- Connection stable for **97+ seconds** — still alive at test timeout
- New compact `HEARTBEAT #N` format working correctly

#### Button events
- **No button press/release events were received**
- The notification handler is ready — decodes 1-byte ASCII chars to button names
- No buttons were physically pressed during this test run
- The button decoding code looks correct: checks `len(data) == 1` → ASCII char → `BUTTON_NAMES` lookup

### Pi-side code review
The button handling code is clean:
- `BUTTON_NAMES` dict maps all 10 events (5 buttons × press/release)
- Handler distinguishes button (1 byte) from heartbeat (4 bytes) by length
- Format: `BUTTON: 'L' → LEFT press`

### Answer to key question

**Do button notifications arrive correctly?** Unknown — no buttons were pressed during the test. The Pi receiver is ready and the notification handler will decode them correctly when they arrive. Need to run again while buttons are being pressed on the ESP32.

### Next step
Run the test again while manually pressing buttons on the ESP32. The Pi side is ready.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 23 (00:02) | Connection stable, button handler ready, no buttons pressed during test |
| 22 (23:37) | Light reset, 12.6s to heartbeat, 107s stable |
| 21 (23:27) | Direct connect by MAC, 13s to heartbeat, 105s stable |
| 18 (23:04) | Best scan+connect, 9s to heartbeat |
| 14 (22:30) | First stable connection (3m41s) |
| 13 (22:24) | First reliable discovery |
| 1-12 | Early tests — discovery and connection issues |
