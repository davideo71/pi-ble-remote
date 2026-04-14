# BLE Remote for Raspberry Pi Media Player

## Project Overview
A 5-button BLE remote control (ESP32-C3 SuperMini) that communicates with a Raspberry Pi 5 running EasyPlay media player. The ESP32-C3 is the BLE peripheral (GATT server), the Pi is the BLE central (GATT client).

## Architecture
- `esp32/remote_c3_v7/` — Current production firmware (Arduino, NimBLE). Deep sleep + dual LED modes.
- `esp32/ble_remote/` — Legacy test firmware (heartbeat only, custom UUIDs).
- `pi/ble_receiver.py` — Standalone BLE receiver (not used when EasyPlay handles BLE directly).

## BLE Protocol
- Device name: `EasyPlay`
- Service: Nordic UART Service (NUS)
  - Service UUID: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
  - TX UUID: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` (notify — remote → Pi)
  - RX UUID: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` (write — Pi → remote)
- Button codes: uppercase = press, lowercase = release (L/l R/r U/u D/d O/o)

## Hardware — Remote
- ESP32-C3 SuperMini (no external antenna needed, same room)
- 5 push buttons wired to GND (active LOW, internal pull-ups):
  - GPIO 0 = On/Off (note: strapping pin, excluded from wake detection)
  - GPIO 1 = Right
  - GPIO 2 = Up
  - GPIO 3 = Down
  - GPIO 4 = Left
- NeoPixel + blue LED on GPIO 8 (shared pin, blue is active HIGH)
- Powered by 2x 18650 batteries in parallel (~5000-6000mAh)
- Deep sleep current: ~50µA (red power LED removed)

## Firmware Features (v7)
- **Deep sleep**: 2 min idle timeout (when not connected), any button wakes
- **Deferred wake button**: detects which button woke from sleep, sends press+release to Pi after BLE connects (GPIO 0 excluded — strapping pin false-triggers)
- **Two LED modes** (hold On/Off 10s to toggle, persists across sleep):
  - **Efficiency** (default): LED off, NeoPixel white flash on button press, double flash if not connected
  - **Debug**: breathing LEDs (blue=advertising, green=connected, red=last 30s before sleep)
- **Clean sleep transition**: stops advertising, disconnects BLE, waits, then sleeps

## Development Setup
- **Mac** (dev machine): edits code, flashes ESP32 via USB, pushes to GitHub
- **Pi**: pulls from GitHub, runs EasyPlay which handles BLE directly
- Repo: https://github.com/davideo71/pi-ble-remote

## Pi Setup

### Force 1080p output (Pi 5)
The Pi 5 ignores legacy `hdmi_group`/`hdmi_mode` in config.txt. Use KMS cmdline instead:
```bash
# Add to end of /boot/firmware/cmdline.txt (single line, space-separated):
video=HDMI-A-1:1920x1080@60D
```

### Set audio volume
```bash
# Set PipeWire system volume to 100% (use TV for volume control)
wpctl set-volume @DEFAULT_AUDIO_SINK@ 1.0
```

### Configure BLE remote MAC
Edit `~/Desktop/EasyPlay/easyplay_config.json`:
```json
{
  "bluetooth_remote_addr": "AC:EB:E6:4B:63:CE",
  "bluetooth_remote_name": "EasyPlay"
}
```
Then restart: `sudo systemctl restart easyplay`

### EasyPlay auto-start
EasyPlay runs as a systemd service (already enabled):
```bash
sudo systemctl status easyplay    # check status
sudo systemctl restart easyplay   # restart
journalctl -u easyplay -f         # follow logs
```

## Flashing the Remote
```bash
cd esp32/remote_c3_v7
arduino-cli compile --fqbn esp32:esp32:esp32c3 remote_c3_v7.ino
arduino-cli upload --fqbn esp32:esp32:esp32c3 --port /dev/cu.usbmodem1101 remote_c3_v7.ino
```

## Code Style
- ESP32: Arduino C++ with descriptive Serial.printf debug output
- Pi: Python with asyncio, timestamped log lines, type hints where helpful
