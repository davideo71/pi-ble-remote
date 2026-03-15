# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth completely:
   ```bash
   sudo systemctl restart bluetooth
   sleep 2
   bluetoothctl remove 38:44:BE:45:AD:86 2>/dev/null; true
   ```
3. Run `python3 pi/ble_receiver.py` for about 60 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration — CRITICAL FIXES
1. **Root cause of `br-connection-canceled` found**: When BlueZ has a stale device cache entry (from old "EasyPlay" firmware), it tries BR/EDR instead of BLE. The ESP32-C3 only supports BLE, so the connection is immediately rejected.
2. **Fix 1: Remove BlueZ cache before connect** — the script now calls `bluetoothctl remove <MAC>` before each connection attempt to force BlueZ to create a fresh LE device entry.
3. **Fix 2: Pass BLEDevice object (not string)** — passing the `BLEDevice` from the scanner preserves the LE address type context. Passing a string address loses this and BlueZ defaults to BR/EDR.
4. **Fix 3: Scanner uses `service_uuids` filter** — this tells BlueZ to only report LE devices advertising our service UUID, ensuring correct device type from the start.
5. **TX power already at +20 dBm** from previous iteration.

## Expected
- No more `br-connection-canceled` errors
- Successful BLE connection with heartbeat notifications
- If InProgress errors occur, the adapter auto-resets
