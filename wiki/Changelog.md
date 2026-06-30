# Changelog

---

## v3.0.0-r2 — 1 July 2026 *(post-release improvements)*

All changes are backward-compatible rolling improvements on the v3.0.0 release tag.
Installers at the v3.0.0 release page are continuously updated.

### Decoder sensitivity improvements
- **All modes — LDPC BP iterations raised** across every mode that had the default 30-iteration ceiling:
  FT8 (ft8bvar + ft8b), FT4, FT2, FST4 (240/101 + 240/74), MSK144 (batch + real-time), WSPR.
  Recovering approximately 0.3 dB on marginal signals at no measurable cost on modern hardware.
- **WSPR OSD depth extended to order-4** (`osdwspr.f90` ndeep=6, C(50,4)≈230k candidates).
  Exposed via a new **Decode > Maximum** menu tier passing `-o 6` to wsprd.
- **FT8 NMS decoder** (`bpdecode174_91var_nms.f90`): experimental Normalized Min-Sum check-node
  update (α=0.75, sign×min×α). Gated behind `WSJTZ_USE_NMS_DECODER=1` env var.
  Not the default — needs on-air validation first.

### UI improvements
- **View > Show callsigns on waterfall** — decoded callsigns overlaid on the 2D spectrum at each
  signal's audio frequency as semi-transparent pill labels. Matches WSJT-X 3.1.0 feature.
  State persisted across restarts.
- **Dark mode** — View > Dark mode already available (QDarkStyleSheet, vendored).

### CAT / rig control
- **OmniRig re-enabled** — previously disabled due to CI build issues.
  Fixed: `mingw-w64-x86_64-qt5-activeqt` package, `dumpcpp-qt5.exe`→`dumpcpp.exe`,
  WOW64 registry bypass via `OmniRig.tlb` fetched from VE3NEA/OmniRig,
  narrowing conversion fix in `OmniRigTransceiver.cpp`.

### Infrastructure
- Wiki sync workflow fixed (bootstrap on first run).
- Windows build pitfalls documented (9 pitfalls, `docs/WINDOWS-BUILD-PITFALLS.md`).

---

## v3.0.0 — 29 June 2026

First release of the **2E0LXY** fork. Based on sq9fve/wsjt-z v2.0.16 (WSJT-X 3.0.0 base).

### Highlights
- 9 upstream bugs fixed
- 4 FT8 receive sensitivity improvements
- Full GitHub Actions CI producing Linux .deb and Windows .exe
- Windows DLL ABI issues resolved (static-linked runtime)
- Complete WSJT-Zii identity and branding

See [Changes from WSJT-Z](Changes-from-WSJT-Z) for the full change log.

### Downloads
- `wsjtz-3.0.0-2.0.16-win64.exe` — Windows 10+ 64-bit installer
- `wsjtz_3.0.0-2.0.16_amd64.deb` — Debian 12 / Ubuntu 22.04+ package

---

## Upstream history

### sq9fve/wsjt-z v2.0.16 (inherited)
- Multi-threaded FT8 decoder
- Auto-call, filters, band-hopper
- Based on WSJT-X 3.0.0

### WSJT-X 3.0.0 (upstream base)
- FT8, FT4, JT65, JT9, JT4, WSPR, MSK144, FST4, Q65 modes
- Hamlib-based CAT and PTT control
- PSK Reporter integration
- ADIF logging
