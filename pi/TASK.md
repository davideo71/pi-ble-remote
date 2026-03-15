# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Clear BlueZ cache: `bluetoothctl remove 38:44:BE:45:AD:86 2>/dev/null; true`
3. Run `python3 pi/ble_receiver.py` for about 45 seconds
4. The scanner now searches by **service UUID** — look for `** MATCH **` lines in the output
5. If it connects, let it run for ~20 seconds to capture heartbeat notifications
6. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
- **Fixed TX power**: was passing old enum `ESP_PWR_LVL_P9` (value ~7) to NimBLE 2.x which takes dBm directly. Now set to **+20 dBm** (maximum). This should significantly improve RSSI (was -86 to -89 dBm at 1 meter!).
- **Scanner uses service UUID**: bypasses BlueZ name caching entirely
- ESP32 boot log now confirms: `TX power set to: 20 dBm`

## Expected
- RSSI should be much better (expect -40 to -60 dBm at 1 meter)
- Service UUID match should trigger `** MATCH **` in scan output
- If found, it should connect and start receiving heartbeat notifications
