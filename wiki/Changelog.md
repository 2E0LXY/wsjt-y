# Changelog

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
