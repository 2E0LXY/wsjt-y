# FT8 Operating Guide

FT8 (Franke-Taylor design, 8-FSK modulation) is the most widely used digital weak-signal mode. This guide covers everyday FT8 operating with WSJT-Zii.

---

## The FT8 protocol

- **T/R period:** 15 seconds (even/odd minutes)
- **Bandwidth:** 50 Hz per signal
- **Message:** 77 bits — callsigns + locator or signal report
- **Theoretical sensitivity:** −27.5 dB S/N (Shannon limit)
- **Practical sensitivity:** −21 dB (standard), −25 dB (WSJT-Zii high-sensitivity mode)

Each 15-second period:
- 0.0–12.6 s — audio transmission
- 12.6–15.0 s — processing and display

---

## The waterfall

The waterfall shows received signals in real time:
- **X-axis:** audio frequency (0–3000 Hz)
- **Y-axis:** time (newest at top)
- **Colour:** signal strength (blue = weak, yellow/white = strong)
- **Red markers:** your transmit frequency
- **Vertical lines:** individual FT8 signals

Good waterfall noise floor: grey/blue background with individual signal traces clearly visible. If it's all white, your input level is too high.

---

## Basic QSO procedure

### Calling CQ

1. Set your dial frequency (e.g. 14.074 MHz)
2. Set TX frequency in the waterfall to a clear spot
3. Check **"Enable Tx"** and click **"CQ"**
4. WSJT-Zii transmits on the next even or odd period

A standard CQ message: `CQ 2E0LXY IO93`

### Answering a CQ

1. Double-click the CQ call in the decoded messages panel
2. Your DX Call and grid are automatically set
3. WSJT-Zii responds with: `2E0LXY G3ABC IO91`
4. The QSO proceeds automatically through Report → R+Report → RRR → 73

### Standard QSO sequence

```
CQ 2E0LXY IO93          ← CQ station transmits
2E0LXY G3ABC IO91       ← You answer with your call and grid
G3ABC 2E0LXY -14        ← CQ station gives signal report
2E0LXY G3ABC R-11       ← You confirm and give report
G3ABC 2E0LXY RRR        ← CQ station confirms receipt
2E0LXY G3ABC 73         ← You sign off
```

---

## Decoding

WSJT-Zii runs the decoder at the end of each 15-second period. Decoded stations appear in the panel below the waterfall.

Columns:
- **UTC** — time of decode (e.g. `1425`)
- **dB** — signal-to-noise ratio (−26 dB = very weak, 0 dB = strong)
- **DT** — time offset from expected (±2.0 s is acceptable)
- **Freq** — audio frequency in Hz
- **Message** — decoded text

### Decode colours

| Colour | Meaning |
|--------|---------|
| Green | Your callsign present |
| Red | CQ from a new DXCC entity |
| Blue | Worked before (in log) |
| Cyan | Callsign on your watchlist |
| White | Normal |

---

## Sensitivity modes

WSJT-Zii has three sensitivity levels accessible from **Decode → Sensitivity**:

| Mode | syncmin | OSD ndeep | Use |
|------|---------|-----------|-----|
| **Normal** | 1.3 | 3/4 | Standard operating, fast decode |
| **Low thresholds** | 0.9–1.225 | 5/6 | DXing, propagation monitoring |
| **Subpass** | 0.9 | 6 | Maximum sensitivity; slower |

The Shannon limit for FT8 is −27.5 dB S/N. WSJT-Zii's high-sensitivity mode approaches −25 dB.

> **Note:** Sensitivity below −26 dB is not achievable for FT8 regardless of algorithm — this is a hard physical limit set by the 77-bit payload, 50 Hz bandwidth and 12.64 s transmission time.

---

## Auto Sequence (auto-call)

WSJT-Zii can operate fully automatically:

1. Tick **"Auto Seq"** in the main window
2. Tick **"Enable Tx"**
3. When a CQ is decoded from a desired station, WSJT-Zii responds automatically and works through the full QSO sequence

**Filters** limit which CQs are automatically answered. See [WSJT-Zii Features](WSJT-Zii-Features#signal-filtering).

---

## Split operation and contest frequencies

On a busy contest weekend, the main FT8 frequency (e.g. 14.074 MHz) may have hundreds of stations. Use the **second FT8 frequency** where one exists:

| Band | Primary | Secondary |
|------|---------|-----------|
| 40 m | 7.074 | 7.078 |
| 20 m | 14.074 | 14.090 |
| 15 m | 21.074 | 21.091 |
| 10 m | 28.074 | 28.180 |

---

## PSK Reporter

WSJT-Zii can upload spots to **PSK Reporter** (https://pskreporter.info) automatically. Enable in **Settings → Reporting → PSK Reporter**.

PSK Reporter shows a real-time map of what you are receiving and who is receiving you — very useful for checking propagation.

---

## Log entries

A contact is logged when the **73** or **RRR** message is confirmed. The ADIF log (`wsjtx_log.adi`) is written to your configured log directory.

Each entry includes:
- Callsign, band, mode
- Signal reports (both directions)
- 6-character grid locator
- UTC time
- DXCC entity and CQ zone (auto-resolved)

---

## Tips

- Keep your audio level stable — avoid ALC compression
- Transmit on an uncrowded frequency (watch the waterfall for clear gaps)
- A 6-character grid locator in your CQ message helps stations calculate beam headings
- Turn off **"Tx watchdog"** if running unattended monitoring, or set it to ≥10 minutes
- Use **"Hold Tx freq"** to stay on a fixed transmit frequency across multiple QSOs
