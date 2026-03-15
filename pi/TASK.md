# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Clear BlueZ cache and reset adapter:
   ```bash
   bluetoothctl remove 38:44:BE:45:AD:86 2>/dev/null; true
   sudo systemctl restart bluetooth
   sleep 2
   ```
3. Run `python3 pi/ble_receiver.py` for about 60 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **TX power fixed to +20 dBm** (was ~7 dBm due to wrong enum). RSSI should be much better.
2. **Connect by address string** instead of device object — avoids BlueZ trying BR/EDR (classic Bluetooth) transport. The `br-connection-canceled` error from test 3 was because BlueZ was trying a classic connection.
3. **BlueZ stuck-state recovery** — when "InProgress" errors occur, the script now resets the Bluetooth adapter automatically (power off/on cycle).

## Expected
- Much better RSSI (was -81 to -85 dBm, should be -40 to -60 dBm now)
- Connection should succeed (no more br-connection-canceled)
- Should see heartbeat notifications flowing
- If BlueZ gets stuck, it should auto-recover
