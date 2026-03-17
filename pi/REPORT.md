# Pi Test Report: Step 2 — Button Handling

## Test 41b — 2026-03-17 23:00 UTC (full 4MB merged binary flash)

**Pi state:** BT restarted, cache cleared, device removed

### Result: STILL "EasyPlay" — even after full 4MB flash!

```
  MAC:  38:44:BE:45:AD:86
  Name from BlueZ:    EasyPlay
  Name from adv data: EasyPlay   <-- STILL EASYPLAY!
  RSSI: -85 dBm
  Service UUIDs: ['4e520001-7354-4288-9a71-81a9bf56c4a8']
```

#### This should be IMPOSSIBLE
The entire 4MB flash was erased to 0xFF and rewritten with a merged binary. There is no NVS, no OTA partition, no cached data — every byte was overwritten. Yet "EasyPlay" persists.

#### The "EasyPlay" string must be IN the merged binary itself

**Check the binary for "EasyPlay":**
```bash
strings merged.bin | grep -i easy
xxd merged.bin | grep -i "4561 7379"  # hex for "Easy"
```

If "EasyPlay" is found in the binary, it means:
1. **Arduino IDE build cache** has a stale compiled object with "EasyPlay"
2. Or a **library** (NimBLE-Arduino?) has "EasyPlay" hardcoded or cached
3. Or the `.ino` file being compiled is NOT the one in the git repo (stale copy in Arduino sketchbook?)

#### Fix: Clean build
1. **Arduino IDE**: Sketch → Clean Build Files (or delete the build cache directory)
2. Verify the `.ino` file path — is Arduino compiling from the git repo or a copy?
3. After clean build, `strings firmware.bin | grep -i easy` to verify "EasyPlay" is gone BEFORE flashing
4. Then flash and test again

---

## Test 41 — 2026-03-17 22:39 UTC (clean BLE init, no nvs_flash_erase)

**Duration:** 2 minutes (timeout)
**Pi state:** BT restarted, cache cleared, device removed

### Result: FAIL — still "EasyPlay" in raw adv, connection still times out

#### Raw scan (pre-test)
```
  Name from BlueZ:    EasyPlay
  Name from adv data: EasyPlay   <-- NOT BLE-Remote
  RSSI: -87 dBm
```

#### ble_receiver.py: 2 connection attempts, both timeout. 0 heartbeats.
- Found by UUID match twice (RSSI -83 to -87)
- Multiple scans where device wasn't found at all (intermittent advertising)
- Connection timeout every time

#### The firmware is NOT being updated on this board

The name "EasyPlay" persists in the raw advertisement data despite:
- Full flash erase + reflash
- OTA/NVS/app1 partition erases
- nvs_flash_erase() in code (which killed BLE entirely)
- Removing nvs_flash_erase() and reflashing again

**The only firmware change that was visible was nvs_flash_erase() killing BLE.** Every other flash attempt has had zero effect on the BLE name — it's always "EasyPlay".

#### Theory: OTA bootloader is loading app1, not app0

NimBLE stores the device name in NVS. The external NVS erase + OTA data erase should have forced boot from app0. But:
- If the bootloader is configured for factory+OTA scheme, erasing OTA data boots from **factory**, not app0
- The factory partition might still have old "EasyPlay" firmware
- Our code might be going into a different slot than expected

#### Alternative theory: Arduino IDE is uploading to the wrong partition

The Arduino IDE might be uploading to a partition that the bootloader doesn't boot from. The LED blink code runs (proving SOME code executes), but BLE uses the name from a different partition's firmware.

#### Suggestion for Mac-Claude
1. **Check partition table**: `esptool read_flash` or `parttool.py` to see the actual partition layout
2. **Try `esptool write_flash 0x0`** to overwrite from the very start (bootloader + all)
3. **Or flash with a merged binary** that includes bootloader + partition table + app, ensuring everything is overwritten
4. Consider using `esptool --chip esp32c3 write_flash 0x10000 firmware.bin` to explicitly target the app0 offset

---

## Test 40c — 2026-03-17 22:50 UTC (raw advertisement scan)

**Pi state:** BT restarted, cache cleared, device removed from bluetoothctl

### Result: "EasyPlay" is in the RAW advertisement — NOT a BlueZ cache issue

```
  MAC:  38:44:BE:45:AD:86
  Name from BlueZ:    EasyPlay
  Name from adv data: EasyPlay     <-- SAME! Not cached.
  RSSI: -88 dBm
  Service UUIDs: ['4e520001-7354-4288-9a71-81a9bf56c4a8']
  Manufacturer data: {}
  Service data: {}
  TX power: None
```

#### Analysis
- **BlueZ name and advertisement name are identical**: "EasyPlay"
- This is NOT a BlueZ caching problem — the device genuinely advertises "EasyPlay" in its BLE packets
- The firmware on the C3 is using "EasyPlay" as the device name in `NimBLEDevice::init()` or equivalent

#### Wait — but earlier the NVS erase firmware showed NO advertising?
When we tested with the `nvs_flash_erase()` firmware, the C3 had no BLE advertising at all (LED blinked but BLE was dead). Now it's back to advertising as "EasyPlay". This means either:
1. **Mac-Claude reflashed** without `nvs_flash_erase()` (removing the BLE-killing code), but the name is still "EasyPlay" in the firmware source
2. Or the board was power-cycled and NVS recovered

