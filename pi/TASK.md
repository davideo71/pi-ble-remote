# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth: `sudo systemctl restart bluetooth && sleep 2`
3. Run `python3 pi/ble_receiver.py` for about 60-90 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **ESP32: re-enabled scan response** — `enableScanResponse(true)` added back without the interval settings. This ensures the service UUID appears in scan response data. Only the interval settings (setMinInterval/setMaxInterval) broke things in Tests 6-8, not scan response itself.
2. **Pi: match by name or MAC as fallback** — detection callback now matches by UUID, MAC address (`38:44:BE:45:AD:86`), or name (`BLE-Remote` or `EasyPlay`). This handles cases where the service UUID isn't in the advertisement data.
3. **ESP32 flash erased and reflashed** — clean NVS state.
4. **ESP32 confirmed running** with scan response enabled.

## Expected
- Device should be found more reliably with name/MAC fallback
- Service UUID should be in scan response data now
- If connection times out: try `bluetoothctl connect 38:44:BE:45:AD:86` manually
