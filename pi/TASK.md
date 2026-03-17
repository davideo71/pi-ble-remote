# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. **Clear the BlueZ cache for the ESP32 device before testing:**
   ```bash
   sudo systemctl stop bluetooth
   sudo rm -rf /var/lib/bluetooth/*/38:44:BE:45:AD:84
   sudo rm -rf /var/lib/bluetooth/*/cache/38:44:BE:45:AD:84
   sudo systemctl start bluetooth
   ```
   If the MAC doesn't match, look for any BLE-Remote device in `/var/lib/bluetooth/` and remove it.
3. Run `python3 pi/ble_receiver.py` for about **2 minutes**
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 34c — cache cleared)
**No firmware changes.** Same interrupt-driven button firmware as Test 34/34b.

We suspect BlueZ is caching stale GATT service discovery data from earlier firmware versions. Every test since 32 has failed with "DISCONNECTED during service discovery" — even though the firmware kept changing. Tests 30-31 passed early in the session before the cache went stale.

**Clearing the cache is the only change.** If this passes, the cache was the root cause of ALL failures since Test 32.

## Incremental progress
| Test | What changed | RSSI | Result |
|------|-------------|------|--------|
| 30 | Heartbeat only | -83/-88 | PASS |
| 31 | + GPIO init | -84 | PASS |
| 32 | + digitalRead polling (5ms) | -81 | PARTIAL |
| 32b-34b | Various firmware changes | -85/-91 | ALL FAIL |
| **34c** | **Same as 34b + BlueZ cache cleared** | **?** | **?** |

## Expected
- If PASS: BlueZ cache was the problem — we need to clear cache whenever firmware changes
- If FAIL: something else is wrong and we need to dig deeper