#### Conclusion
**The firmware source code has "EasyPlay" as the device name.** Check the `.ino` file — look for the string passed to `NimBLEDevice::init()` or the advertising name setter. It needs to be changed to "BLE-Remote".

---

## Test 40b — 2026-03-17 22:40 UTC (unplug test + NVS erase kills BLE)

### Key findings

1. **Unplugged C3 → "EasyPlay" disappeared from scan.** Confirms the C3 IS the "EasyPlay" device.
2. **Plugged C3 back in (with nvs_flash_erase firmware) → LED blinks but NO BLE advertising at all.** Multiple scans over 2+ minutes, device never appeared.
3. **This proves the latest firmware IS running on the board.** The flash was working all along — the old "EasyPlay" name was from the previous firmware in the OTA partition.
4. **`nvs_flash_erase()` is killing BLE.** NimBLE stores its configuration in NVS. Erasing NVS on every boot wipes NimBLE's required state, so BLE init fails silently (no crash, LED still blinks, but no advertising).

### Fix needed
**Remove `nvs_flash_erase()` from setup()** and reflash. The firmware is confirmed running on this board now. Without the NVS nuke, NimBLE should initialize normally and advertise as "BLE-Remote".

---

## Test 40 — 2026-03-17 22:25 UTC (NVS erase in firmware)

**Duration:** 2 minutes (timeout)
**Pi state:** BT restarted, BlueZ cache cleared, device removed from bluetoothctl

### Result: FAIL — STILL "EasyPlay", connection still times out

#### Device name: STILL "EasyPlay"
Even with `nvs_flash_erase()` in firmware setup(), the device at `38:44:BE:45:AD:86` still advertises as "EasyPlay".

#### Scan result (pre-test)
```
  38:44:BE:45:AD:86  RSSI=-81  name=EasyPlay  UUIDs=['4e520001-...']  <-- STILL EASYPLAY
  (+ 6 other neighborhood devices)
```

#### Connection: 2 attempts, both timeout at 15s. 0 heartbeats.

#### This confirms: the firmware being flashed is NOT running on this device

The "EasyPlay" name has now survived:
1. `esptool erase_flash` (full flash erase)
2. OTA data partition erase
3. NVS partition erase
4. App1 partition erase
5. `nvs_flash_erase()` in firmware code
6. Multiple reflashes

**This is impossible if the erase/flash operations are targeting this device.** The device at `38:44:BE:45:AD:86` is running completely different firmware than what's being flashed.

#### Most likely explanation
There are **two ESP32 boards**:
- **Board A** (being flashed via USB): This is the C3 that Mac-Claude is erasing/flashing. Its LED blinks. But its BLE may not be working, or it has a different MAC.
- **Board B** (`38:44:BE:45:AD:86`, "EasyPlay"): This is a DIFFERENT board still powered on somewhere, running old EasyPlay firmware. It has our service UUID because it was previously flashed with our code.

#### Definitive test
**Power off ALL ESP32 boards**, then scan from Pi:
- If "EasyPlay" disappears → it IS one of our boards, just not the one being flashed
- If "EasyPlay" remains → it's a neighbor's device (unlikely given our service UUID)

---

## Test 39b — 2026-03-17 22:12 UTC (after OTA+NVS+app1 erase)

**Duration:** 2 minutes (timeout)
**Pi state:** BT restarted, BlueZ cache dir cleared

### Result: FAIL — STILL "EasyPlay", connection still times out

#### Device name: STILL "EasyPlay" (not "BLE-Remote")
- The OTA+NVS+app1 erase did NOT change the advertised name
- MAC: `38:44:BE:45:AD:86`, Service UUID: correct
- RSSI: -84 to -88 dBm

#### Connection attempts (3 found + 4 not found over 2 minutes)

| Attempt | Time | Result |
|---------|------|--------|
| 1 | 22:12:42 | Found by UUID, RSSI -84. Timeout at 15s |
| 2 | 22:13:02 | Scan timeout — 47 devices, not found |
| 3 | 22:13:19 | Found by UUID, RSSI -88. Timeout at 15s |
| (recovery) | 22:13:35 | BT restart + cache clear |
| 4-5 | 22:13:43, 22:13:56 | Scan timeouts — 41, 43 devices |
| 6 | 22:14:10 | Scan timeout — 54 devices |
| (recovery) | 22:14:20 | BT restart + cache clear |
| 7 | 22:14:30 | Found by UUID, RSSI -88. Connecting... (test ended) |

#### Heartbeats: NONE

#### Critical question
**Is there a SECOND ESP32 board still powered on?** The "EasyPlay" name has persisted through:
- Full flash erase (`esptool erase_flash`)
- OTA partition erase
- NVS partition erase
- App1 partition erase
- Multiple reflashes

This is very unusual. If ALL partitions were erased on the correct board, the NVS-stored BLE name "EasyPlay" should be gone. Possibilities:
1. **A different board** is advertising "EasyPlay" — there may be an old ESP32 still powered somewhere nearby
2. **The erase is targeting the wrong serial port** — the C3 being flashed is not the one advertising
3. **The C3 has a second flash chip or protected region** (very unlikely for standard C3)

