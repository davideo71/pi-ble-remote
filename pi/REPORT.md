# Pi Test Report: Step 2 — Button Handling

## Test 28 — 2026-03-17 00:32 UTC (full bluetooth service restart before test)

**Duration:** ~120 seconds (after `sudo systemctl restart bluetooth && sleep 3`)

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — "Device not found" on all attempts, even after service restart

#### Pre-test
- Ran `sudo systemctl restart bluetooth && sleep 3` successfully
- This should have cleared all BlueZ state completely

#### Attempt log
1. Full reset → timeout (16s)
2. Full reset → **"Device not found"** (15s)
3. Full reset → **"Device not found"** (15s)
4. Recovery (cache remove) → full reset → **"Device not found"** (15s)
5. Full reset → **"Device not found"** (15s)
6. Full reset → (test timed out)

### Conclusion: This is NOT a Pi/BlueZ issue

A full `systemctl restart bluetooth` clears ALL BlueZ state — cached devices, connections, internal state, everything. If the ESP32 were advertising, a fresh BlueZ would find it via the `BleakClient(address)` call (which does a background scan).

**The ESP32 is not advertising.** The simulated button firmware is either:
1. Crashing before or during BLE initialization
2. Not starting advertising due to the button simulation code
3. Advertising on a different MAC address after reflash

### Diagnostic: Manual scan from Pi

I'll do a quick manual scan to verify:

```
bluetoothctl scan on   (look for any device with "BLE-Remote" or "EasyPlay" or MAC 38:44:BE:45:AD:86)
```

### Suggestion
1. **Check ESP32 serial monitor** — is the ESP32 printing startup messages and heartbeat counts?
2. **Try reverting to the pre-button firmware** (Test 22 version) to confirm the ESP32 hardware is OK
3. **The direct-connect path is not the problem** — the device simply isn't there

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 28 (00:32) | **FAILED** — full service restart didn't help, ESP32 not advertising |
| 27 (00:29) | FAILED — "Device not found" |
| 26 (00:20) | FAILED — "Device not found" |
| 25 (00:12) | FAILED — ESP32 drops connections |
| 24 (00:07) | FAILED — InProgress errors |
| 23 (00:02) | Last successful connection (heartbeats, no buttons) |
| 22-21 | Success — direct connect by MAC |
| 18 | Best scan+connect, 9s to heartbeat |
| 14 | First stable connection (3m41s) |
