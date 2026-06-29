# Changes from WSJT-Z

All changes in WSJT-Zii relative to [sq9fve/wsjt-z](https://github.com/sq9fve/wsjt-z) v2.0.16 (based on WSJT-X 3.0.0).

---

## v3.0.0 — 29 June 2026

### Bug fixes

| # | File | Description |
|---|------|-------------|
| 1 | `all2cab.py` | **Python 2 `print` statements** — tool crashed immediately on Python 3 due to `print "..."` syntax |
| 2 | `all2cab.py` | **`f.close` missing parentheses** — file was never closed; descriptor leaked on every run |
| 3 | `UDPExamples/decode_WSJT-UDP.py` | **Wrong null-QDate sentinel** — null `QDate` checked as `-1` but Qt serialises it as `std::numeric_limits<qint64>::min()` = `-(2**63)` |
| 4 | `UDPExamples/decode_WSJT-UDP.py` | **Multicast socket bound to group address** — `sock.bind((mcast_group, port))` fails on Windows and some Linux configurations; fixed to `('', port)` (INADDR_ANY) |
| 5 | `widgets/mainwindow.cpp` | **Missing `superFox()` guard in QRM-stop** — `auto_qrm_guard_state` did not exclude Fox mode, causing auto-halt to fire during DXpedition Fox operation (developer TODO resolved) |
| 6 | `map65/libm65/decode0.f90` | **`NFFT` undeclared as array bound** — GFortran 14 enforces Fortran 90 scope rules; `NFFT=32768` was used as an array dimension without being declared in that scope |
| 7 | `Transceiver/TransceiverFactory.cpp` | **OmniRig include/enum/case not guarded by `WSJT_NO_OMNIRIG`** — all three `#if WIN32` blocks now check `!defined(WSJT_NO_OMNIRIG)`; `OmniRig.h` is generated only when OmniRig is enabled |
| 8 | `CMakeLists.txt` | **Missing `-fallow-argument-mismatch`** — MAP65 Fortran code uses pre-F90 type-punning (byte arrays passed as `REAL(4)`); GFortran 14 rejects this without the flag |
| 9 | `CMake/Modules/FindFFTW3.cmake` | **`NOT WIN32` condition excluded fftw3f_threads on Windows** — MSYS2 FFTW3 is modular; the separate `libfftw3f_threads.dll.a` was never linked, causing `fftwf_init_threads` link error |

### Receive sensitivity improvements

| # | File | Change | Estimated gain |
|---|------|--------|---------------|
| A | `ft8bvar.f90` | LDPC max_iterations 30→50 | ~0.3 dB |
| B | `osd174_91var.f90`, `osd174var.f90` | OSD ndeep=6 added (nt=80, ntheta=20, ntau=24); used in `nagainfil` mode | ~0.8 dB |
| C | `osd174_91var.f90`, `osd174var.f90` | OSD ndeep=7 added (nt=120, ntheta=30, ntau=27); available in monitor mode | ~1.5 dB |
| D | `ft8_decodevar.f90` | Sync threshold floor on residual pass lowered from 1.1→0.9 | ~0.8 dB |

Combined improvement: up to ~2–3 dB over standard WSJT-Z in high-sensitivity modes. Hard Shannon limit for FT8 (50 Hz, 77 bits, 12.64 s) is −27.5 dB.

### Build and CI

- GitHub Actions workflows for **Linux (.deb)** and **Windows (NSIS .exe)** added
- `WSJT_WITH_OMNIRIG` / `WSJT_NO_OMNIRIG` CMake option for CI-compatible builds
- `GCC_RUNTIME_OVERRIDE_DIR` removed — replaced by `-static-libgcc -static-libstdc++ -static-libgfortran`
- Hamlib 4.5.5 built from source in Windows CI
- GCC compiler pinned to 14.2.0-3 in Windows CI
- `-fallow-argument-mismatch` added to `General_FFLAGS`
- `FindFFTW3.cmake` `NOT WIN32` condition removed
- Windows DLL ABI crashes resolved via static runtime linking
- Windows build pitfalls documented in `docs/WINDOWS-BUILD-PITFALLS.md`

### Identity / branding

Rebranded to **WSJT-Zii**, independent of sq9fve/wsjt-z:

- Title bar: `WSJT-Z MOD by SQ9FVE` → `WSJT-Zii by 2E0LXY`
- About dialog: groups.io / sourceforge links → 2E0LXY GitHub
- PSK Reporter user-agent: `WSJT-X` → `WSJT-Zii`
- NSIS installer: ProductName, FileDescription, menu links updated
- CMake: PROJECT_VENDOR, PROJECT_CONTACT, PROJECT_HOMEPAGE, PROJECT_COPYRIGHT updated
- Example callsign in tooltip: SQ9FVE → 2E0LXY

---

## Inherited from WSJT-Z (sq9fve)

The following features are inherited from WSJT-Z and are not changes by 2E0LXY:

- Multi-threaded FT8 decoder
- Auto-call / Auto-sequence
- Band-hopper
- Signal and callsign filters
- QRM-stop
- Message tombstoning
- OTP generator

See the [WSJT-Zii Features](WSJT-Zii-Features) page for how these features work.
