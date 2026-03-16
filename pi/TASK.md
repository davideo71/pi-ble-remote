# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about 90-120 seconds (at least 3 scan cycles)
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 13)
1. **ESP32: fast advertising interval restored** — 20-60ms interval (was default ~1280ms). This was lost in a previous iteration. Should make the ESP32 MUCH more visible during scans.
2. **ESP32: flash erased and reflashed** — completely clean NVS/BLE state.
3. **Pi: adapter reset before EVERY scan** — `reset_bluetooth_adapter()` now runs before each scan cycle. This clears BlueZ's internal duplicate filter that was likely suppressing already-seen devices. This is the "surgical" fix for the observation that restarting bluetooth helped.
4. **Pi: active scanning enabled** — `scanning_mode="active"` forces the scanner to send scan requests, which triggers the ESP32's scan response containing the service UUID.

## Expected
- The adapter reset + active scanning should make the ESP32 reliably visible every scan cycle
- ESP32 should be found by MAC, name ("BLE-Remote"), or service UUID
- If found → connect → subscribe to notifications → report heartbeat data
- Report whether the adapter reset adds noticeable delay to each scan cycle

## Key question to answer
Does the ESP32 show up **consistently** across multiple scan cycles? (Previous tests showed 0-3 out of 5 scans finding it)
