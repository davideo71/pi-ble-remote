#!/usr/bin/env python3
"""
EasyPlay Status — Mac companion tool to view watch progress on LuckyPi.

Connects to the Pi via SSH (local network or Tailscale), reads the
EasyPlay progress file, and displays a formatted overview of what's
been watched, what's in progress, and what's unwatched.

Usage:
    python3 easyplay_status.py              # auto-detect Pi (LAN then Tailscale)
    python3 easyplay_status.py --host IP    # specific host
    python3 easyplay_status.py --refresh 30 # auto-refresh every 30 seconds
"""

import argparse
import json
import subprocess
import sys
import os
import time
from pathlib import Path
from datetime import datetime

# Pi connection details
PI_USER = "david"
PI_HOSTS = [
    "192.168.178.231",   # LAN ethernet
    "192.168.178.232",   # LAN wifi
    "100.65.29.11",      # Tailscale
]
PROGRESS_FILE = "/home/david/Desktop/EasyPlay/easyplay_progress.json"
MEDIA_DIR = "/home/david/Desktop/codevideos"

# Display
BAR_WIDTH = 20
COLORS = {
    "green":   "\033[92m",
    "yellow":  "\033[93m",
    "red":     "\033[91m",
    "blue":    "\033[94m",
    "cyan":    "\033[96m",
    "dim":     "\033[2m",
    "bold":    "\033[1m",
    "reset":   "\033[0m",
}

def c(color, text):
    """Wrap text in ANSI color."""
    if not sys.stdout.isatty():
        return text
    return f"{COLORS.get(color, '')}{text}{COLORS['reset']}"


def ssh_cmd(host, cmd, timeout=10):
    """Run a command on the Pi via SSH, return stdout or None on failure."""
    try:
        result = subprocess.run(
            ["ssh", "-o", "ConnectTimeout=5", "-o", "StrictHostKeyChecking=accept-new",
             f"{PI_USER}@{host}", cmd],
            capture_output=True, text=True, timeout=timeout
        )
        if result.returncode == 0:
            return result.stdout
        return None
    except (subprocess.TimeoutExpired, Exception):
        return None


def find_pi(forced_host=None):
    """Find a reachable Pi host."""
    if forced_host:
        out = ssh_cmd(forced_host, "echo ok")
        if out and "ok" in out:
            return forced_host
        print(f"Cannot reach Pi at {forced_host}")
        sys.exit(1)

    for host in PI_HOSTS:
        out = ssh_cmd(host, "echo ok")
        if out and "ok" in out:
            return host
    return None


def fetch_progress(host):
    """Fetch the progress JSON from the Pi."""
    out = ssh_cmd(host, f"cat {PROGRESS_FILE}")
    if out:
        try:
            return json.loads(out)
        except json.JSONDecodeError:
            return None
    return None


def fetch_all_media(host):
    """Fetch the list of all media folders and their video files."""
    cmd = f"""python3 -c "
import json
from pathlib import Path
VIDEO_EXTS = ('.mp4', '.mkv', '.avi', '.mov', '.webm')
HIDDEN_NAMES = {{'sample', 'samples', '.ds_store', 'thumbs.db', '__macosx'}}
def _is_hidden(p): return p.name.startswith('.') or p.name.startswith('._') or p.name.lower() in HIDDEN_NAMES
root = Path('{MEDIA_DIR}')
result = {{}}
for subdir in sorted(root.iterdir()):
    if not subdir.is_dir() or _is_hidden(subdir): continue
    videos = [str(f) for f in subdir.iterdir()
              if f.is_file() and not _is_hidden(f)
              and f.suffix.lower() in VIDEO_EXTS]
    if videos:
        result[subdir.name] = sorted(videos)
print(json.dumps(result))
"
"""
    out = ssh_cmd(host, cmd, timeout=15)
    if out:
        try:
            return json.loads(out.strip())
        except json.JSONDecodeError:
            return None
    return None


def format_duration(seconds):
    """Format seconds as h:mm or m:ss."""
    if seconds <= 0:
        return "0:00"
    h = int(seconds) // 3600
    m = (int(seconds) % 3600) // 60
    s = int(seconds) % 60
    if h > 0:
        return f"{h}:{m:02d}:{s:02d}"
    return f"{m}:{s:02d}"


