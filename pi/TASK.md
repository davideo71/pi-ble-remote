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

## How we collaborate

There are three of us working on this project:
- **Mac-Claude** (me): writes firmware, flashes the ESP32, updates TASK.md, pushes to git
- **Pi-Claude** (you): pulls from git, runs tests on the Pi, writes results to REPORT.md, pushes
- **User** (David): manages the hardware, moves things around, power cycles devices

### Workflow
1. Mac-Claude pushes firmware + TASK.md with test instructions
2. Pi-Claude polls `git pull`, sees the new TASK.md, runs the test
3. Pi-Claude writes results to REPORT.md, commits and pushes
4. Mac-Claude polls `git pull`, reads the report, decides next steps
5. Repeat

### What makes a good report
- **RSSI** — always include signal strength so we can track range issues
- **Uptime** — note how long since last Pi reboot (BlueZ degrades over many test cycles)
- **Number of connection attempts** — 1 attempt = clean state, 3+ = possible degradation
- **Heartbeat count + duration** — how long was the connection stable
- **Keep it concise** — results table at the top, details below

### Key lessons learned (from 36 tests on the C3)
- BlueZ on the Pi degrades after many connect/disconnect cycles — restarting bluetooth service or rebooting fixes it
- RSSI below -90 dBm is unreliable for BLE connections
- Always restart bluetooth service before a test if the previous test failed
- The `ble_receiver.py` now auto-recovers after 3 failures, but a fresh Pi reboot is the nuclear option
