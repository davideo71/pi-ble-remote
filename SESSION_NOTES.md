# Session Notes — March 18, 2026 (Session 3)

## Quick Summary for Next Session

**BLE connection is WORKING!** Test 45b: ESP32-S3 SuperMini connected to Pi, received 34 heartbeats over 65 seconds with rock-solid stability. No buttons wired yet — that's the next step.

**Current hardware:**
- ESP32-S3 SuperMini (fresh board, flashed via esptool)
- Raspberry Pi with CSR8510 USB Bluetooth dongle
- No buttons wired yet

**What's flashed right now:**
- S3: Test 45 firmware — always-on advertising, heartbeat every 2s, LED slow blink when advertising / solid when connected
- Pi: `ble_receiver.py` v2 with nuclear BlueZ cache clearing

---

## Major Breakthrough: BlueZ Cache is the Root Cause of Everything

### The "EasyPlay" Ghost Name Mystery (SOLVED)
The ESP32-C3 kept showing as "EasyPlay" (old MicroPython firmware name) despite being flashed with "BLE-Remote" firmware. We tried:
- Full 4MB flash erase + rewrite
- NVS partition erase
- OTA data partition erase
- `nvs_flash_erase()` in firmware setup()
- Binary verification (strings showed "BLE-Remote", no "EasyPlay")

**None of it worked** because the problem was never on the ESP32. It was on the Pi.

**Root cause:** BlueZ (Linux Bluetooth daemon) aggressively caches device names, GATT service tables, and connection parameters **in memory**. Deleting files in `/var/lib/bluetooth/*/cache/` while bluetoothd is running does nothing — the daemon regenerates them instantly from its in-memory copy.

**Proof:** User unplugged the ESP32 → "EasyPlay" disappeared from scans → plugged back in → "EasyPlay" reappeared. The name was coming from BlueZ's memory cache, not from the device's actual advertisement.

### BlueZ Cache Also Caused Connection Failures
Looking back at all test results:
- **Test 31 (fresh Pi session):** Connected on 1st attempt
- **Tests 32-34c (same session, many cycles later):** All FAILED
- **Test 35 (after Pi reboot):** Connected on 1st attempt, 89 heartbeats
- **Test 45b (after nuclear cache clear):** Connected, 34 heartbeats

The pattern is clear: BlueZ accumulates stale GATT data from repeated connect/disconnect cycles. This stale data causes GATT service discovery to fail silently, manifesting as connection timeouts.

### The Fix: Nuclear Cache Clear
The ONLY reliable way to clear BlueZ cache:

```bash
# 1. STOP the daemon (must be stopped to clear memory)
sudo systemctl stop bluetooth
sleep 0.5

# 2. DELETE all cached device data while daemon is stopped
sudo rm -rf /var/lib/bluetooth/*/cache/*

# 3. RESTART the daemon with clean state
sudo systemctl start bluetooth
sleep 3  # BlueZ LE subsystem needs time to initialize
```

This is now built into `ble_receiver.py` v2:
- Runs automatically on startup
- Triggers again after 3 consecutive connection failures
- Adapter power cycle (less aggressive) runs between scan attempts

### Other BlueZ Cache Facts Learned
- `bluetoothctl remove <MAC>` removes device from BlueZ internal list BUT this makes bleak unable to find it for connection — counterproductive before connect
- After nuclear cache clear, you MUST wait ~3 seconds for BlueZ LE subsystem to reinitialize
- Simple `systemctl restart bluetooth` is NOT enough — it may preserve some in-memory state
- A full Pi reboot always works but is slow

---

## Hardware Notes