def progress_bar(ratio, width=BAR_WIDTH):
    """Create a visual progress bar."""
    filled = int(ratio * width)
    empty = width - filled
    if ratio >= 0.95:
        color = "green"
    elif ratio > 0:
        color = "yellow"
    else:
        color = "dim"
    bar = "█" * filled + "░" * empty
    return c(color, bar)


def clean_name(folder_name):
    """Clean folder name for display (simplified version of EasyPlay's cleaner)."""
    import re
    name = folder_name
    # Remove bracket/paren groups
    name = re.sub(r"\[[^\]]*\]", "", name)
    name = re.sub(r"\([^)]*\)", "", name)
    # Remove common tags
    for tag in ["720p", "1080p", "2160p", "WEBRip", "BluRay", "WEB-DL", "WEB-DLRip",
                "x264", "x265", "HEVC", "AAC", "DD5.1", "DDP", "5.1",
                "AMZN", "NF", "HMAX", "YTS", "MX", "RARBG", "GalaxyRG",
                "TGx", "eztv", "ION265", "ExKinoRay", "WEBRip", "10Bit",
                "NeoNoir", "REPACK", "PROPER", "COMPLETE",
                "BONE", "ESub", "aWEBRip", "Eng", "Criterion",
                "-GalaxyTV", "GalaxyTV"]:
        name = re.sub(rf"\b{re.escape(tag)}\b", "", name, flags=re.IGNORECASE)
    # Clean separators
    name = name.replace(".", " ").replace("_", " ")
    # Remove standalone years (but keep if part of title like "1984")
    name = re.sub(r"\b(19|20)\d{2}\b", "", name)
    # Remove season tags like S01, S02 (keep if it's the main identifier like "S1")
    name = re.sub(r"\bS\d{2}\b", "", name)
    # Remove codec-like fragments: H 264, H264
    name = re.sub(r"\bH\s*264\b", "", name)
    # Remove filesize patterns like 900MB, 1600MB
    name = re.sub(r"\b\d+MB\b", "", name)
    name = re.sub(r"\s+", " ", name).strip(" -,;")
    return name


