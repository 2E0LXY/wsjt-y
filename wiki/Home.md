# WSJT-Zii

**WSJT-Zii** is an independent fork of [WSJT-Z](https://github.com/sq9fve/wsjt-z) by SQ9FVE, which is itself based on [WSJT-X](https://physics.princeton.edu/pulsar/k1jt/wsjtx.html) by K1JT et al.

Maintained by **2E0LXY** (Daren Loxley, Wakefield, West Yorkshire, UK).

---

## What is WSJT-Zii?

WSJT-Zii supports the following digital modes for amateur radio weak-signal communication:

| Mode | T/R Period | Typical Sensitivity | Use Case |
|------|-----------|---------------------|----------|
| **FT8** | 15 s | −21 dB (−25 dB high-sens) | DX, contest, everyday |
| **FT4** | 7.5 s | −17 dB | Fast contest operation |
| **JT65** | 60 s | −25 dB | HF DX, EME |
| **JT9** | 60 s | −27 dB | HF narrowband DX |
| **JT4** | 60 s | −33 dB | VHF/UHF EME |
| **WSPR** | 120 s | −31 dB | Propagation beaconing |
| **MSK144** | 2 s | −1 dB | Meteor scatter |
| **FST4** | 15–1800 s | −40 dB | Very weak-signal HF |
| **Q65** | 15–60 s | −25 dB | EME, aircraft scatter |

WSJT-Zii adds beyond the base WSJT-X/Z feature set:

- **Multi-threaded FT8 decoder** — scans more candidates in parallel
- **Auto-call / auto-sequence** — automatically responds to CQ calls
- **Band-hopper** — scheduled frequency hopping for propagation monitoring
- **Signal filtering** — configurable callsign and DX entity filters
- **QRM stop** — halts TX when mutual QRM detected
- **Improved receive sensitivity** — OSD ndeep 6/7, LDPC iterations 50, lower sync threshold

---

## Quick Links

| | |
|---|---|
| **Latest release** | https://github.com/2E0LXY/wsjt-zii/releases/latest |
| **Source code** | https://github.com/2E0LXY/wsjt-zii |
| **Bug reports** | https://github.com/2E0LXY/wsjt-zii/issues |

---

## Wiki Contents

- **[Installation](Installation)** — Windows and Linux install guide
- **[Getting Started](Getting-Started)** — first-run setup and audio configuration
- **[FT8 Operating Guide](FT8-Operating-Guide)** — complete FT8 reference
- **[Modes Reference](Modes-Reference)** — all supported modes
- **[Configuration Reference](Configuration-Reference)** — every settings option
- **[WSJT-Zii Features](WSJT-Zii-Features)** — features beyond WSJT-X
- **[Building from Source](Building-from-Source)** — Linux and Windows build guide
- **[Windows Build Pitfalls](Windows-Build-Pitfalls)** — MSYS2/MinGW technical reference
- **[Changes from WSJT-Z](Changes-from-WSJT-Z)** — bugs fixed, improvements
- **[Changelog](Changelog)** — version history
- **[Troubleshooting](Troubleshooting)** — common problems and fixes
- **[Contributing](Contributing)** — how to submit patches
- **[Licence](Licence)** — GPL-3.0 and attribution
