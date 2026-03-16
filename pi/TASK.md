# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Run `python3 pi/ble_receiver.py` for about **2 minutes**
3. Update `pi/REPORT.md` with results, commit and push

## What changed this iteration (Test 25)
**Reverted to full adapter reset.** The light reset (disconnect + hci reset) was unreliable — Test 24 had 8 failed attempts, all hitting InProgress. Back to the proven full power cycle (1s off + 3s on = 4s).

Buttons are soldered and will be pressed during the test.

## Expected
- Connection on first attempt (~13s cold start like Test 21)
- Button events arriving as `BUTTON: 'L' → LEFT press` etc.

## Key question
Does the full adapter reset restore reliable connections? Do button events arrive?
