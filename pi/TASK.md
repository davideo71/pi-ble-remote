# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about 90-120 seconds (at least 3 scan cycles)
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 13)
1. **ESP32: TX power reduced to +9 dBm** (was +20 dBm). Cheap ESP32-C3 Super Mini clones have poor RF matching — max power can distort the signal. Lower power = cleaner signal.
2. **ESP32: fast advertising interval 20-60ms** (was default ~1280ms).
3. **ESP32: flash erased and reflashed** — completely clean NVS/BLE state.
4. **Pi: adapter reset before EVERY scan** — `reset_bluetooth_adapter()` now runs before each scan cycle. This clears BlueZ's internal duplicate filter.
5. **Pi: active scanning enabled** — `scanning_mode="active"` forces scan requests, triggering the ESP32's scan response.

## Expected
- The combination of lower TX power + adapter reset + active scanning should make the ESP32 reliably visible
- Compare RSSI to previous tests (was -81 to -89 dBm at +20 dBm power)
- If found → connect → subscribe to notifications → report heartbeat data
- Report whether the adapter reset adds noticeable delay

## Key question to answer
Does the ESP32 show up **consistently** across multiple scan cycles?
