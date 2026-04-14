# BLE Remote for EasyPlay

A wireless 5-button remote control for [EasyPlay](https://github.com/davideo71/EasyPlay), a media player running on Raspberry Pi 5. The remote uses BLE (Bluetooth Low Energy) to communicate with the Pi.

## The Remote

A compact handheld remote built around the ESP32-C3 SuperMini, powered by two 18650 batteries. Five buttons control media playback: navigate left/right/up/down through the interface and on/off to toggle playback.

### Components
- **MCU**: ESP32-C3 SuperMini (built-in BLE 5, USB-C for programming)
- **Buttons**: 5x momentary push buttons wired to GND (active LOW with internal pull-ups)
- **Power**: 2x 18650 Li-ion batteries in parallel (~5000-6000mAh)
- **LED**: Onboard WS2812B NeoPixel for status feedback
- **Enclosure**: 3D printed (STL file TBD)

### Pin Assignment
| GPIO | Button  |
|------|---------|
| 0    | On/Off  |
| 1    | Right   |
| 2    | Up      |
| 3    | Down    |
| 4    | Left    |
| 8    | NeoPixel LED |

### Features
- **Deep sleep**: Sleeps after 2 minutes of inactivity (when not connected). Any button press wakes the remote. Sleep current ~50uA.
- **Deferred wake button**: When a button wakes the remote from sleep, that button press is sent to the Pi once BLE reconnects.
- **Two LED modes** (hold On/Off for 10 seconds to toggle):
  - **Efficiency** (default): LED off, brief NeoPixel flash on button press. Double flash if Pi is not yet connected.
  - **Debug**: Breathing LED — blue (advertising), green (connected), red (last 30s before sleep).
- **Battery life**: Estimated 5-6 months with light use (red power LED removed).

## Raspberry Pi Setup

### Requirements
- Raspberry Pi 5 running Raspberry Pi OS (Bookworm/Trixie)
- [EasyPlay](https://github.com/davideo71/EasyPlay) installed as a systemd service
- Bluetooth adapter (built-in or USB dongle)

### Force 1080p Output
The Pi 5 uses KMS for display. Add to the end of `/boot/firmware/cmdline.txt` (keep it one line):
```
video=HDMI-A-1:1920x1080@60D
```

### Set Audio Volume
```bash
wpctl set-volume @DEFAULT_AUDIO_SINK@ 1.0
```
Use the TV remote for volume control after this.

### Configure BLE Remote
Edit `~/Desktop/EasyPlay/easyplay_config.json`:
```json
{
  "bluetooth_remote_addr": "AC:EB:E6:4B:63:CE",
  "bluetooth_remote_name": "EasyPlay"
}
```
Then restart EasyPlay:
```bash
sudo systemctl restart easyplay
```

## Flashing the Remote

Requires [arduino-cli](https://arduino.github.io/arduino-cli/) with the ESP32 board package and NimBLE-Arduino + Adafruit NeoPixel libraries installed.

```bash
cd esp32/remote_c3_v7
arduino-cli compile --fqbn esp32:esp32:esp32c3 remote_c3_v7.ino
arduino-cli upload --fqbn esp32:esp32:esp32c3 --port /dev/cu.usbmodem1101 remote_c3_v7.ino
```

## Planned Features
- **Safe shutdown**: Long-press On/Off (5s) to trigger `sudo shutdown -h now` on the Pi, avoiding SD card corruption from unplugging. Needs implementation in EasyPlay's button handler (5s hold timer on `O` press, trigger on late release). Won't conflict with remote's 10s LED mode toggle.
- **3D printed enclosure**: STL file TBD.

## Repository Structure
```
esp32/remote_c3_v7/   Current production firmware (v7, deep sleep + LED modes)
esp32/ble_remote/     Legacy test firmware (heartbeat only)
pi/ble_receiver.py    Standalone BLE receiver (not needed when EasyPlay handles BLE)
```
