# Task for Pi-side Claude

## What to do
1. `git pull` to get the latest code
2. Install dependencies: `pip install -r pi/requirements.txt`
3. Run `python3 pi/ble_receiver.py` for about 30 seconds
4. Capture the full terminal output
5. Create a file `pi/REPORT.md` with:
   - The full output from the receiver script
   - Whether it found the "BLE-Remote" device
   - Whether it connected and received heartbeat notifications
   - Any errors or warnings
   - Your BlueZ version (`bluetoothctl --version`)
   - Python and bleak versions
6. Commit and push the report:
   ```bash
   git add pi/REPORT.md
   git commit -m "Pi test report: step 1 BLE connection test"
   git push origin main
   ```

## Context
The ESP32-C3 is flashed and advertising as "BLE-Remote" right now. It sends heartbeat notifications every 2 seconds. We want to verify the Pi can see it, connect, and receive data.
