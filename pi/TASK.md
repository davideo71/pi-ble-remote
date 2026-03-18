# Task for Pi-side Claude

## IMPORTANT: How we collaborate
- **Mac-Claude** (me) flashes the ESP32 and pushes firmware + this TASK.md
- **Pi-Claude** (you) pulls, runs the test, writes REPORT.md, commits and pushes
- Mac-Claude polls `git pull` every 2 minutes to check for your results
- Always commit and push REPORT.md when done, even if the test fails — Mac-Claude needs the results

## NEW TEST — Test 45: ble_receiver v2 + EasyPlay support (triggered 2026-03-18)

### Context — IMPORTANT BREAKTHROUGH
We discovered that **BlueZ cache was the root cause of MOST of our connection problems** across all tests. Here's what happened:

1. BlueZ aggressively caches device names, GATT tables, and connection parameters
2. Deleting cache files while bluetoothd is running is **useless** — the daemon regenerates them from its in-memory copy instantly
3. The ONLY way to clear the cache is: **stop bluetooth → delete files → restart bluetooth**
4. This is why Test 35 (after Pi reboot) worked perfectly, and everything after degraded
5. The "EasyPlay" ghost name was BlueZ cache all along (confirmed in Test 42)

### What changed
- **`ble_receiver.py` v2** — completely rewritten with:
  - `nuclear_cache_clear()`: stops bluetoothd, deletes cache, restarts. Used on startup and after 3 failures.
  - `remove_device()`: removes specific device from BlueZ before each connection (forces fresh GATT discovery)
  - Supports BOTH firmware profiles:
    - **BLE-Remote** (C3, Arduino/NimBLE) — custom UUID `4e520001...`, char `4e520002...`
    - **EasyPlay** (S3, MicroPython) — Nordic UART `6e400001...`, TX char `6e400003...`
  - Auto-detects which device is present and uses correct characteristic UUID

### What's running on the ESP32
An ESP32-S3 is currently plugged in running the **EasyPlay** MicroPython firmware.
- Device name: "EasyPlay"
- Service: Nordic UART Service (`6e400001-b5a3-f393-e0a9-e50e24dcca9e`)
- **CRITICAL: EasyPlay goes to sleep after 3 minutes of inactivity and STOPS advertising!**
- If you can't find it in a scan, the user will need to press a button on the remote to wake it
- During wake burst, it advertises at 20ms intervals for 10 seconds

### Steps
1. `git pull` (check `git log --oneline -3` to confirm new commit)
2. Run `python3 pi/ble_receiver.py` — it will:
   - Nuclear cache clear on startup (stop BT → delete cache → restart)
   - Scan for 15 seconds looking for "EasyPlay" or "BLE-Remote"
   - Auto-detect the device profile
   - Connect and subscribe to notifications
3. If device not found on first scan, it will retry automatically
4. Let it run for 2+ minutes to collect heartbeats or button events
5. **Commit and push REPORT.md** with results

### What to report
- Did the nuclear cache clear work? (check log output)
- Was the device found? Under what name? What RSSI?
- Did it connect? Did GATT service discovery complete?
- Were notifications received? (heartbeats from BLE-Remote, button chars from EasyPlay)
- If EasyPlay: it sends heartbeat-like pings too — report what you receive
- If connection failed: copy the exact error and the RSSI values

### About the Pi's BLE adapter
Previous tests used a CSR8510 USB dongle that showed very weak RSSI (-83 to -87 at 10cm). If possible:
- Check which adapter is active: `hciconfig -a`
- If both onboard and USB dongle exist, try the onboard one: pass `adapter="hci0"` or `adapter="hci1"` in the BleakScanner/BleakClient calls
- Report which adapter was used and RSSI values

### Success criteria
- Device found in scan (any name)
- Connected and received notifications (heartbeats or button events)
- No BlueZ cache-related errors