### ESP32-S3 SuperMini (ACTIVE)
- **MAC (base):** `A0:F2:62:EC:51:C8`
- **MAC (BLE):** `A0:F2:62:EC:51:CA` (base + 2, though Pi saw `A0:F2:62:EC:51:C9`)
- **LED pin:** GPIO 48 (NeoPixel)
- **Flash:** 4MB XMC, 2MB PSRAM
- **Chip:** ESP32-S3 (QFN56) revision v0.2, dual-core + LP core, 240MHz
- **USB mode:** USB-Serial/JTAG
- **Board config:** `esp32:esp32:lolin_s3_mini` (closest match for SuperMini)
- **IMPORTANT:** `arduino-cli upload` does NOT work — use `esptool` directly for flashing

### ESP32-C3 (with antenna, NOT IN USE)
- **MAC (base):** `38:44:be:45:ad:84`
- **MAC (BLE):** `38:44:BE:45:AD:86` (base + 2)
- **LED pin:** GPIO 8
- **Board config:** `esp32:esp32:esp32c3`
- Had persistent "EasyPlay" name issue (BlueZ cache — now understood)
- RSSI was -83 to -88 even at 10cm — may have antenna connector issue
- `arduino-cli upload` works fine for this board

### Raspberry Pi
- **BLE adapter:** CSR8510 A10 USB dongle (hci0), MAC `00:1A:7D:DA:71:13`
- No onboard Bluetooth adapter detected (may be disabled or absent)
- RSSI consistently -83 to -87 dBm — this is a dongle limitation, same with both C3 and S3

### The Old EasyPlay Setup (HISTORICAL)
- `remote_main_v7.py` — MicroPython on ESP32-S3, using `aioble` library
- Used Nordic UART Service UUIDs (different from our BLE-Remote UUIDs)
- Had sleep/wake cycle: sleeps after 3 min idle, wakes on button press with 10s burst advertising
- Device name `EasyPlay` got cached in BlueZ, persisted across firmware changes
- Located at `/Users/dkousemaker/Desktop/EasyPlay/remote_main_v7.py`

---

## Current File State

| File | Description | Status |
|------|-------------|--------|
| `esp32/ble_remote/ble_remote.ino` | Test 45 firmware for S3 — always-on advertising, heartbeat, LED | FLASHED ON S3 |
| `pi/ble_receiver.py` | v2 receiver with nuclear cache clear, dual-profile support | WORKING |
| `pi/TASK.md` | Test 45b instructions for Pi-Claude | DONE |
| `pi/REPORT.md` | Test 45b: SUCCESS — 34 heartbeats | LATEST |
| `CLAUDE.md` | Project overview and dev instructions | UP TO DATE |

---

## Flashing Commands

### ESP32-S3 SuperMini (use esptool, NOT arduino-cli upload)
```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:lolin_s3_mini esp32/ble_remote/

# Or compile to output dir for esptool
arduino-cli compile --fqbn esp32:esp32:lolin_s3_mini --output-dir /tmp/s3_build esp32/ble_remote/

# Flash with esptool (hold BOOT button while plugging in USB first!)
python3 -m esptool --chip esp32s3 --port /dev/cu.usbmodem1101 --baud 921600 write_flash \
  0x0000 /tmp/s3_build/ble_remote.ino.bootloader.bin \
  0x8000 /tmp/s3_build/ble_remote.ino.partitions.bin \
  0x10000 /tmp/s3_build/ble_remote.ino.bin

# Check chip ID (verify board is detected)
python3 -m esptool --chip esp32s3 --port /dev/cu.usbmodem1101 chip_id
```

### ESP32-C3 (arduino-cli works fine)
```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3 esp32/ble_remote/
arduino-cli upload --fqbn esp32:esp32:esp32c3 --port /dev/cu.usbmodem1101 esp32/ble_remote/
```

---

## BLE Protocol

- **Device name:** `BLE-Remote`
- **Service UUID:** `4e520001-7354-4288-9a71-81a9bf56c4a8`
- **Button Characteristic UUID:** `4e520002-7354-4288-9a71-81a9bf56c4a8` (read, notify)
- **Heartbeat:** 4-byte uint32 counter, sent every 2 seconds
- **Buttons (not yet wired):** single ASCII char — Uppercase=KEYDOWN, lowercase=KEYUP
  - L/l = LEFT, R/r = RIGHT, U/u = UP, D/d = DOWN, O/o = ON/OFF
