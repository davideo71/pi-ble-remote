# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth: `sudo systemctl restart bluetooth && sleep 2`
3. Run `python3 pi/ble_receiver.py` for about 60-90 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **ESP32 advertising interval dramatically reduced**: from default ~1280ms to 20-100ms. This means the ESP32 sends advertising packets 10-60x more frequently, making it much more discoverable. This should fix the intermittent visibility issue (only 1 out of 5 scans found it before).
2. ESP32 was power-cycled via reflash — clean BLE stack state.
3. No changes to the Pi receiver code (the cache timing fix from last round is still in place).

## Expected
- Device should appear in nearly every scan cycle now (not 1 out of 5)
- RSSI should be similar (~-81 dBm) but discovery much more reliable
- If found → connect → heartbeat notifications should flow
