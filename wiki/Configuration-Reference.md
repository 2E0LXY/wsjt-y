# Configuration Reference

Complete reference for all WSJT-Zii settings (**File → Settings** or `F2`).

---

## General tab

| Setting | Description |
|---------|-------------|
| **Double-click on call sets Tx enable** | Clicking a decoded call automatically starts TX |
| **Tx watchdog** | Stops TX after N minutes of no new decodes; prevents endless calling |
| **Hold Tx freq** | Keep your transmit frequency fixed across QSOs |
| **Show DXCC, grid, and worked-before** | Colour-code decoded calls by DXCC status |
| **TX delay** | Milliseconds to delay TX start (compensates for slow audio hardware) |
| **Decode at t+N** | Delay decode start for stations with slow-rising signals |

---

## Radio tab

| Setting | Description |
|---------|-------------|
| **Rig** | Your transceiver model (uses Hamlib) |
| **Serial port** | COM port (Windows) or /dev/ttyUSBn (Linux) |
| **Baud rate** | Must match radio CAT setting |
| **Data bits** | Typically 8 |
| **Stop bits** | Typically 1 or 2 |
| **Handshake** | None, XON/XOFF, Hardware |
| **Poll interval** | How often WSJT-Zii queries radio status (ms) |
| **Split mode** | None / Fake It / Rig: controls transmit frequency split |
| **PTT method** | CAT / DTR / RTS / VOX / None |
| **PTT port** | Serial port for hardware PTT (if not using CAT PTT) |
| **TX audio channel** | Left / Right / Both for sound card TX |
| **RX audio channel** | Left / Right / Both for sound card RX |

---

## Audio tab

| Setting | Description |
|---------|-------------|
| **Soundcard Input** | Audio device connected to radio receive output |
| **Soundcard Output** | Audio device connected to radio transmit input |
| **Sample rate** | 48000 Hz recommended |

### Calibration
- **Input level:** aim for −30 to −10 dBFS on the waterfall colour scale
- **Output level:** adjust radio mic gain so ALC does not operate

---

## Tx Macros tab

Define up to 5 free-text messages (used for non-standard contacts, contest exchanges, special operations). Access during a QSO via **Tx 1–5** buttons.

---

## Reporting tab

| Setting | Description |
|---------|-------------|
| **PSK Reporter** | Upload received spots to pskreporter.info |
| **N1MM+ server** | UDP address:port for N1MM+ contest logger |
| **Log ADI file** | Path for ADIF log file |
| **Prompt me to log each QSO** | Pause auto-log for manual confirmation |
| **Deconflict UDP port** | Avoid port conflict when running multiple instances |

---

## Frequencies tab

Manage the list of band/mode frequencies shown in the Band/Freq selectors. You can:
- Add custom frequencies
- Remove frequencies you don't use
- Restore defaults

---

## WSJT-Zii tab

Settings specific to WSJT-Zii (not present in base WSJT-X):

| Setting | Description |
|---------|-------------|
| **FT8 Threads** | Number of parallel decoder threads (default: CPU core count) |
| **FT8 Sensitivity** | Normal / Low thresholds / Subpass |
| **FT8 Decoder Start** | Seconds after period start to begin decode (default 3) |
| **Use multithreaded FT8** | Enable the Z-specific parallel decoder |
| **FT8 Cycles** | Number of decode cycles per period |
| **Wide DX call search** | Search wider frequency range for DX calls |
| **Hide FT8 duplicates** | Do not show signals already decoded this period |
| **Reduce false decodes** | Apply additional validation to suspected false decodes |
| **Skip A8 decodes** | Skip the most expensive decode variant (speeds up decode) |
| **ID interval** | Minutes between station ID transmissions |
| **Ignore list reset** | Auto-clear the "ignore" list after N minutes |

---

## Advanced tab

| Setting | Description |
|---------|-------------|
| **Decode depth** | Affects OSD depth for non-FT8 modes |
| **Two-pass decode** | Run decode twice per period (improves weak signal recovery) |
| **CAT command delay** | Wait time between CAT commands (ms) |
| **Close calls on Tx** | Suppress decode panel during transmit |
| **TX power** | Reported power for PSK Reporter and ADIF |

---

## Keyboard shortcuts

| Key | Action |
|-----|--------|
| `F2` | Open Settings |
| `F3` | Open Log QSO |
| `F4` | Clear DX Call / Grid |
| `F5` | Show WSPR decoded messages |
| `Ctrl+F1` | About WSJT-Zii |
| `Ctrl+G` | Open Band Hopper |
| `Ctrl+L` | Open Ordered decode messages |
| `Space` | Enable / Disable Tx |
| `Esc` | Stop TX immediately |
