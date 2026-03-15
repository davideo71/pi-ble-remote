# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth: `sudo systemctl restart bluetooth && sleep 2`
3. Run `python3 pi/ble_receiver.py` for about 60-90 seconds (device appears intermittently)
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **Moved `bluetoothctl remove` from between scan/connect to BEFORE the scan** — Test 5 showed that removing the BlueZ device after scanning destroyed the D-Bus object bleak needed. Now we clear stale cache first, then scan discovers the device fresh as LE.
2. **Only removes cache on reconnect** (when we have a known address) — first scan runs without cache removal.
3. **service_uuids filter confirmed working** — only our device shows up in scans.

## Known issues
- ESP32 appears intermittently (only 1 out of 5 scan cycles in test 5). If it doesn't appear, let it run longer.
- Name still shows as "EasyPlay" — cosmetic, doesn't affect functionality since we match by UUID.

## Expected
- Device found by UUID scan → connect succeeds → heartbeat notifications flow
