# Troubleshooting

Common problems and their solutions.

---

## Installation problems

### "Windows Defender SmartScreen prevented an unrecognised app from starting"

The installer is not code-signed. Click **More info** then **Run anyway**.

### "Entry Point Not Found: `__emutls_v_ZSt11__once_call`" or "`_ZSt15__get_once_callv`"

You have an old installer with a DLL mismatch bug. Download the **latest release** from https://github.com/2E0LXY/wsjt-zii/releases — this is fixed in v3.0.0 and later via static-linked runtime.

### Linux: `dpkg: dependency problems`

```bash
sudo apt-get install -f
```

If specific packages are missing, identify them from the dpkg error and install manually.

---

## Audio problems

### No waterfall / waterfall is blank

1. Check **Settings → Audio** — verify the correct input device is selected
2. Click the **Monitor** button in the main window (it should light up)
3. Check your soundcard mixer — ensure the line-in or USB audio device is not muted
4. On Linux, check PulseAudio/PipeWire routing: `pavucontrol`

### Waterfall is all white / overloaded

Your audio input level is too high. Reduce the level in your radio or soundcard mixer until the noise floor appears as dark blue/grey.

### No audio transmitted

1. Check **Settings → Audio** — verify the correct output device is selected
2. Check **Settings → Radio → PTT method** — ensure it matches your hardware
3. Verify your radio is in DATA/USB mode (not SSB or FM)
4. Check the radio's microphone gain / data input level

### Audio clicks / discontinuities

- On Linux: try `--pulse` audio backend or switch to ALSA
- On Windows: increase the sound card buffer size
- Check for USB audio device disconnection issues

---

## CAT control problems

### "Test CAT" button stays red

1. Verify the COM port number in Device Manager (Windows) or `ls /dev/ttyUSB*` (Linux)
2. Check baud rate matches your radio's setting
3. Try toggling **RTS** and **DTR** settings
4. Ensure no other application (logging software, another instance of WSJT-Zii) has the port open
5. On Linux: check permissions — `sudo usermod -a -G dialout $USER` then log out/in

### Radio doesn't switch bands

WSJT-Zii changes frequency when you select a different band/frequency. Ensure **CAT** is enabled and the Test CAT button is green.

### Wrong frequency displayed

Check the **Offset** setting in Settings → Radio if your radio reports a different frequency than expected (e.g. some radios report the carrier frequency when in USB mode).

---

## Decode problems

### "No decodes" even though I can hear signals

1. Verify you are on the correct dial frequency (e.g. 14.074 MHz USB for 20 m FT8)
2. Check your computer clock — FT8 requires UTC accuracy within ±1 second. Use Windows Time Service or install `chrony`/`ntp` on Linux
3. Check the **DT** column in decodes — if all values are > ±2.0, your clock is off

### Fewer decodes than expected

- Try **Decode → Sensitivity → Low thresholds** or **Subpass**
- Check your audio level is not too low (waterfall should be visible)
- Verify you are not in a transmit period (signals are suppressed during TX)

### Computer clock sync (Windows)

Open an elevated command prompt:
```
w32tm /resync
```

Or install Meinberg NTP: https://www.meinbergglobal.com/english/sw/ntp.htm

### Computer clock sync (Linux)

```bash
sudo timedatectl set-ntp true
timedatectl status
```

---

## Transmit problems

### "Tx watchdog" stops transmission

The watchdog fires after N minutes of no new decodes. Either:
- Increase the watchdog time in Settings → General
- Disable the watchdog (set to 0)

### TX enable turns itself off

The auto-sequence has completed a QSO and disabled TX as expected. If this happens unexpectedly, check the **Auto Seq** logic and filters.

### Audio distortion on transmit

ALC is compressing. Reduce the output level in Settings → Audio or in your radio's data input gain. FT8 is constant-amplitude — ALC is never needed.

---

## Logging problems

### Contact not logged

- WSJT-Zii logs a contact when `73` or `RRR` is received and confirmed
- Check the log file path in Settings → Reporting
- If using UDP logging, verify the logging application is running

### Duplicate contacts in log

Turn on **"Worked before" indicator** — this highlights previously worked stations in the decode panel. Consider enabling **message tombstoning** in Settings → WSJT-Zii.

---

## PSK Reporter

### Spots not appearing on PSK Reporter

1. Enable in Settings → Reporting → "Enable PSK Reporter"
2. Check your callsign and grid are correctly set in Settings → Station
3. PSK Reporter updates can take 5–15 minutes to appear on the map
4. Verify your internet connection is working

---

## Getting more help

If your issue is not covered here:

1. Check the [GitHub Issues](https://github.com/2E0LXY/wsjt-zii/issues) for known problems
2. Open a new issue with your OS, WSJT-Zii version, and a description of the problem
3. Include the debug log: enable via **Help → WSJT-Zii Debug**, reproduce the issue, then attach `wsjtx_debug.log`
