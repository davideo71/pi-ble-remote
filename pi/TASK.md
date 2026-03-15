# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about 30 seconds
3. If "BLE-Remote" is still not found by name, also check if MAC `38:44:BE:45:AD:86` appears (that's our ESP32-C3's BLE address). The previous scan showed this MAC as "EasyPlay" — after a full flash erase and reflash it should now show as "BLE-Remote".
4. If neither name nor MAC works, try a raw `bluetoothctl` scan to see what the Pi's BlueZ stack reports:
   ```bash
   bluetoothctl scan le
   ```
   (run for ~10 seconds, then Ctrl+C)
5. Update `pi/REPORT.md` with the new results, commit and push.

## What changed
- Full flash erase (`erase_flash`) was done to clear stale NVS data that was caching the old name "EasyPlay"
- Firmware was reflashed — ESP32-C3 should now advertise as "BLE-Remote"
- ESP32-C3 WiFi MAC: `38:44:BE:45:AD:84`, BLE MAC should be `38:44:BE:45:AD:86`
