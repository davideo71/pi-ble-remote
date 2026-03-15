# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth: `sudo systemctl restart bluetooth && sleep 2`
3. Run `python3 pi/ble_receiver.py` for about 60-90 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **Stripped advertising config back to minimal** — removed `enableScanResponse(true)`, `setMinInterval`, and `setMaxInterval`. These were added in the last iteration and may have broken NimBLE 2.x advertising. Now using only `addServiceUUID()` + `start()`.
2. **Full flash erase** before reflash — cleared all NVS/NimBLE state.
3. **ESP32 confirmed running** — boot log shows correct BLE address, TX power 20 dBm, service UUID registered.

## Key insight
The ESP32 was visible in Tests 2-3-5 (old advertising config) but invisible in Tests 6-7-8 (after adding setMinInterval/setMaxInterval/enableScanResponse). Reverting to the minimal config that worked before.

## Expected
- Device should be visible again (like Tests 2-3-5)
- If found → connect → heartbeat notifications should flow
- If still not found: run `sudo hcitool lescan` for 10 seconds as a lower-level scan check
