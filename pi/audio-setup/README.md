# Pi Audio Setup for EasyPlay

## Problem
VLC 3.0 on Pi 5 with PipeWire tries to pass through AC3/EAC3 audio as raw bitstream
via its spdif plugins. Most displays can't decode raw bitstream audio, resulting in
silence. Additionally, PipeWire defaults to stereo HDMI output, rejecting surround
audio streams.

## Fix (3 parts)

### 1. Disable VLC spdif passthrough plugins
Forces VLC to decode AC3/EAC3 to PCM instead of passing raw bitstream:
```bash
sudo mv /usr/lib/aarch64-linux-gnu/vlc/plugins/codec/libspdif_plugin.so \
        /usr/lib/aarch64-linux-gnu/vlc/plugins/codec/libspdif_plugin.so.disabled
sudo mv /usr/lib/aarch64-linux-gnu/vlc/plugins/audio_filter/libtospdif_plugin.so \
        /usr/lib/aarch64-linux-gnu/vlc/plugins/audio_filter/libtospdif_plugin.so.disabled
```

### 2. Install the HDMI surround boot script
Switches PipeWire from hdmi-stereo to hdmi-surround (5.1) on every boot:
```bash
mkdir -p ~/bin
cp set-hdmi-surround.sh ~/bin/
chmod +x ~/bin/set-hdmi-surround.sh
```

### 3. Enable the systemd user service
Runs the script automatically at login:
```bash
mkdir -p ~/.config/systemd/user
cp hdmi-surround.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable hdmi-surround.service
```

### Manual run (if needed)
```bash
~/bin/set-hdmi-surround.sh
```

## How it works
- PipeWire's HDMI profile defaults to stereo or "off" after boot (timing dependent)
- The boot script waits for PipeWire, then sets the surround 5.1 profile on all
  HDMI audio devices, forces the sink as default via `pw-metadata`, sets volume
  to 100%, and restarts easyplay
- With spdif plugins disabled, VLC always decodes surround audio to PCM, which
  PipeWire outputs as 5.1 channel PCM over HDMI

## VLC args in easyplay
```python
vlc_args = ["--no-audio-time-stretch", "--avcodec-hw=drm", "--no-spdif",
            "--audio-language=eng,en", "--sub-language=none"]
```

## Troubleshooting
```bash
# Check current audio sink
wpctl status | grep Sink

# Manually set surround profile (device ID may vary)
wpctl set-profile <DEVICE_ID> 2

# Check boot script status
systemctl --user status hdmi-surround.service

# Check if spdif plugins are disabled
ls /usr/lib/aarch64-linux-gnu/vlc/plugins/codec/libspdif_plugin.so*
ls /usr/lib/aarch64-linux-gnu/vlc/plugins/audio_filter/libtospdif_plugin.so*

# Re-enable spdif if ever needed (e.g. receiver that decodes AC3)
sudo mv /usr/lib/aarch64-linux-gnu/vlc/plugins/codec/libspdif_plugin.so.disabled \
        /usr/lib/aarch64-linux-gnu/vlc/plugins/codec/libspdif_plugin.so
sudo mv /usr/lib/aarch64-linux-gnu/vlc/plugins/audio_filter/libtospdif_plugin.so.disabled \
        /usr/lib/aarch64-linux-gnu/vlc/plugins/audio_filter/libtospdif_plugin.so
```
