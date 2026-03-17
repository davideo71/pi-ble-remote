# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 39: BLE firmware confirmed running (triggered 2026-03-17 21:35)

### Context
We confirmed the C3 IS running code (blink test worked, LED blinks). Serial output is broken after full flash erase (USB-JTAG config lost), but the app runs. Just reflashed BLE heartbeat firmware. The device should now advertise as **"BLE-Remote"** (NOT "EasyPlay").

BLE MAC is likely `38:44:BE:45:AD:86` (base MAC +2).

### Steps
1. `git pull`
2. `sudo systemctl restart bluetooth && sleep 3`
3. Broad BLE scan — look for "BLE-Remote"
4. If found — connect with `ble_receiver.py` (update KNOWN_MAC if needed) and run for 2 minutes
5. If "EasyPlay" appears instead of "BLE-Remote" — try connecting to it anyway since it has our service UUID
6. **Commit and push REPORT.md** — Mac-Claude is waiting!

### Success criteria
- Device name changed from "EasyPlay" to "BLE-Remote"
- Connected and received 30+ heartbeats
