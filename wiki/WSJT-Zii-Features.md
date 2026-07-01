# WSJT-Zii Features

Features added or improved by 2E0LXY's WSJT-Zii fork beyond the base WSJT-Z (sq9fve) and WSJT-X 3.0.0 implementation.

---

## Multi-threaded FT8 decoder

WSJT-Zii runs multiple decoder threads in parallel during each receive period. Configure in **Settings → WSJT-Zii → FT8 Threads**. Default matches your CPU core count.

---

## Sensitivity improvements

### LDPC BP iterations raised — all modes

The belief-propagation LDPC decoder now runs more iterations across every mode that previously used the default 30-iteration ceiling. More iterations mean the decoder can resolve ambiguities that earlier iterations left uncertain.

| Mode | Previous | Current |
|---|---|---|
| FT8 (OMP + non-OMP) | 30 | 50 |
| FT4 | 30–40 | 50–55 |
| FST4 (240/101 + 240/74) | 30 | 50 |
| MSK144 (batch + real-time) | 5–10 | 8–15 |
| WSPR | (OSD-only path) | (unchanged) |

### OSD search depth extended — FT8

Ordered Statistics Decoding now has two additional depth tiers for FT8 (`ft8var/osd174_91var.f90`):

- **ndeep=6**: order-3 OSD (nt=80, ntheta=20, ntau=24) — wider search than the original ndeep=5
- **ndeep=7**: order-4 OSD (nt=120, ntheta=30, ntau=27) — maximum depth, available in monitor/SWL mode

The OSD in `ft8/osd174_91.f90` (shared by FT4) now also supports ndeep=7, matching the FT8 path.

### OSD depth extended — FT4, FST4, MSK144

The `maxosd` pass counter caps in `decode174_91.f90`, `decode240_101.f90`, and `decode240_74.f90` were raised from 3 to 4, allowing one additional OSD pass per decode attempt. The underlying OSD backends already supported the extra depth.

`osd128_90.f90` (MSK144's OSD) gained a new ndeep=6 tier (order-2, K=90 so C(90,2)=4005 candidates — negligible compute cost).

### WSPR OSD depth — Maximum tier

`osdwspr.f90` has a new ndeep=6 tier (order-4 OSD, C(50,4)≈230k candidates — feasible in WSPR's 120s T/R period). Exposed via **Decode → Maximum** in the menu. The wsprd `-o` flag cap was raised from 5 to 6 to allow it.

### Experimental NMS decoder

`bpdecode174_91var_nms.f90` implements a Normalized Min-Sum check-node update (α=0.75) as an alternative to the default sum-product (tanh/atanh) decoder. Claimed gain: +0.2–0.4 dB. Enable with `WSJTZ_USE_NMS_DECODER=1` environment variable. **Not the default** — needs on-air validation first.

---

## Waterfall callsign overlay

**View → Show callsigns on waterfall** overlays decoded callsigns as small pill labels on the 2D spectrum display at each signal's audio frequency:

- **Blue**: CQ / DE / QRZ — the calling station
- **Green**: directed at your callsign
- **Grey**: directed at another station

State is saved across restarts.

---

## WSPR Decode depth — Maximum tier

A fourth decode depth tier (**Decode → Maximum**) passes `-o 6` to wsprd, enabling the order-4 OSD backend added in osdwspr.f90. This is significantly slower than Deep mode but may recover stations that no other tier decodes.

| Tier | wsprd flags | OSD ndeep |
|---|---|---|
| Quick | `-qB` | none |
| Medium | `-C 500 -o 4` | 4 |
| Deep | `-C 500 -o 4 -d` | 4 + more candidates |
| **Maximum** | `-C 500 -o 6 -d` | 6 (order-4) |

---

## OmniRig CAT control

OmniRig Rig 1 / Rig 2 appear in the **Settings → Radio → Rig** dropdown. Requires [OmniRig](http://www.dxatlas.com/OmniRig/) installed separately.

---

## Dark mode

**View → Dark mode** applies the QDarkStyleSheet to the entire application.

---

## 9 upstream bug fixes

Bugs fixed over the base wsjt-z fork: operator precedence in `ft8bvar.f90`, count/ratio type mix errors, `log10()` of non-positive value, `ctab[]` out-of-bounds, and five further correctness issues. Full list in [Changelog](Changelog).
