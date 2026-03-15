# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Reset Bluetooth: `sudo systemctl restart bluetooth && sleep 2`
3. Run `python3 pi/ble_receiver.py` for about 60-90 seconds
4. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration
1. **ESP32 was dead/crashed** — it was not advertising at all during Tests 6 and 7. We just reset it via serial and confirmed it booted successfully.
2. **ESP32 confirmed running** via serial monitor: TX power 20 dBm, advertising interval 20-100ms, correct BLE address `38:44:be:45:ad:86`, heap stable at 202712 bytes.
3. **No code changes** — same firmware and receiver code as last round.

## Expected
- Device should now be visible (it was genuinely offline before, not a scanning issue)
- Fast advertising (20-100ms) should make it appear in nearly every scan cycle
- If found → connect → heartbeat notifications should flow
- If still not found: try an unfiltered scan (without service_uuids) to see if it shows up at all, and report what devices you do see
