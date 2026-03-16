# Pi Test Report: Step 2 — Button Handling

## Test 27 — 2026-03-17 00:29 UTC (ESP32 confirmed advertising, retry simulated buttons)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED — "Device not found" on all 6 attempts

Same as Test 26. The direct-connect path (`BleakClient(address)`) consistently fails with "Device with address 38:44:BE:45:AD:86 was not found."

#### Attempt log
1. Full reset → **"Device not found"** (15s)
2. Full reset → **"Device not found"** (15s)
3. Full reset → timeout (24s)
4. Recovery (cache remove) → full reset → **"Device not found"** (15s)
5. Full reset → **"Device not found"** (15s)
6. Full reset → (test timed out)

### Analysis: Direct-connect may need a prior scan

The direct-connect-by-MAC approach worked in Tests 21-22 but is now consistently failing. The issue may be:

1. **After a full adapter power cycle, BlueZ has no cached devices.** `BleakClient(address)` may rely on BlueZ having seen the device in a prior scan. Since we reset the adapter every attempt, the cache is empty each time.

2. **In Tests 21-22, BlueZ may have retained the device in its cache** from a previous run (because the device was previously paired/connected). After the failed Tests 24-26 (with cache removals and multiple resets), that cached entry may now be gone.

3. **The ESP32 may actually not be advertising.** Despite being "confirmed," the simulated button firmware may be consuming all CPU in the button simulation loop, preventing the BLE stack from advertising.

### Suggestion: Fall back to scan-based discovery

The direct-connect optimization only works when BlueZ already knows about the device. We should:

1. **On first connection, always scan** (like the original flow in Tests 13-18). This populates BlueZ's cache.
2. **Only use direct-connect for reconnection** after a successful connection in the same session.
3. Alternatively, **do a quick scan before direct connect** to ensure BlueZ has the device.

This was actually how it worked in Test 18 (the best test): scan first → find device → connect. The pre-seeded MAC optimization (Test 21+) skips this scan, which breaks when BlueZ cache is empty.

### Quick diagnostic
Could also try a manual scan from the Pi to check if the ESP32 is visible:
```
bluetoothctl scan on   # (10 seconds, look for 38:44:BE:45:AD:86)
```

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 27 (00:29) | **FAILED** — "Device not found" on all attempts |
| 26 (00:20) | FAILED — "Device not found" |
| 25 (00:12) | FAILED — ESP32 drops connections during discovery |
| 24 (00:07) | FAILED — InProgress errors |
| 23 (00:02) | Connection stable, heartbeats OK, no buttons pressed |
| 22-21 | Success — direct connect by MAC |
| 18 | Best scan+connect, 9s to heartbeat |
| 14 | First stable connection (3m41s) |
| 1-13 | Early tests |
