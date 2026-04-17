#!/bin/bash
# set-hdmi-surround.sh v2 — adaptive HDMI audio setup
#
# Tries Digital Surround 5.1 (profile 2) first for TVs that support it,
# falls back to Stereo (profile 1) for computer monitors that only
# advertise stereo audio in their EDID.
#
# v1 (preserved as set-hdmi-surround-v1.sh) hardcoded profile 2, which
# left HDMI audio dead on stereo-only displays (PipeWire dropped back
# to auto_null / Dummy Output).
#
# Install: cp set-hdmi-surround.sh ~/bin/ && chmod +x ~/bin/set-hdmi-surround.sh

sleep 10  # wait for PipeWire to fully start

# Returns the sink ID of the first working HDMI sink, or empty if none.
# "Working" means: a sink named "Built-in Audio" + profile suffix, NOT
# the Dummy Output and NOT auto_null.
find_working_sink() {
    # Match either "Digital Surround" or "Stereo" variants
    wpctl status 2>/dev/null | grep -oP '\d+(?=\. Built-in Audio (?:Digital Surround|Stereo))' | head -1
}

# Apply a given profile number to every HDMI audio device, then wait
# for PipeWire to reconfigure and return whether a working sink appeared.
try_profile() {
    local profile=$1
    local label=$2
    echo "Trying profile $profile ($label)..."
    for DEV in $(wpctl status 2>/dev/null | grep -oP '\d+(?=\. Built-in Audio\s+\[alsa\])'); do
        wpctl set-profile $DEV $profile 2>/dev/null
    done
    sleep 2
    local sink
    sink=$(find_working_sink)
    if [ -n "$sink" ]; then
        echo "  Got sink $sink with profile $profile ($label)"
        echo "$sink"
        return 0
    fi
    return 1
}

# Finalize a sink: set it as default, restart easyplay.
finalize() {
    local sink=$1
    wpctl set-default $sink 2>/dev/null
    wpctl set-volume $sink 1.0 2>/dev/null
    # Restart easyplay so it connects to the correct sink
    sudo systemctl restart easyplay 2>/dev/null || true
    echo "Success: sink=$sink, easyplay restarted"
}

# Main adaptive loop: a few attempts, each trying surround then stereo.
for attempt in $(seq 1 10); do
    echo "=== Attempt $attempt ==="

    # Try surround 5.1 first (best quality for TVs)
    sink=$(try_profile 2 "Digital Surround 5.1")
    if [ -n "$sink" ]; then
        finalize "$sink"
        exit 0
    fi

    # Fall back to stereo (works on computer monitors)
    sink=$(try_profile 1 "Stereo")
    if [ -n "$sink" ]; then
        finalize "$sink"
        exit 0
    fi

    echo "Neither profile produced a working sink, retrying in 3s..."
    sleep 3
done

echo "Failed: no working HDMI sink after 10 attempts on either profile"
exit 1
