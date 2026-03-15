# BLE Remote for Raspberry Pi Media Player

## Project Overview
A 5-button BLE remote control (ESP32-C3) that communicates with a Raspberry Pi running a custom media player. The ESP32-C3 is the BLE peripheral (GATT server), the Pi is the BLE central (GATT client).

## Architecture
- `esp32/ble_remote/` — Arduino firmware (NimBLE-Arduino library). Flashed from the Mac dev machine.
- `pi/ble_receiver.py` — Python BLE receiver using bleak (asyncio). Runs on the Pi.

## BLE Protocol
- Device name: `BLE-Remote`
- Service UUID: `4e520001-7354-4288-9a71-81a9bf56c4a8`
- Button Characteristic UUID: `4e520002-7354-4288-9a71-81a9bf56c4a8` (notify)
- The ESP32 sends button events as notifications. The Pi subscribes and reacts.

## Hardware
- ESP32-C3 with 5 push buttons: left, right, up, down, on/off
- Powered by 2x 18650 batteries (sleep modes matter but not yet implemented)
- Pi and remote are in the same room

## Development Setup
- **Mac** (dev machine): edits both ESP32 and Pi code, flashes ESP32, pushes to GitHub
- **Pi**: pulls from GitHub, runs `pi/ble_receiver.py`
- Repo: https://github.com/davideo71/pi-ble-remote (public, no auth needed to clone/pull)

## Key Design Decisions
- ESP32 sends generic button events (press/release with button ID), Pi decides what they mean
- Button-to-action mapping lives on the Pi side
- Robust auto-reconnection is critical on both sides
- NimBLE-Arduino chosen over Bluedroid for RAM efficiency on ESP32-C3
- bleak chosen as the standard Python async BLE library

## Current State
Step 1: basic BLE connection with heartbeat notifications and verbose debug logging on both sides. No buttons wired yet. Next steps are testing the connection, then adding GPIO button handling on the ESP32.

## Running on the Pi
```bash
pip install -r pi/requirements.txt
python3 pi/ble_receiver.py
```

## Code Style
- ESP32: Arduino C++ with descriptive Serial.printf debug output
- Pi: Python with asyncio, timestamped log lines, type hints where helpful
- Both sides should have verbose debug logging during development