#### Suggestion for Mac-Claude
- **Power off ALL ESP32 boards**, then scan from Pi — if "EasyPlay" still appears, it's a neighbor's device
- If "EasyPlay" disappears, power on boards one at a time to find which one is "EasyPlay"
- Check `esptool chip_id` output — does the MAC in the output match `38:44:BE:45:AD:84`?

---

## Test 39 — 2026-03-17 21:44 UTC (BLE firmware confirmed running)

**Duration:** 2 minutes (timeout)
**Pi state:** Bluetooth service restarted, KNOWN_MAC updated to `38:44:BE:45:AD:86`

### Result: FAIL — still "EasyPlay", connection always times out

#### Device name: NOT changed
- Device still advertises as **"EasyPlay"** (not "BLE-Remote")
- MAC: `38:44:BE:45:AD:86`
- Service UUID: correct (`4e520001-...`)
- RSSI: -87 to -91 dBm

#### Connection attempts (6 over 2 minutes)

| Attempt | Time | Found by | RSSI | Result |
|---------|------|----------|------|--------|
| 1 | 21:45:03 | UUID | -87 | Disconnected at 1s, timeout at 15s |
| 2 | 21:45:25 | UUID | -87 | Disconnected at 6s, timeout at 15s |
| 3 | 21:45:48 | MAC | -91 | Disconnected at 5s, timeout at 15s |
| (recovery) | 21:46:04 | — | — | BT restart + cache clear |
| 4 | 21:46:13 | MAC | -87 | Disconnected at 8s, timeout at 15s |
| 5 | 21:46:34 | — | — | Scan timeout, not found (44 devices) |
| 6 | 21:46:47 | — | — | Scan timeout, not found (41 devices) |

#### Heartbeats: NONE

#### Key observations
1. **Name still "EasyPlay"** — the flash erase + reflash did NOT change the device name
2. **Connection pattern**: BlueZ reports DISCONNECTED 1-8 seconds into the connection attempt, then times out waiting for GATT services. This is consistent across all tests since Test 36.
3. **RSSI -87 to -91 dBm** — weaker than Test 38c (-83 dBm). Signal marginal.
4. **Same failure as Tests 36b, 37, 38b, 38c** — the C3 can be discovered but never completes GATT connection

#### Comparison: What worked vs what doesn't

| Test | Device name | RSSI | Connection |
|------|------------|------|------------|
| **35 (worked!)** | **BLE-Remote** | **-89** | **PASS — 1st attempt, 89 heartbeats** |
| 36b | EasyPlay | -83 | FAIL — all timeouts |
| 37 | EasyPlay | -83 | FAIL — scan only |
| 38c | EasyPlay | -83 | FAIL — scan only |
| **39** | **EasyPlay** | **-87 to -91** | **FAIL — all timeouts** |

**Test 35 was the ONLY test that worked, and it used firmware advertising as "BLE-Remote".** Every test with "EasyPlay" firmware has failed to connect.

#### Conclusion
The firmware on the C3 has NOT been successfully updated — it's still running the old "EasyPlay" firmware. This old firmware can advertise but cannot complete GATT connections from this distance. The flash erase + reflash either:
1. Targeted the wrong board/port
2. Didn't actually complete successfully
3. There's a different ESP32 still running "EasyPlay"

**The working Test 35 firmware ("BLE-Remote") needs to be successfully flashed to this C3.**

---

## Test 38c — 2026-03-17 21:38 UTC (quick scan after erase+reflash)

**Duration:** 1 broad scan × 15 seconds

### Result: FAIL — still "EasyPlay", no "BLE-Remote"

#### All devices found
```
  38:44:BE:45:AD:86  RSSI=-83  name=EasyPlay  UUIDs=['4e520001-...']  <-- STILL OLD NAME
  69:58:F5:64:27:04  RSSI=-84  name=S41 152A LE
  7E:5B:39:26:13:D5  RSSI=-85  name=None
  A8:51:AB:8F:DE:49  RSSI=-88  name=None
  94:E6:BA:80:A1:6B  RSSI=-88  name=IAe-65" The Frame
  5C:5A:2F:BC:25:07  RSSI=-88  name=None
  67:B3:A0:41:0E:07  RSSI=-89  name=LE-My oaces of quiet
  6A:81:E4:03:F6:3A  RSSI=-90  name=None
  CE:D1:40:53:ED:10  RSSI=-90  name=None
  Total: 9 devices
```

**No "BLE-Remote" found.** The C3 at `38:44:BE:45:AD:86` is still advertising as "EasyPlay" with the correct service UUID.

#### Theory
The flash erase + reflash is not taking effect on this board. The old firmware persists. Possible causes:
- Wrong serial port being used for flash
- ESP32-C3 has a second flash partition that wasn't erased
- There may be a **different ESP32 board** (not the C3 being flashed) that's still powered on and advertising "EasyPlay"

---

## Test 38b — 2026-03-17 21:18 UTC (rescan after C3 reset)

**Duration:** 2 broad scans × 15 seconds each
**Pi state:** Bluetooth service restarted before test

### Result: FAIL — still "EasyPlay", NOT "BLE-Remote"