- **TX power:** +9 dBm (max)
- **Advertising interval:** 20-40ms (fast for testing, increase for battery life later)

---

## Next Steps (Priority Order)

### 1. Wire buttons on the S3
- 5 buttons: LEFT, RIGHT, UP, DOWN, ON/OFF
- Connect to GPIO pins with internal pull-ups, buttons pull to GND
- Use interrupt-driven approach (worked on C3 in Test 35 with 89 heartbeats)
- Send single ASCII char per button event (uppercase=press, lowercase=release)
- Pick GPIO pins that are safe on S3 SuperMini (avoid strapping pins)

### 2. Test buttons end-to-end
- Flash button firmware on S3
- Run `ble_receiver.py` on Pi
- Verify button events arrive correctly
- Test rapid button presses, simultaneous buttons

### 3. Button-to-action mapping on Pi
- Pi decides what each button does (per CLAUDE.md design)
- Map buttons to media player controls
- This is all Pi-side code — no firmware changes needed

### 4. Production hardening
- Add sleep mode on S3 for battery life (wake on button press)
- Increase advertising interval when not in active use
- Add connection parameter negotiation for lower power
- Consider adding battery level BLE characteristic
- Consider replacing CSR8510 dongle with better BLE adapter

---

## Lessons Learned

1. **BlueZ cache is aggressive and dangerous** — always do nuclear cache clear (stop → delete → restart) when debugging BLE on Linux. Deleting cache files while daemon runs is useless.

2. **arduino-cli upload doesn't work for all boards** — for S3 SuperMini, use esptool directly. Compile with arduino-cli, flash with esptool.

3. **RSSI -83 to -87 at close range is a dongle issue** — the CSR8510 USB dongle has weak reception. Consider using Pi's onboard Bluetooth if available, or a better USB adapter.

4. **OTA dual-partition can cause confusion** — the ESP32 OTA scheme writes new firmware to app0 but bootloader may load from app1 if OTA data partition is stale. Erase OTA data (0xe000) when switching firmware.

5. **Always verify with a clean observer** — when device names seem wrong, test from a device that has never seen the device before (we tried Mac Bluetooth to rule out Pi-side caching).

6. **`remove_device()` before connect is counterproductive** — it removes the device from BlueZ's internal device list, causing bleak to fail on connect. The nuclear cache clear on startup is sufficient.

7. **Pi reboot is the guaranteed BlueZ fix** — when all else fails, reboot the Pi. It's slow but always works.

8. **The Mac-Claude / Pi-Claude collaboration model works** — using TASK.md and REPORT.md via git, with Mac-Claude polling every 2 minutes. Main friction: Pi-Claude sometimes stops (token expiry, crashes) and needs manual restart.

---

## Test History (Sessions 2-3)

| Test | Board | Result | Key Finding |
|------|-------|--------|-------------|
| 30 | C3 | PASS | First successful connection |
| 31 | C3 | PASS | Baseline confirmed |
| 31b | C3 | DEGRADED | 8 attempts — BlueZ degradation |
| 32-34c | C3 | FAIL | Blamed buttons, was actually BlueZ |
| 35 | C3 | PASS (89 hb) | After Pi reboot — proved BlueZ was the issue |
| 36-38 | S3 (old) | FAIL | S3 SuperMini wouldn't output serial |
| 39-39b | C3 | FAIL | "EasyPlay" ghost name (BlueZ cache) |
| 40-44 | C3 | Various | Debugging EasyPlay name, OTA partition |
| 45 | S3 (new) | FAIL | EasyPlay firmware asleep, no button press |
| **45b** | **S3 (new)** | **PASS (34 hb)** | **Working! Always-on firmware + nuclear cache clear** |
