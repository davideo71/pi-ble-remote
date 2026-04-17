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
# "Working" means: any sink whose description starts with "Built-in Audio"
# (excludes Dummy Output / auto_null which doesn't have that prefix).
# Matches all real variants: "Digital Stereo (HDMI)", "Digital Surround 5.1 (HDMI)",
# "Pro 32", etc.
find_working_sink() {
    wpctl status 2>/dev/null | awk '
        /^Audio/           {in_audio=1}
        /^Video/ || /^Settings/ {in_audio=0}
        in_audio && / Sinks:/ {in_sinks=1; next}
        in_audio && /─/ && !/Sinks:/ {in_sinks=0}
        in_sinks && /Built-in Audio/ {
            match($0, /[0-9]+\./)
            if (RSTART > 0) {
                id = substr($0, RSTART, RLENGTH - 1)
                print id
                exit
            }
        }
    '
}

# Apply a given profile number to every HDMI audio device, then wait
# for PipeWire to reconfigure and return whether a working sink appeared.
try_profile() {
    local profile=$1
    local label=$2
    # Diagnostic output goes to stderr so only the sink ID is captured via $()
    echo "Trying profile $profile ($label)..." >&2
    for DEV in $(wpctl status 2>/dev/null | grep -oP '\d+(?=\. Built-in Audio\s+\[alsa\])'); do
        wpctl set-profile $DEV $profile 2>/dev/null
    done
    sleep 2
    local sink
    sink=$(find_working_sink)
    if [ -n "$sink" ]; then
        echo "  Got sink $sink with profile $profile ($label)" >&2
        echo "$sink"
        return 0
    fi
    return 1
}

# Finalize a sink: set it as default.
# NOTE: Does NOT restart easyplay. The old v1 did, which caused dueling
# easyplay instances when the watcher had already launched one. EasyPlay
# picks up the default sink on its own next launch via the watcher.
finalize() {
    local sink=$1
    wpctl set-default $sink 2>/dev/null
    wpctl set-volume $sink 1.0 2>/dev/null
    echo "Success: sink=$sink set as default"
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