#### Scan 1: No ESP32 device (6 devices, all neighborhood)
```
  57:AC:F8:A3:75:AD  RSSI=-82  None
  D0:7D:37:93:5F:02  RSSI=-85  None
  6D:30:69:B4:8D:C1  RSSI=-85  S41 152A LE
  94:E6:BA:80:A1:6B  RSSI=-88  IAe-65" The Frame
  A8:51:AB:8F:DE:49  RSSI=-89  None
  7C:BB:C5:61:4A:E5  RSSI=-89  None
```

#### Scan 2: "EasyPlay" reappears
```
  D0:7D:37:93:5F:02  RSSI=-82  None
  94:E6:BA:80:A1:6B  RSSI=-84  IAe-65" The Frame
  A8:51:AB:8F:DE:49  RSSI=-85  None
  57:AC:F8:A3:75:AD  RSSI=-86  None
  38:44:BE:45:AD:86  RSSI=-88  EasyPlay  UUIDs=['4e520001-...']  <-- STILL EASYPLAY
  7C:BB:C5:61:4A:E5  RSSI=-89  None
  6D:30:69:B4:8D:C1  RSSI=-89  S41 152A LE
```

#### Conclusion
**The flash erase + reflash did NOT change the device name.** The C3 still advertises as "EasyPlay" at `38:44:BE:45:AD:86` with the correct service UUID.

Possible explanations:
1. **The flash didn't actually get erased/reflashed** — maybe the wrong USB port or board was targeted
2. **There's a second ESP32 nearby** — an old board still powered on advertising "EasyPlay"?
3. **The firmware source code still has "EasyPlay"** — check the `NimBLEDevice::init()` call in the .ino file

The device was absent in scan 1 and appeared in scan 2, which is consistent with the intermittent advertising we've seen before — this is the same C3 board.

#### Suggestion for Mac-Claude
- **Check the firmware source**: `grep -i "easyplay\|init(" esp32/ble_remote/ble_remote.ino`
- Is there a second ESP32 board that could be advertising "EasyPlay"?
- Verify esptool targeted the correct serial port during erase/flash
- Try monitoring serial output after flash to confirm the new firmware is running

---

## Test 38 — 2026-03-17 21:05 UTC (C3 after full flash erase + reflash)

**Duration:** 3 broad scans × 15 seconds each
**Pi state:** Bluetooth service restarted before test
**Goal:** Find "BLE-Remote" (not "EasyPlay") after esptool erase_flash + reflash

### Result: FAIL — ESP32-C3 not advertising at all

#### Scan results (3 scans, 15s each)

All 3 scans returned the same 6 neighborhood devices. **No ESP32 device found:**
- No "BLE-Remote"
- No "EasyPlay"
- No `38:44:BE:45:AD:84`
- No `38:44:BE:45:AD:86`

```
Scan 1:  94:E6:BA:80:A1:6B  RSSI=-84  IAe-65" The Frame
         A8:51:AB:8F:DE:49  RSSI=-85  None
         7C:BB:C5:61:4A:E5  RSSI=-85  None
         DC:62:09:47:89:54  RSSI=-88  None
         4B:77:48:8D:C8:FC  RSSI=-89  S41 152A LE
         57:AC:F8:A3:75:AD  RSSI=-90  None

Scan 2:  DC:62:09:47:89:54  RSSI=-81  None
         94:E6:BA:80:A1:6B  RSSI=-84  IAe-65" The Frame
         A8:51:AB:8F:DE:49  RSSI=-85  None
         7C:BB:C5:61:4A:E5  RSSI=-85  None
         4B:77:48:8D:C8:FC  RSSI=-89  S41 152A LE
         57:AC:F8:A3:75:AD  RSSI=-90  None

Scan 3:  Same 6 devices, no ESP32
```

Pi's BLE adapter is working fine (6 other devices consistently visible).

#### Analysis
The flash erase + reflash appears to have left the ESP32-C3 in a non-advertising state. Possible causes:
1. **Reflash didn't complete** — esptool may have erased but the upload failed or was incomplete
2. **ESP32 in boot loop** — firmware crash before `NimBLEDevice::init()` completes
3. **ESP32 not powered** — battery disconnected during flash process?
4. **Firmware compiling with wrong board** — if compiled for wrong target, BLE init could silently fail

#### Suggestion for Mac-Claude
- Check ESP32 serial output — is it printing anything? Boot messages? Crash trace?
- Verify the upload completed successfully (esptool should show "Leaving... Hard resetting")
- Try a simple blink sketch first to confirm the board is alive
- If serial shows nothing, the board may need a manual reset (hold BOOT, press RST, release BOOT)

---

## Test 37 — 2026-03-17 20:40 UTC (C3 with antenna, clean slate — find "BLE-Remote")

**Duration:** 3 broad scans × 15 seconds each
**Pi state:** Fresh BLE cache clear (`rm -rf /var/lib/bluetooth/*/`), bluetooth restarted
**Goal:** Find device named "BLE-Remote" at MAC `38:44:BE:45:AD:84`

### Result: FAIL — "BLE-Remote" NOT found. "EasyPlay" still present.

#### Cache clearing
- Stopped bluetooth, deleted all `/var/lib/bluetooth/*/`, restarted, waited 5s
- Confirmed clean slate — first scan showed no cached names

#### Scan results (3 scans, 15s each)

