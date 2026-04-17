#!/bin/bash
# Set HDMI audio to Digital Surround 5.1 on boot
# Then restart easyplay so it picks up the correct audio sink
#
# Install: cp set-hdmi-surround.sh ~/bin/ && chmod +x ~/bin/set-hdmi-surround.sh

sleep 10  # wait for PipeWire to fully start

for attempt in $(seq 1 30); do
    for DEV in $(wpctl status 2>/dev/null | grep -oP '\d+(?=\. Built-in Audio\s+\[alsa\])'); do
        wpctl set-profile $DEV 2 2>/dev/null
    done

    sleep 2

    SINK_ID=$(wpctl status 2>/dev/null | grep -oP '\d+(?=\. Built-in Audio Digital Surround)')
    if [ -n "$SINK_ID" ]; then
        wpctl set-default $SINK_ID 2>/dev/null
        pw-metadata 0 default.audio.sink '{"name": "alsa_output.platform-107c701400.hdmi.hdmi-surround"}' Spa:String:JSON 2>/dev/null
        wpctl set-volume $SINK_ID 1.0 2>/dev/null
        # Restart easyplay so it connects to the correct sink
        sudo systemctl restart easyplay
        echo "Success: sink=$SINK_ID (attempt $attempt), easyplay restarted"
        exit 0
    fi

    sleep 3
done
echo "Failed after 30 attempts"
exit 1
