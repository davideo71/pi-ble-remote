# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. First, clear any remaining BlueZ cache for our device:
   ```bash
   bluetoothctl remove 38:44:BE:45:AD:86 2>/dev/null; true
   ```
3. Run `python3 pi/ble_receiver.py` for about 45 seconds
4. The scanner now searches by **service UUID** instead of device name, which should bypass BlueZ name caching issues
5. Update `pi/REPORT.md` with results, commit and push

## What changed
- `ble_receiver.py` now scans by service UUID (`4e520001-...`) as primary match, falls back to name
- The detection callback logs `** MATCH **` when it finds our service UUID in advertisements
- If still not found, check whether the service UUID appears in `bluetoothctl` output:
  ```bash
  sudo btmon &
  bluetoothctl scan le
  ```
  (run for ~15 seconds, look for UUID `4e520001` in the btmon output)

## Known state
- ESP32-C3 MAC (BLE): `38:44:BE:45:AD:86`
- ESP32-C3 is advertising with service UUID `4e520001-7354-4288-9a71-81a9bf56c4a8`
- Signal was weak (-86 to -89 dBm) last test — try moving Pi and ESP32 closer if possible