**Scan 1** (immediately after cache clear): 6 devices, NO "BLE-Remote", NO "EasyPlay"
```
  E4:E2:7D:E3:72:F2  RSSI= -81  name=None
  94:E6:BA:80:A1:6B  RSSI= -84  name=IAe-65" The Frame
  44:DA:21:14:E5:F0  RSSI= -85  name=S41 152A LE
  A8:51:AB:8F:DE:49  RSSI= -85  name=None
  66:32:47:D7:EC:AA  RSSI= -89  name=None
  6E:1A:95:A6:DF:FC  RSSI= -89  name=None
```

**Scan 2**: 7 devices — "EasyPlay" appears at `38:44:BE:45:AD:86`
```
  44:DA:21:14:E5:F0  RSSI= -81  name=S41 152A LE
  6E:1A:95:A6:DF:FC  RSSI= -81  name=None
  38:44:BE:45:AD:86  RSSI= -83  name=EasyPlay  UUIDs=['4e520001-...']  <-- WRONG NAME
  94:E6:BA:80:A1:6B  RSSI= -84  name=IAe-65" The Frame
  66:32:47:D7:EC:AA  RSSI= -85  name=None
  E4:E2:7D:E3:72:F2  RSSI= -86  name=None
  A8:51:AB:8F:DE:49  RSSI= -89  name=None
```

**Scan 3**: Same — "EasyPlay" at `38:44:BE:45:AD:86`, RSSI -83
```
  38:44:BE:45:AD:86  RSSI= -83  name=EasyPlay  UUIDs=['4e520001-...']  <-- WRONG NAME
  6E:1A:95:A6:DF:FC  RSSI= -84  name=None
  44:DA:21:14:E5:F0  RSSI= -85  name=S41 152A LE
  E4:E2:7D:E3:72:F2  RSSI= -86  name=None
  94:E6:BA:80:A1:6B  RSSI= -88  name=IAe-65" The Frame
  66:32:47:D7:EC:AA  RSSI= -89  name=None
  A8:51:AB:8F:DE:49  RSSI= -89  name=None
```

#### Key findings
1. **"BLE-Remote" does NOT exist** — not found in any of 3 scans after full cache clear
2. **"EasyPlay" at `38:44:BE:45:AD:86`** is the only ESP32 visible — with the correct service UUID
3. **MAC is `:AD:86`**, not `:AD:84` as expected in TASK.md
4. **The device name "EasyPlay" is coming from the firmware**, not from BlueZ cache — confirmed because cache was fully cleared and device still advertises as "EasyPlay"
5. RSSI: -83 dBm consistently

#### Conclusion
The ESP32-C3 firmware is advertising as **"EasyPlay"**, not **"BLE-Remote"**. This is NOT a BlueZ caching issue — the cache was completely wiped and the device still shows up as "EasyPlay". The firmware's `NimBLEDevice::init("...")` call likely has "EasyPlay" as the device name.

Also the MAC `38:44:BE:45:AD:86` differs from the expected `38:44:BE:45:AD:84` — please verify which board/MAC is correct.

#### Suggestion for Mac-Claude
- Check the firmware source: is `NimBLEDevice::init()` using "BLE-Remote" or "EasyPlay"?
- Verify the ESP32-C3 board MAC — is it actually `...AD:84` or `...AD:86`?
- Re-flash if needed, then push a new TASK.md and I'll re-test

---

## Test 36b — 2026-03-17 20:12 UTC (C3 with external antenna, fresh Pi reboot)

**Duration:** 2 minutes (timeout)
**Pi state:** Fresh reboot, bluetooth service restarted before test
**Firmware:** C3 with external antenna — device advertises as "EasyPlay"

### Result: FAIL — device found but all connections time out

#### Discovery: PASS
- Device found on **every scan** within 1–3 seconds
- MAC: `38:44:BE:45:AD:86` (note: task expected `38:44:BE:45:AD:84` — off by 2)
- Device name: **"EasyPlay"** (not "BLE-Remote")
- Matched by UUID on first scan, by name on subsequent scans
- RSSI: consistently **-83 dBm** (slight improvement over previous -89 dBm)

#### Connection attempts

| Attempt | Time | Found by | RSSI | Result |
|---------|------|----------|------|--------|
| 1 | 20:12:16 | UUID | -83 | Disconnected at 2s, then timeout at 15s |
| 2 | 20:12:37 | name | -83 | Timeout at 15s |
| 3 | 20:12:59 | name | -83 | Timeout at 15s |
| (recovery) | 20:13:19 | — | — | BT service restarted, BlueZ cache cleared |
| 4 | 20:13:23 | — | — | Scan: 40 devices, target NOT found |
| 5 | 20:13:36 | — | — | Scan: 51 devices, target NOT found |
| 6 | 20:13:51 | name | -88 | Started connecting... (test ended at 2min timeout) |

#### Heartbeats: NONE
- Never got past connection phase

#### Broad scan (after test)
```
  94:E6:BA:80:A1:6B  RSSI= -80  name=IAe-65" The Frame
  58:8F:8E:22:B2:1C  RSSI= -82  name=None
  38:44:BE:45:AD:86  RSSI= -83  name=EasyPlay
  49:F1:89:C1:25:3B  RSSI= -85  name=None
  D9:D2:94:31:79:DA  RSSI= -85  name=None
  A8:51:AB:8F:DE:49  RSSI= -89  name=None
  75:14:BC:5F:0C:A1  RSSI= -89  name=S41 152A LE
```
Device clearly visible at -83 dBm. Discovery is NOT the problem.

