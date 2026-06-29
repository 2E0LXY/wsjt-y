# Installation

## Windows (64-bit)

### Requirements
- Windows 10 64-bit or later
- No additional runtimes required — all dependencies are bundled

### Steps

1. Go to the [Releases page](https://github.com/2E0LXY/wsjt-zii/releases/latest)
2. Download `wsjtz-3.0.0-2.0.16-win64.exe`
3. Run the installer — accept the UAC prompt
4. Default install path: `C:\WSJT\wsjtx\`
5. A desktop shortcut and Start Menu entry for **WSJT-Zii** are created

### Uninstall

Use **Add or Remove Programs** → search for **WSJT-Zii** → Uninstall.

### Upgrading

Run the new installer over the existing installation. Settings in `%APPDATA%\WSJT-X` are preserved.

---

## Linux (Debian / Ubuntu 22.04+)

### Requirements
- Debian 12 (Bookworm) or Ubuntu 22.04 LTS or later, amd64
- PulseAudio or ALSA (pipewire with pulseaudio compatibility also works)

### Steps

1. Download `wsjtz_3.0.0-2.0.16_amd64.deb` from the [Releases page](https://github.com/2E0LXY/wsjt-zii/releases/latest)

2. Install:
```bash
sudo dpkg -i wsjtz_3.0.0-2.0.16_amd64.deb
sudo apt-get install -f          # fix any missing dependencies
```

3. Launch from your desktop menu, or from a terminal:
```bash
wsjtx
```

### Upgrading

Install the new `.deb` over the old one — `dpkg` will replace in place. Configuration in `~/.config/WSJT-X 2.x/` is preserved.

### Removing

```bash
sudo dpkg -r wsjtz
```

---

## Post-install: audio and CAT setup

See the **[Getting Started](Getting-Started)** guide for audio device selection, radio CAT configuration and PTT setup.

---

## Verifying the installation

After launch you should see:
- Title bar: **wsjtx  (WSJT-Zii by 2E0LXY v2.0.16)**
- **Help → About WSJT-Zii** shows the version and GitHub link

If you see an older title bar (e.g. "WSJT-Z MOD by SQ9FVE"), you have an old release installed. Download the latest from the releases page.

---

## Troubleshooting installation

| Problem | Solution |
|---------|----------|
| Windows Defender SmartScreen blocks installer | Click **More info** → **Run anyway** — the installer is unsigned |
| `dpkg: dependency problems` on Linux | Run `sudo apt-get install -f` after dpkg |
| "Entry point not found" on Windows | You have an old installer — download the latest |
| Audio not working | See [Getting Started — Audio Setup](Getting-Started#audio-setup) |
