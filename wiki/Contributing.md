# Contributing

Contributions are welcome. This page covers how to report bugs, submit patches, and work with the codebase.

---

## Reporting bugs

1. Check [existing issues](https://github.com/2E0LXY/wsjt-zii/issues) first
2. Open a new issue with:
   - **OS** and **version** (e.g. Windows 10 64-bit, Ubuntu 22.04)
   - **WSJT-Zii version** (title bar shows this)
   - **Steps to reproduce**
   - **Expected vs actual behaviour**
   - **Debug log** if applicable — enable via **Help → WSJT-Zii Debug**

---

## Submitting patches

### Workflow

1. Fork the repository on GitHub
2. Create a feature branch: `git checkout -b fix/my-bug`
3. Make your changes
4. Test on both Linux and Windows if possible
5. Commit with a clear message describing what changed and why
6. Open a Pull Request against `master`

### Commit message format

```
type: short description (under 72 chars)

Longer explanation if needed. Describe what the bug was, why the fix
works, and any side effects or caveats.

Fixes #123
```

Types: `fix`, `feat`, `ci`, `docs`, `refactor`, `perf`

### Code style

- **C++:** follow the existing WSJT-X style (Qt conventions)
- **Fortran:** match indentation and naming of surrounding code
- **CMake:** use existing patterns for new options
- **Python:** Python 3 only; no `print` statements

---

## Architecture

```
wsjt-zii/
├── widgets/mainwindow.cpp    # Main UI — auto-call, filters, band-hopper
├── lib/ft8var/               # Multi-threaded FT8 decoder (wsjt-z specific)
│   ├── ft8_decodevar.f90     # Top-level FT8 decode orchestration
│   ├── ft8bvar.f90           # Per-candidate decode + OSD dispatch
│   ├── osd174_91var.f90      # Ordered Statistics Decoding
│   └── bpdecode174_91var.f90 # Belief-propagation LDPC decoder
├── lib/                      # WSJT-X mode decoders (JT65, JT9, WSPR, etc.)
├── map65/                    # MAP65 EME decoder
├── Transceiver/              # CAT control (Hamlib + OmniRig)
├── CMake/Modules/            # Custom CMake finders (FFTW3, Hamlib, etc.)
├── .github/workflows/        # CI/CD
├── wiki/                     # Wiki source (synced to GitHub wiki)
└── docs/                     # Technical documentation
    └── WINDOWS-BUILD-PITFALLS.md
```

### Key extension points

**Adding a new filter type:**
- `widgets/mainwindow.cpp` — `callsignFiltered()` function
- `Configuration.cpp` — add a new config option
- `Configuration.ui` — add UI widget

**Changing decoder sensitivity:**
- `lib/ft8var/ft8bvar.f90` — `ndeep` selection logic
- `lib/ft8var/osd174_91var.f90` — OSD depth/search parameters
- `lib/ft8var/ft8_decodevar.f90` — `syncmin` thresholds

**Adding a new band-hopper frequency:**
- Via the Settings UI only — no code change needed

---

## Testing

### Manual testing checklist (before submitting a PR)

- [ ] Linux build succeeds: `cmake -G Ninja .. && ninja`
- [ ] Windows build succeeds (or CI passes)
- [ ] FT8 decodes work correctly with a known audio file
- [ ] CAT test passes with a connected radio (if applicable)
- [ ] No new compiler warnings
- [ ] No regressions in existing functionality

### CI

Both workflows (`linux-deb.yml` and `windows-installer.yml`) run automatically on Pull Requests. A green CI is required before merge.

---

## Licence

WSJT-Zii is GPL-3.0. Any contribution must be compatible with GPL-3.0.

See [Licence](Licence) for full details.
