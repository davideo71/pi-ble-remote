# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth: `sudo systemctl restart bluetooth && sleep 2`
3. Run `python3 pi/ble_receiver.py` for about 60-90 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **Removed `service_uuids` filter from BleakScanner** — Test 10 proved this D-Bus filter is unreliable and silently drops the ESP32. UUID matching is now done purely in the detection callback (which was already there).
2. **No ESP32 firmware changes** — same minimal advertising config.

## Expected
- Device should be found reliably (unfiltered scan sees it consistently)
- Connection should succeed (no more BR/EDR confusion, device discovered fresh as LE)
- If connected → heartbeat notifications every 2 seconds
- If connection still times out: try `bluetoothctl connect 38:44:BE:45:AD:86` manually and report the error
