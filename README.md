# WSJT-Y

**Fork by 2E0LXY (Daren, Wakefield UK) · GPL-3.0 · Based on WSJT-Z v2.0.16 / WSJT-X 3.0.0**

---

## Downloads — v3.4.0

| Platform | File |
|---|---|
| Windows 10/11 64-bit | `wsjty-3.4.0-win64.exe` |
| Debian 12 / Ubuntu 22.04 | `wsjty_3.4.0_amd64.deb` |

**https://github.com/2E0LXY/wsjt-zii/releases/latest**

SHA256 checksums on the releases page.

---

## What is WSJT-Y?

WSJT-X is the reference implementation of weak-signal digital modes (FT8, FT4, WSPR, FST4, MSK144, Q65, JT65, JT9). WSJT-Z extended it with multi-threaded decoding and automation. WSJT-Y adds a comprehensive UI redesign, decoder sensitivity improvements, a live DX Station Map, Auto Band Hopping, and a GridTracker-style Call Roster — while remaining fully compatible with standard WSJT-X.

---

## Features over standard WSJT-X

### Decoder Sensitivity

All decoder paths use extended LDPC iteration counts and raised OSD depth, pushing FT8 sensitivity toward the Shannon limit of −27.5 dB.

| Mode | Change | Expected gain |
|---|---|---|
| FT8 (both paths) | BP iterations 30→50 | ~0.3–0.5 dB |
| FT4 | BP iterations 30→55; ndeep 2→3; maxosd 3→4 | ~0.3–0.4 dB |
| FST4 (240/101 & 240/74) | BP iterations 30→50; maxosd 3→4 | ~0.3 dB |
| MSK144 | BP iterations 10→15 | marginal |
| WSPR | OSD ndeep 5→6 | ~0.2–0.3 dB |
| **FT8 sync (new v3.3.1)** | Parabolic sub-sample time interpolation | ~0.5–1 dB |
| **FT8 freq peak-up (new)** | Parabolic 0.5 Hz grid refinement → ~0.05 Hz | ~0.2–0.5 dB |

The parabolic interpolation improvement was added in v3.3.1. Instead of quantising the Costas sync peak to the nearest integer sample/frequency bin, WSJT-Y refines it with:

`δ = (y[n+1] − y[n−1]) / (2 × (2y[n] − y[n−1] − y[n+1]))`

This significantly reduces misalignment at SNR threshold, where even 22 ms of timing error degrades LDPC LLR quality.

### Experimental NMS Decoder

`Decode → NMS decoder (experimental)` — Normalised Min-Sum LDPC check-node update (α=0.75). Set env var `WSJTY_USE_NMS_DECODER=1` to force-enable without the menu. Claimed gain +0.2–0.4 dB; not the default pending on-air validation.

### WSPR Maximum Tier

`Decode → Maximum` passes `-o 6` to `wsprd` (order-4 OSD, ~230k candidates). Four tiers: Quick, Medium, Deep, **Maximum**.

### DX Station Map

Dockable world map panel (`View → DX Station Map`) powered by a real satellite relief image.

- **All decoded stations** plotted as colour-coded dots accumulating across T/R periods
- **CQ calls**: blue flashing dot + callsign label
- **Stations calling you**: red pulsing dot + expanding radar halo rings
- **Click any dot**: tunes Rx to that station's frequency, populates DX Call/Grid, generates Tx messages, triggers DXCC lookup
- **Great-circle arc** from home QTH to selected station
- **Grid square highlight** (4-char minor square only, no nested boxes)
- **Station info panel** at bottom: callsign, grid, distance, bearing, SNR
- **QRZ XML lookup**: if QRZ credentials entered in Settings → WSJT-Y, station details and photo auto-populate on click
- **Scroll wheel zoom** (×1–8), right-click drag to pan, **⌂ Home button** resets to world view centred on your QTH
- **Clear button** (✕) manually clears all dots; auto-clear on band change
- **Right-click dock title** → Pin Left / Pin Right / Float
- **⋮ hamburger button** on map — dock repositioning even when floating

### Auto Band Hop

`Band Select toolbar → Auto Hop ☑ N min`

When ticked, WSJT-Y automatically selects the best FT8 band every N minutes (default 5) based on UTC time and calendar month, using the propagation model from RSGB 2026 / Ofcom 2024:

| Band | FT8 MHz | Optimal hours UTC | Optimal months | Propagation |
|---|---|---|---|---|
| 160m | 1.840 | 22:00–04:00 | Nov–Feb | Winter night F-layer; zero D-layer absorption |
| 80m | 3.573 | 20:00–06:00 | Nov–Mar | Nocturnal regional/DX; grey-line Pedersen Ray |
| 40m | 7.074 | 24/7 | Year-round | Most reliable 24/7; daytime EU, night global DX |
| 30m | 10.136 | 24/7 | Year-round | WARC digital haven; no contest QRM |
| 20m | 14.074 | 06:00–22:00 | Mar–May, Sep–Nov | F2 equinox DX king |
| 17m | 18.100 | 08:00–19:00 | Mar–May, Sep–Nov | WARC quiet DX; closes at sunset |
| 15m | 21.074 | 08:00–18:00 | Mar–May, Sep–Nov | SFI>100 F2 DX; excellent path loss at peak |
| 12m | 24.915 | 09:00–17:00 | May–Aug, Sep–Mar | Sporadic-E summer; WARC |
| 10m | 28.074 | 10:00–18:00 | May–Aug, equinox | Summer Es + solar-max F2 |