#### Analysis
1. **Discovery works perfectly** — ESP32-C3 advertising found quickly every time
2. **Connection consistently fails** — immediate disconnect or timeout on every attempt
3. **RSSI -83 dBm** — slight improvement over previous -89 dBm, but still fails
4. **MAC is `38:44:BE:45:AD:86`** not `38:44:BE:45:AD:84` as specified in TASK.md
5. **Device name "EasyPlay"** not "BLE-Remote" — firmware name may not have been updated
6. **After recovery (BT restart + cache clear)**, device temporarily disappeared from scans for 2 cycles then reappeared at weaker -88 dBm

#### Comparison with previous tests

| Test | Pi State | RSSI | Result |
|------|----------|------|--------|
| 35 (close, 11:01) | Fresh reboot | -89 dBm | **PASS** — first attempt, 89 heartbeats |
| 35c (far, no WiFi) | Same session | -88 to -91 dBm | FAIL — found but can't connect |
| 36 (11:47) | Fresh reboot | -86 to -90 dBm | FAIL — found but can't connect |
| **36b (20:12)** | **Fresh reboot** | **-83 dBm** | **FAIL — found but can't connect** |

#### Conclusion
Despite fresh Pi reboot, antenna addition, and slightly improved RSSI (-83 vs -89), connection still fails. Since Test 35 connected at -89 dBm from close range, and Test 36b fails at -83 dBm, **the problem is unlikely to be signal strength alone**. Something may have changed in the firmware flashed for this test, or there's a connection-level incompatibility.

#### Suggestion for Mac-Claude
- Try `bluetoothctl connect 38:44:BE:45:AD:86` directly to isolate bleak vs BlueZ
- Check the ESP32 serial output during connection attempts — is it seeing the connection request?
- Consider reverting to the exact firmware from Test 35 (which worked) to confirm it's not a firmware regression
- The device name "EasyPlay" and MAC `...AD:86` suggest this may be the same firmware as before the antenna mod

---

## Test S3-1b — 2026-03-17 12:35 UTC (broad scan to find ESP32-S3 actual MAC)

**Duration:** 3 broad scans × 15 seconds each
**Uptime at start:** ~50 minutes
**Bluetooth:** Service restarted before scanning

### Result: FAIL — "BLE-Remote" not found in any scan

#### Scan results
Ran 3 consecutive 15-second active scans. Same 5 devices appeared each time:

| Address | RSSI | Name | UUIDs |
|---------|------|------|-------|
| 7A:CA:CD:E6:10:69 | -81 | None | — |
| EE:E7:90:F8:20:70 | -81 to -87 | None | — |
| 73:62:35:37:8F:66 | -84 to -88 | None | — |
| 7C:39:D0:0F:D1:64 | -88 | None | — |
| 73:64:F7:50:AB:46 | -90 to -91 | S41 152A LE | 0000fe07-... |

- **No device named "BLE-Remote"**
- **No device with service UUID `4e520001-...`**
- **MAC `A0:F2:62:EC:7A:D0` not seen**
- The old ESP32-C3 "EasyPlay" (38:44:BE:45:AD:86) also not seen

#### Conclusion
The ESP32-S3 is not advertising at all. The Pi's Bluetooth adapter is working fine (5 other devices consistently visible). Possible causes:
1. **ESP32-S3 not powered on** or in reset loop
2. **Firmware didn't flash correctly** — upload may have succeeded but BLE init may be failing silently
3. **NimBLE not starting** — if there's a crash before `NimBLEDevice::init()`, no BLE advertising would occur

#### Suggestion
- Check if ESP32-S3 has power LED lit
- Try connecting via serial to see if there's any output (even boot messages)
- Re-flash and verify with a simple blink sketch first to confirm the board works

---

## Test S3-1 — 2026-03-17 12:02 UTC (new ESP32-S3 SuperMini — heartbeat baseline)

**Duration:** 2 minutes (timeout)
**Uptime at start:** ~17 minutes (rebooted earlier this session)
**Hardware:** ESP32-S3 SuperMini (new), MAC `A0:F2:62:EC:7A:D0`

### Result: FAIL — device never found in scans

#### Details
- **7 full scan cycles** over 2 minutes, ~230+ devices seen
- MAC `A0:F2:62:EC:7A:D0` and name "BLE-Remote" never appeared in any scan
- Other BLE devices visible: "S41 152A LE" (-82 to -91 dBm), various unknowns (-77 to -90 dBm)
- Recovery triggered twice (bluetooth service restart after 3 consecutive failures) — no help
- Old ESP32-C3 "EasyPlay" (38:44:BE:45:AD:86) also not seen — may be powered off

#### Possible causes
1. **ESP32-S3 not powered on or not advertising** — most likely
2. **Out of BLE range** — but other devices are visible, so the Pi's adapter is working
3. **Firmware issue** — device may not have been flashed successfully
4. **Wrong MAC** — the actual MAC may differ from what was expected

#### Recommendation
- Verify the ESP32-S3 is powered on and LED is blinking (if firmware has an indicator)
- Check Serial Monitor on the Mac to confirm the ESP32-S3 is advertising
- Move ESP32-S3 closer to the Pi if range is uncertain

---

## Test 36 — 2026-03-17 11:47 UTC (fresh reboot + BT restart + improved recovery)

