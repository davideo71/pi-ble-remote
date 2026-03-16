# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 19)
**Skip scanning on reconnect — connect directly by MAC.** Test 18 was perfect (9s cold start, first-attempt success). Now optimizing reconnection speed. When the Pi has previously connected, it skips scanning entirely and connects directly to the known MAC address using `connect_and_listen_by_address()`.

## Test procedure
1. Let it connect normally (cold start, will scan as usual)
2. Once connected and receiving heartbeats, **power-cycle the ESP32** (unplug and replug USB) to force a disconnect
3. Watch the reconnection — it should skip scanning and connect directly by MAC
4. Log how long the reconnection takes

## Expected
- Cold start: ~9 seconds (same as Test 18)
- **Reconnection after disconnect: ~4-5 seconds** (no scan, no adapter reset — just direct connect)
- Stable connection once established

## Key question
How fast is reconnection after a disconnect? (Compare cold start vs reconnect timing)