Won't hop if transmitting or mid-auto-QSO. Status bar shows band chosen + reason. 60m excluded (UK Full Licensees only — tune manually).

### Call Roster (GridTracker-style)

`View → Call Roster` — dockable sortable table of all stations heard this period:

- Columns: Callsign, Grid, SNR, Freq Hz, Distance km, Bearing, DXCC, Continent, Message
- **Red row** = station calling your callsign
- **Blue row** = CQ call
- Filter bar for callsign/DXCC/continent
- Stats bar: total count + "N CQ" + "⚡ N calling YOU" flash
- Click any row → tunes Rx + populates DX Call (same as decode browser double-click)
- Entries expire 120 s after last heard; cleared on band change

### Band Quick-Select Toolbar

12-button toolbar (160m–2m) preset to FT8 frequencies. Click any button to instantly switch band.

### Right-Click Waterfall → Set Tx Frequency

Right-click anywhere on the waterfall sets the Tx audio frequency to the clicked position. Rx frequency is unchanged. Left-click behaviour is unaltered.

### UI & Theme

- **Dark teal theme** applied by default — navy/teal panels, dock headers, menus, buttons
- **Modern rounded UI** — 4px radius inputs, buttons, tabs, tooltips; JetBrains Mono decode font
- **Flashing update badge** in status bar when a newer release is on GitHub; Windows: silent auto-install; Linux: opens package manager

### Bug Fixes vs WSJT-Z Base

| Bug | Fix |
|---|---|
| OmniRig Rig 1 missing `case` label in TransceiverFactory → "Hamlib init error" | Added missing `case OmniRigOneId:` label |
| TX goal-post markers invisible (drawn to wrong pixmap) | `painter0` → `painter` (OverlayPixmap) |
| Waterfall callsign overlay invisible (wrong pixmap + accumulation) | Rewritten to use `m_DialOverlayPixmap` correctly |
| FT8 two-pass duplicate lines in Band Activity | Per-period `QSet<QString>` dedup on (freq, clean_string) |
| Narrowing conversion in OmniRig switch (GCC 14 error) | `static_cast<qint64>` |
| 6 further correctness fixes in Fortran decoder paths | See commit history |

---

## Settings

| Location | Setting | Purpose |
|---|---|---|
| `Settings → WSJT-Y` | QRZ.COM username / password | Enables QRZ XML lookup + photo on map click |
| `Decode` menu | `NMS decoder (experimental)` | Toggle Normalised Min-Sum LDPC |
| `Decode` menu | `Maximum` | 4th WSPR decode depth tier |
| `View` menu | `Show callsigns on waterfall` | Callsign overlay on 2D spectrum |
| `View` menu | `DX Station Map` | Toggle/restore map dock |
| `View` menu | `Call Roster` | Toggle/restore roster dock |
| Band toolbar | `Auto Hop` + `N min` | Auto Band Hop enable + interval |
| `Settings → Station` | `My Grid` | Home QTH for map great-circle arcs and Auto Hop |

---

## Build

### Linux (Debian 12 / Ubuntu 22.04)

```bash
sudo apt-get install build-essential cmake ninja-build gfortran \
  libfftw3-dev libhamlib-dev libudev-dev libsamplerate-dev \
  qtbase5-dev qtmultimedia5-dev libqt5serialport5-dev \
  libqt5websockets5-dev libqt5sql5-sqlite libqt5svg5-dev

cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DWSJT_SKIP_MANPAGES=ON ..
ninja
```

### Windows (MSYS2 / MinGW64)

See `docs/WINDOWS-BUILD-PITFALLS.md`. Key points:
- Pin GCC 14 (GCC 16 has `.tls_common` regression)
- `dumpcpp-qt5.exe` → `dumpcpp.exe` rename required for OmniRig
- OmniRig.tlb fetched from 64-bit registry path

---

## Licence

WSJT-Y is distributed under **GNU GPL v3.0**, the same licence as WSJT-X and WSJT-Z.

- WSJT-X: © Joe Taylor K1JT and the WSJT-X Development Group
- WSJT-Z extensions: © SQ9FVE (Mateusz Lubas)  
- WSJT-Y modifications: © 2E0LXY (Daren, 2025–2026)
- NMS decoder: © IU8LMC (Carmine Vitiello), GPL-3.0
- DX Station Map background (`images/worldmap.jpg`): NASA Blue Marble imagery, courtesy Reto Stockli (NASA/GSFC) — public domain (US Government work)

Issues: **https://github.com/2E0LXY/wsjt-zii/issues**
