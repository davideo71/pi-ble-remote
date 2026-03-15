# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth: `sudo systemctl restart bluetooth && sleep 2`
3. Run `python3 pi/ble_receiver.py` for about 60-90 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **ESP32: periodic re-advertising every 30 seconds** — NimBLE was silently stopping advertising. The loop now calls `start()` every 30s as a safeguard.
2. **ESP32: scan response enabled** — service UUID in scan response data.
3. **Pi: name/MAC fallback matching** — already deployed last round.
4. **ESP32 flash erased and reflashed** — clean state.

## Expected
- ESP32 should stay discoverable even after long uptime
- Device should be found by MAC, name, or UUID
- If found → try to connect → report results