def display_status(progress, all_media, host):
    """Display the watch status overview."""
    # Clear screen
    if sys.stdout.isatty():
        print("\033[2J\033[H", end="")

    print(c("bold", f"  EasyPlay Watch Status") + c("dim", f"  ({PI_USER}@{host})"))
    print(c("dim", "  " + "─" * 70))
    print()

    # Categorize all media
    completed = []
    in_progress = []
    unwatched = []

    # Build a lookup from video path to progress entry
    # Normalize paths: progress uses /mnt/media/codevideos, scan uses /home/david/Desktop/codevideos
    def norm_path(p):
        return p.replace("/home/david/Desktop/codevideos/", "/mnt/media/codevideos/")

    progress_by_path = {}
    for key, entry in (progress or {}).items():
        progress_by_path[entry.get("path", key)] = entry

    for folder_name, video_paths in (all_media or {}).items():
        display_name = clean_name(folder_name)
        is_series = len(video_paths) > 1

        if is_series:
            # Series: check progress on each episode
            total_eps = len(video_paths)
            watched_eps = 0
            started_eps = 0
            last_updated = None
            for vp in video_paths:
                entry = progress_by_path.get(norm_path(vp))
                if entry:
                    if entry.get("completed"):
                        watched_eps += 1
                    elif entry.get("position_sec", 0) > 0:
                        started_eps += 1
                    ts = entry.get("last_updated")
                    if ts and (last_updated is None or ts > last_updated):
                        last_updated = ts

            if watched_eps == total_eps:
                completed.append({
                    "name": display_name,
                    "detail": f"{total_eps} episodes",
                    "ratio": 1.0,
                    "last_updated": last_updated,
                    "is_series": True,
                })
            elif watched_eps > 0 or started_eps > 0:
                ratio = watched_eps / total_eps
                in_progress.append({
                    "name": display_name,
                    "detail": f"{watched_eps}/{total_eps} episodes watched",
                    "ratio": ratio,
                    "last_updated": last_updated,
                    "is_series": True,
                })
            else:
                unwatched.append({
                    "name": display_name,
                    "detail": f"{total_eps} episodes",
                    "is_series": True,
                })
        else:
            # Movie: single video
            vp = video_paths[0]
            entry = progress_by_path.get(norm_path(vp))
            if entry:
                pos = entry.get("position_sec", 0)
                dur = entry.get("duration_sec", 0)
                is_completed = entry.get("completed", False)
                last_updated = entry.get("last_updated")

                if is_completed:
                    completed.append({
                        "name": display_name,
                        "detail": format_duration(dur),
                        "ratio": 1.0,
                        "last_updated": last_updated,
                        "is_series": False,
                    })
                elif pos > 0 and dur > 0:
                    ratio = pos / dur
                    in_progress.append({
                        "name": display_name,
                        "detail": f"{format_duration(pos)} / {format_duration(dur)}",
                        "ratio": ratio,
                        "last_updated": last_updated,
                        "is_series": False,
                    })
                else:
                    unwatched.append({
                        "name": display_name,
                        "detail": format_duration(dur) if dur > 0 else "",
                        "is_series": False,
                    })
            else:
                unwatched.append({
                    "name": display_name,
                    "detail": "",
                    "is_series": False,
                })

    # Sort: in-progress by last updated (most recent first), others alphabetically
    in_progress.sort(key=lambda x: x.get("last_updated") or "", reverse=True)
    completed.sort(key=lambda x: x.get("last_updated") or "", reverse=True)
    unwatched.sort(key=lambda x: x["name"].lower())

    # Display in-progress
    if in_progress:
        print(c("yellow", "  ▶ IN PROGRESS"))
        print()
        for item in in_progress:
            icon = "📺" if item.get("is_series") else "🎬"
            pct = int(item["ratio"] * 100)
            bar = progress_bar(item["ratio"])
            ts = ""
            if item.get("last_updated"):
                try:
                    dt = datetime.fromisoformat(item["last_updated"])
                    ts = c("dim", f"  {dt.strftime('%b %d')}")
                except: pass
            print(f"    {icon} {item['name'][:40]:<40s} {bar} {pct:>3d}%  {c('dim', item['detail'])}{ts}")
        print()

    # Display completed
    if completed:
        print(c("green", "  ✓ COMPLETED"))
        print()
        for item in completed:
            icon = "📺" if item.get("is_series") else "🎬"
            ts = ""
            if item.get("last_updated"):
                try:
                    dt = datetime.fromisoformat(item["last_updated"])
                    ts = c("dim", f"  {dt.strftime('%b %d')}")
                except: pass
            print(f"    {icon} {c('green', item['name'][:40]):<50s} {c('dim', item['detail'])}{ts}")
        print()

    # Display unwatched
    if unwatched:
        print(c("dim", "  ○ NOT YET WATCHED"))
        print()
        for item in unwatched:
            icon = "📺" if item.get("is_series") else "🎬"
            detail = f"  {c('dim', item['detail'])}" if item["detail"] else ""
            name_str = c('dim', f"{item['name'][:40]:<40s}")
            print(f"    {icon} {name_str}{detail}")
        print()

    # Summary
    total = len(completed) + len(in_progress) + len(unwatched)
    print(c("dim", "  " + "─" * 70))
    print(f"  {c('green', str(len(completed)))} completed  "
          f"{c('yellow', str(len(in_progress)))} in progress  "
          f"{c('dim', str(len(unwatched)))} unwatched  "
          f"{c('dim', f'({total} total)')}")
    print()


def main():
    parser = argparse.ArgumentParser(description="EasyPlay watch status viewer")
    parser.add_argument("--host", help="Pi hostname or IP (auto-detect if omitted)")
    parser.add_argument("--refresh", type=int, metavar="SEC",
                        help="Auto-refresh interval in seconds")
    args = parser.parse_args()

    print(c("dim", "  Connecting to Pi..."), end="", flush=True)
    host = find_pi(args.host)
    if not host:
        print(f"\r  {c('red', 'Cannot reach Pi on any known address.')}")
        print(f"  Tried: {', '.join(PI_HOSTS)}")
        sys.exit(1)
    print(f"\r  {c('green', f'Connected via {host}')}" + " " * 20)

    try:
        while True:
            progress = fetch_progress(host)
            all_media = fetch_all_media(host)

            if all_media is None:
                print(c("red", "  Failed to read media library from Pi."))
                sys.exit(1)

            display_status(progress, all_media, host)

            if args.refresh:
                print(c("dim", f"  Refreshing in {args.refresh}s... (Ctrl+C to quit)"))
                time.sleep(args.refresh)
            else:
                break
    except KeyboardInterrupt:
        print(f"\n  {c('dim', 'Bye!')}")


if __name__ == "__main__":
    main()