**Duration:** ~2 minutes (timeout)
**Uptime at start:** 2 minutes (freshly rebooted)
**Firmware:** Same as 35b/35c — device advertises as "EasyPlay" (not "BLE-Remote")

### Result: FAIL — device found but all connections time out

#### Bug fix applied
`ble_receiver.py` had an `UnboundLocalError` on `connected_address` at line 336. The variable was never initialized in `main()` and the `global` declaration was missing. Fixed by adding `global connected_address` and initializing to `None`.

#### Connection attempts
- **Attempt 1:** Found "EasyPlay" at -90 dBm after scanning 3 devices. Disconnected at 5s, timed out at 15s.
- **Attempt 2:** Found "EasyPlay" at -86 dBm after 9 devices. Timed out at 15s.
- **Attempt 3:** Scan timeout — 37 devices seen, target not found. Recovery triggered (BT service restart).
- **Attempt 4:** Found "EasyPlay" at -90 dBm after 14 devices. Timed out at 15s.
- **Attempt 5:** Found "EasyPlay" at -90 dBm after 7 devices. Timed out at 15s (test ended at timeout).

#### RSSI
- **-86 to -90 dBm** — weak but consistently found during scans

#### Key observations
1. **Pi was freshly rebooted** — clean BlueZ state, so degradation is not the issue this time
2. **Device name is "EasyPlay"** not "BLE-Remote" — still matched via KNOWN_MAC fallback
3. **UUIDs=[]** — device not advertising the expected service UUID
4. **Scanning works, connecting doesn't** — same pattern as Test 35c
5. RSSI is comparable to Test 35 (-89 dBm, which connected) but connection never establishes

#### RSSI comparison
| Test | Pi State | RSSI | Result |
|------|----------|------|--------|
| Test 35 (close) | Fresh reboot | -89 dBm | PASS — first attempt, 89 heartbeats |
| Test 35b (far) | Same session | not seen | FAIL — out of range |
| Test 35c (far, no WiFi) | Same session | -88 to -91 dBm | FAIL — found but can't connect |
| **Test 36** | **Fresh reboot** | **-86 to -90 dBm** | **FAIL — found but can't connect** |

#### Conclusion
The ESP32 ("EasyPlay" is the correct device name) is visible in scans (-86 to -90 dBm) but connection never establishes. Since the Pi was freshly rebooted, BlueZ state degradation is ruled out. The issue is likely:
1. **Signal is marginal** — -86 to -90 dBm is borderline for reliable GATT connections. Test 35 connected at -89 dBm but was very close to the limit.
2. **No service UUID advertised** — UUIDs=[] on scans, so receiver falls back to MAC match only. The `ble_receiver.py` name check for "BLE-Remote" also won't match — needs updating to "EasyPlay" or just relying on MAC.
3. **ESP32 may need to be moved closer** for reliable connections.

---

## Test 35c — 2026-03-17 11:34 UTC (range test retry — WiFi disabled)

**Duration:** ~5 minutes
**Firmware:** New firmware — device now advertises as "EasyPlay" (was "BLE-Remote")
**Pi state:** Same session, WiFi disabled via rfkill, onboard BT already disabled. USB dongle (CSR8510) only.

### Result: FAIL — device found but every connection times out

#### Details
- **7 connection attempts**, all timed out after 15s
- Device found by MAC address match (name changed to "EasyPlay", receiver expects "BLE-Remote")
- RSSI: **-88 to -91 dBm** — right at the noise floor
- Disabling WiFi made no difference vs Test 35b

#### Key observations
1. **Device name changed** — firmware now advertises as "EasyPlay" instead of "BLE-Remote". Receiver matches by MAC fallback but UUID match also fails (UUIDs=[] on most scans)
2. **Scanning works, connecting doesn't** — at -91 dBm the signal is strong enough for discovery but too weak for the multi-packet GATT connection handshake
3. **WiFi interference is not the issue** — same failure pattern with WiFi off

#### RSSI comparison
| Test | RSSI | Result |
|------|------|--------|
| Test 35 (close) | -89 dBm | PASS — first attempt, 89 heartbeats |
| Test 35b (far) | not seen | FAIL — completely out of range |
| Test 35c (far, no WiFi) | -88 to -91 dBm | FAIL — found but can't connect |

#### Conclusion
ESP32 is too far away. Needs to be moved closer. The -89 dBm from Test 35 was already marginal but worked; -91 dBm does not. Also: firmware needs to either keep advertising as "BLE-Remote" or `ble_receiver.py` needs updating to match "EasyPlay".

---

## Test 35b — 2026-03-17 11:17 UTC (range test — ESP32 moved further away)

**Duration:** ~4 minutes (15 scan attempts)
**Firmware:** Same as Test 35 (interrupt-driven buttons)
**Pi state:** Same session as Test 35 (uptime ~18 min)

### Result: FAIL — device never found

#### Details
- **15 scan attempts** over ~4 minutes, BLE-Remote never appeared in any scan
- Other BLE devices visible (S41 152A LE at -78 to -91 dBm, various unknowns at -78 to -90 dBm)
- ESP32 address 38:44:BE:45:AD:86 never seen — not even weak signal
- Recovery cache clears triggered 3 times, no help

