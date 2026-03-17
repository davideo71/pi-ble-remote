# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. **Restart bluetooth service** before testing: `sudo systemctl restart bluetooth && sleep 3`
3. Run `python3 pi/ble_receiver.py` for about **2 minutes**
4. Update `pi/REPORT.md` with results, commit and push

## IMPORTANT: New hardware — ESP32-S3 SuperMini
We switched from the ESP32-C3 to an **ESP32-S3 SuperMini** (dual-core, 240MHz). This changes:
- **New BLE MAC address**: `A0:F2:62:EC:7A:D0` (updated in `ble_receiver.py`)
- **Device name**: still `BLE-Remote`
- **Service/Char UUIDs**: unchanged
- The S3 is dual-core so NimBLE gets its own core — the single-core starvation issues are gone

## Test S3-1: Heartbeat only
Simple baseline test — same as our proven heartbeat firmware but on the S3. Confirm BLE connects and heartbeats flow.

## Collaboration plan going forward
After this baseline passes, we'll iterate quickly:
1. **S3-1**: Heartbeat only (this test)
2. **S3-2**: Add button polling (digitalRead + debounce) — should work fine on dual-core
3. **S3-3**: Add BLE button notifications (simulated presses, no pins wired)
4. **S3-4**: Wire physical buttons and test

**If Pi bluetooth degrades** (multiple failed connections), the updated `ble_receiver.py` now auto-restarts the bluetooth service after 3 consecutive failures. If that doesn't help, a Pi reboot clears everything.
