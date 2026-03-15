# BLE Remote Project — Progress Notes

## Session: 2026-03-15/16

### What we built
- **ESP32-C3 firmware** (`esp32/ble_remote/ble_remote.ino`): NimBLE GATT server advertising as "BLE-Remote" with heartbeat notifications every 2s
- **Pi receiver** (`pi/ble_receiver.py`): bleak-based scanner/connector with auto-reconnect, detection by UUID/MAC/name
- **Git-based workflow**: Two Claude instances (Mac + Pi) coordinating via `pi/TASK.md` and `pi/REPORT.md`

### What works
- ESP32 boots, initializes NimBLE, advertises, stays stable (heap ~202K, no crashes)
- Pi scanner detects the ESP32 intermittently (RSSI ~-88 dBm at 1 meter — weak signal from this Super Mini clone)
- Git coordination workflow between Mac-Claude and Pi-Claude works well

### What doesn't work yet
- **BLE connection never completed** — we got close but never received heartbeat notifications
- **ESP32 advertising is intermittent** — visible sometimes, invisible other times

### Bugs found and fixed (12 iterations)
1. **NimBLE 2.x API changes**: `setScanResponse` → `enableScanResponse(true)`, `setPower(enum)` → `setPower(20)` dBm
2. **BlueZ name caching**: old "EasyPlay" name cached, fixed by scanning by service UUID
3. **BR/EDR transport**: BlueZ tried classic BT instead of BLE, fixed with `service_uuids` filter + BLEDevice object
4. **BlueZ InProgress stuck state**: adapter reset (`bluetoothctl power off/on`) resolves it
5. **bluetoothctl remove timing**: removing cache between scan/connect destroyed D-Bus entry, moved to before scan
6. **setMinInterval/setMaxInterval broke NimBLE 2.x advertising** (Tests 6-8 — completely invisible)
7. **service_uuids D-Bus filter unreliable**: silently drops ESP32, removed in favor of callback-based matching
8. **NimBLE silently stops advertising**: added periodic re-start every 30s

### Current state (latest commit: `691b888`)
- ESP32 firmware: scan response enabled, periodic re-advertising every 30s, max TX power
- Pi receiver: unfiltered scan, matches by UUID/MAC/name, removes BlueZ cache before scan, auto adapter reset
- **Test 12** was just pushed — waiting for Pi-Claude to run it

### Next steps to try
1. **Check Test 12/13 results** — does periodic re-advertising fix the intermittency?
2. **If still intermittent**: the -88 dBm signal is concerning at 1 meter. Try moving them closer or using an ESP32 with a better antenna
3. **Connection timeout**: if device is found but connection times out, the issue may be BlueZ caching it with wrong address type. The `remove_bluez_device()` before scan should handle this, but may need more aggressive clearing
4. **User hint**: "restarting bluetooth on the Pi for each scan" worked before — if all else fails, add `sudo systemctl restart bluetooth` before each scan cycle
5. **Once connected**: verify heartbeat notifications flow, then move to button GPIO handling

### How to resume
1. Open terminal on Mac in `/Users/dkousemaker/Desktop/Remote_work`
2. `git pull` to get latest Pi reports
3. Read `pi/REPORT.md` for the latest test result
4. The ESP32 should still be connected via USB at `/dev/cu.usbmodem1101` — check with serial monitor
5. If ESP32 needs reflash: `arduino-cli compile --fqbn esp32:esp32:nologo_esp32c3_super_mini esp32/ble_remote/ && arduino-cli upload --fqbn esp32:esp32:nologo_esp32c3_super_mini --port /dev/cu.usbmodem1101 esp32/ble_remote/`
6. Start Pi-Claude and tell it to read `pi/TASK.md` and run the test
7. Continue the iteration loop: read report → fix code → push → wait for report

### Key technical details
- **ESP32 MAC**: `38:44:BE:45:AD:86`
- **Service UUID**: `4e520001-7354-4288-9a71-81a9bf56c4a8`
- **Char UUID**: `4e520002-7354-4288-9a71-81a9bf56c4a8`
- **NimBLE version**: 2.3.8 (2.x API — different from most online examples which target 1.x)
- **Board FQBN**: `esp32:esp32:nologo_esp32c3_super_mini`
- **Pi BlueZ**: 5.82, bleak 2.1.1
- **GitHub repo**: `davideo71/pi-ble-remote` (public)