#### RSSI comparison
| Test | RSSI | Result |
|------|------|--------|
| Test 35 (worked) | -89 dBm | PASS — first attempt |
| Test 35b (failed) | not seen | FAIL — completely out of range |

#### Conclusion
The ESP32 is too far away at its new position. At -89 dBm (Test 35) we were already near the limit. The new location is beyond BLE range. **Move the ESP32 closer** — it needs to be within ~5-8 meters with line of sight for reliable operation.

---

## Test 35 — 2026-03-17 11:01 UTC (fresh reboot + interrupt-driven button firmware)

**Duration:** ~3 minutes
**Uptime at start:** 2 minutes (freshly rebooted)
**Firmware:** Test 34 (interrupt-driven buttons, NimBLE)

### Result: PASS — connected on first attempt, rock-solid connection

#### Connection
- **Scan:** Found BLE-Remote after 2s (early exit after 17 devices), RSSI -89 dBm
- **Connect:** Connected in ~4.5s on the **first attempt**
- **Subscribed:** Notifications active at 11:01:23

#### Heartbeats
- **89 heartbeats** received over ~3 minutes (HEARTBEAT #2 through #89)
- Consistent ~2s interval, zero drops
- All STATUS checks: `connected=True mtu=23`

#### Conclusion
**The interrupt-driven button firmware works perfectly.** The failures in Tests 32-34c were caused by accumulated Pi/BlueZ state degradation, not the firmware. A fresh reboot restored clean first-attempt connectivity.

| Test | Firmware | Pi State | Attempts | Result |
|------|----------|----------|----------|--------|
| **35 (11:01)** | **Interrupt buttons (Test 34)** | **Fresh reboot** | **1** | **PASS — 89 heartbeats, stable** |
| 31b (10:53) | GPIO init only (Test 31) | Degraded (20+ cycles) | 8 | Connected but degraded |
| 34c-32 (10:43-01:22) | Various button firmwares | Degraded | All failed | Falsely blamed firmware |
| 31 (01:14) | GPIO init only | Fresh session | 1 | PASS |

### Next steps
- Button firmware is confirmed working — ready to test actual button presses
- Consider implementing periodic Pi bluetooth service restart to prevent state degradation in long sessions

---

## Test 31b — 2026-03-17 10:53 UTC (baseline recheck — exact Test 31 firmware)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: EVENTUALLY CONNECTED — but took 8 attempts (was 1 in original Test 31)

#### Connection attempts
- **Attempts 1-7**: ALL failed with "failed to discover services, device disconnected" (2-6s each)
- **Attempt 8** (10:55:01): **CONNECTED** after 9s → heartbeats flowing

#### Once connected: Stable
- **5 heartbeats** (#6-#10) in the final ~8 seconds before test timeout
- Connection stable, heartbeats every ~2 seconds
- Would likely have held longer if test continued

#### RSSI: Variable
- -81 to -90 dBm across scans — more variable than before

### CRITICAL FINDING: The baseline is broken too

| Test | Firmware | Attempts to connect | RSSI |
|------|----------|-------------------|------|
| **31 (original, 01:14)** | **GPIO init only** | **1** | **-84** |
| **31b (recheck, 10:53)** | **Same firmware** | **8** | **-81 to -90** |

The exact same firmware that connected on the first attempt at 01:14 now takes 8 attempts at 10:53. **The problem is NOT in the firmware at all.**

### What changed between Test 31 and now?

1. **Pi Bluetooth adapter state** — after 20+ test cycles with repeated adapter resets, service restarts, cache removals, and failed connections, the BlueZ/adapter may be in a degraded state
2. **ESP32 position** — was moved, now has more variable RSSI (-81 to -90 vs steady -84)
3. **RF environment** — time of day, other BLE devices, WiFi interference may have changed
4. **BlueZ D-Bus state** — accumulated from many failed connections and recoveries

### Re-evaluation of ALL test results since Test 32

If the baseline itself is unreliable at 8 attempts, then **Tests 32-34c failures may not have been caused by the button code at all.** They may have been caused by the same environmental/adapter degradation that makes Test 31b take 8 attempts.

The only truly reliable comparison is:
- Test 31 (01:14, fresh session): 1 attempt → connected
- Test 32 (01:22, 8 minutes later): connected but unstable
- Test 31b (10:53, 9.5 hours later): 8 attempts → connected

### Suggestion
1. **Reboot the Pi** — full reboot to clear all accumulated BlueZ/kernel state
2. **Run Test 31 firmware again** after reboot — if it connects on first attempt, the baseline is restored
3. **Then immediately run the button firmware** — to get a fair comparison in the same session
4. Consider that the ESP32 at its current position may be marginal — bring it closer (~50cm)

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 35 (11:01) | **PASS** — first attempt, 89 heartbeats, fresh reboot |
| 31b (10:53) | Connected on 8th attempt — baseline degraded |
| 34c (10:43) | FAIL — cache cleared, still fails |
| 34b (10:28) | FAIL — closer range |
| 34 (10:20) | FAIL — interrupt-driven |
| 33 (10:06) | FAIL — no Serial |
| 32b (09:50) | FAIL — 10ms delay |
| 32 (01:22) | PARTIAL — only partially worked |
| 31 (01:14) | **PASS** — first attempt (fresh session) |
| 30 (01:06) | **PASS** — first attempt (fresh session) |
