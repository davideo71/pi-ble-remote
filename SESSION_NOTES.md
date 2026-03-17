# Session Notes — March 17, 2026 (Session 2)

## Quick Summary for Next Session

Switched from ESP32-C3 to ESP32-S3 SuperMini (dual-core). The S3 flashes successfully but **we can't verify it's running** — no serial output from native USB CDC, and Pi can't find it in BLE scans. Currently flashed with a blink test sketch to check if the board runs code at all.

---

## What Was Proven Today

### C3 single-core starvation was real BUT Pi BlueZ degradation was the bigger problem
- Tests 32-34c all failed, we blamed digitalRead/interrupts/serial
- **Test 31b proved the baseline had degraded** — same firmware that passed at 01:14 took 8 attempts at 10:53
- **Test 35 (after Pi reboot)** passed perfectly — 89 heartbeats, first attempt, interrupt-driven buttons
- Test 35b (range test) failed — ESP32 was out of BLE range at new position

### Key lesson: Pi BlueZ degrades after many test cycles
- `systemctl restart bluetooth` may help
- Full Pi reboot is the guaranteed fix
- `ble_receiver.py` now auto-restarts bluetooth after 3 consecutive failures

### Switched to ESP32-S3 SuperMini
- Dual-core eliminates NimBLE starvation entirely
- Flashes OK via `arduino-cli` with FQBN `esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc`
- **Problem: No serial output** from native USB CDC — tried multiple times
- **Problem: Not advertising BLE** — Pi scans find nothing
- Board may not be running code, or `LED_BUILTIN` pin may be wrong for SuperMini

---

## Current State of Hardware

- **ESP32-S3 SuperMini**: Connected via USB-C to Mac. Flashed with blink test. No confirmed activity (no serial, not found in BLE scans). Need to visually check LED.
- **ESP32-C3**: Disconnected/off. Last known-good firmware was Test 34 (interrupt-driven buttons) — worked perfectly on fresh Pi reboot.
- **Raspberry Pi**: Rebooted ~11:45 UTC. Pi-Claude last reported at 12:35 UTC. May need restart.

---

## What To Do Next

### Step 1: Verify S3 board works
1. Check if LED is blinking on the S3 SuperMini
2. If no LED: the board may be dead, or `LED_BUILTIN` is wrong pin. Try pin 48 (common on S3 SuperMini) or check board docs
3. If LED works but no serial: native USB CDC issue — try different USB cable or `USBMode=default` (OTG mode)

### Step 2: If S3 works, get BLE running
1. Flash BLE heartbeat firmware
2. Confirm Pi can find and connect
3. Then add buttons — should be straightforward on dual-core

### Step 2 (alt): If S3 doesn't work, go back to C3
The C3 interrupt-driven firmware (Test 34) works fine on a fresh Pi. We just need to:
- Keep the ESP32 within range (RSSI > -89 dBm)
- Reboot Pi before test sessions
- Add BLE button notifications (Test S3-3 plan applies to C3 too)

---

## Key Files

| File | Description |
|------|-------------|
| `esp32/ble_remote/ble_remote.ino` | Currently has S3 heartbeat firmware (but /tmp/blink_test is flashed) |
| `pi/ble_receiver.py` | Pi BLE receiver — MAC set to S3 address, has auto-recovery |
| `pi/TASK.md` | Instructions for Pi-Claude — set for S3 broad scan |
| `pi/REPORT.md` | Last entry: S3-1b scan found nothing |

---

## BLE Protocol Reminder

- Device name: `BLE-Remote`
- Service UUID: `4e520001-7354-4288-9a71-81a9bf56c4a8`
- Button Characteristic UUID: `4e520002-7354-4288-9a71-81a9bf56c4a8` (notify)
- Heartbeat: 4-byte uint32 counter, sent every 2 seconds
- Buttons: single ASCII char — Uppercase=KEYDOWN, lowercase=KEYUP (L/l, R/r, U/u, D/d, O/o)

---

## Arduino-CLI Commands

```bash
# Compile for S3
arduino-cli compile --fqbn "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc" esp32/ble_remote/

# Flash S3 (may need bootloader mode: hold BOOT, tap RESET, release BOOT)
arduino-cli upload --fqbn "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc" --port /dev/cu.usbmodem1101 esp32/ble_remote/

# Compile for C3 (if we go back)
arduino-cli compile --fqbn esp32:esp32:esp32c3 esp32/ble_remote/
arduino-cli upload --fqbn esp32:esp32:esp32c3 --port /dev/cu.usbmodem1101 esp32/ble_remote/
```
