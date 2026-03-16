# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 28)
Two fixes based on analysis of Tests 24-27 failures:

1. **Scan first on cold start, direct connect only for reconnect.** The pre-seeded MAC broke when BlueZ cache was emptied by `bluetoothctl remove` during recovery. Now `connected_address` starts as `None`, so the first connection uses scan-based discovery (like Test 18). Direct connect only after a successful connection in the same session.

2. **`systemctl restart bluetooth` on startup.** Clears accumulated BlueZ state that caused 4 consecutive test failures. This runs automatically at script start.

## Expected
- Bluetooth service restart clears bad state
- Scan finds the device on first try (like Tests 18-21)
- Connection succeeds
- Simulated button events arrive (L, R, U, D, O cycling every 500ms)
- Direct connect used for any reconnection within the session

## Key question
Does scan-first + service restart restore reliable connections? Do buttons arrive?
