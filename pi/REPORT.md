# Pi Test Report: Step 2 — Button Handling

## Test 24 — 2026-03-17 00:07 UTC (button test with soldered buttons)

**Duration:** ~120 seconds

### Environment

- **Pi OS:** Linux 6.12.62+rpt-rpi-2712 (aarch64)
- **Python:** 3.13.5
- **bleak:** 2.1.1 (with dbus-fast 4.0.0)
- **BlueZ:** 5.82

### Result: FAILED TO CONNECT — 8+ attempts, all timed out

#### Attempt log
1. Light reset → connect → DISCONNECTED after 12.6s → timeout
2. Light reset → InProgress error → full adapter reset
3. Light reset → connect → "failed to discover services, device disconnected" after 5.3s
4. Recovery (bluetoothctl remove) → Light reset → InProgress → full adapter reset
5. Light reset → connect → DISCONNECTED after 15s → timeout
6. Light reset → InProgress → full adapter reset
7. Recovery (bluetoothctl remove) → Light reset → connect → timeout after 29s
8. Light reset → connect → (test timed out)

**Zero successful connections in 120 seconds.**

#### Pattern: light reset is NOT clearing InProgress
The light reset (disconnect + HCI reset) is consistently failing to clear BlueZ's connection state. After every failed connection or disconnect, the next attempt hits InProgress. The full adapter reset clears it, but then the next connection attempt fails too.

This is a regression from Test 22 where the same light reset worked on cold start.

### Analysis

Possible causes:
1. **BlueZ accumulated bad state from Test 23** — the previous session's connection state may be lingering in BlueZ/D-Bus even after the script exits
2. **ESP32 may be in a bad state** — the button GPIO configuration in the new firmware could be affecting BLE stability
3. **Light reset is unreliable as sole reset method** — it works sometimes (Test 22) but not consistently. The full power cycle is more reliable but still not enough when BlueZ state is deeply corrupted.

### Suggestion
1. **Start with a full adapter reset (not light reset) on cold start** — the light reset should only be used for reconnection after a known-good connection
2. **Consider running `sudo systemctl restart bluetooth` as nuclear option** when recovery fails after 5+ attempts
3. **Check ESP32 serial output** — the new button firmware may have introduced a BLE issue

### Button events
No buttons were tested — couldn't establish a connection.

---

## Previous Tests Summary

| Test | Result |
|------|--------|
| 24 (00:07) | **FAILED** — 8+ attempts, all timed out or InProgress. Light reset unreliable. |
| 23 (00:02) | Connection stable, heartbeats OK, no buttons pressed |
| 22 (23:37) | Light reset, 12.6s to heartbeat, 107s stable |
| 21 (23:27) | Direct connect by MAC, 13s to heartbeat, 105s stable |
| 18 (23:04) | Best scan+connect, 9s to heartbeat |
| 14 (22:30) | First stable connection (3m41s) |
| 1-13 | Early tests — discovery and connection issues |
