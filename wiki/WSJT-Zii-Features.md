# WSJT-Zii Features

Features added or improved by WSJT-Z and WSJT-Zii beyond the base WSJT-X implementation.

---

## Multi-threaded FT8 decoder

WSJT-Zii runs multiple decoder threads in parallel during each 15-second receive period. This allows:

- More decode candidates examined per period
- Better performance on multi-core systems
- Higher decode success rate on weak signals

Configure in **Settings → WSJT-Zii → FT8 Threads**. Default matches your CPU core count.

---

## Improved receive sensitivity

WSJT-Zii v3.0.0 implements four sensitivity improvements over the base decoder:

### 1. LDPC iterations (50 vs 30)
The belief-propagation LDPC decoder runs 50 iterations instead of 30. Each additional iteration can recover signals the previous step missed. Gain: ~0.3 dB at negligible compute cost.

### 2. OSD depth levels 6 and 7
Ordered Statistics Decoding (OSD) now has two new depth levels:
- **ndeep=6**: order-2 OSD with wider search (nt=80, ntheta=20, ntau=24) — ~0.8 dB gain over ndeep=5
- **ndeep=7**: maximum order-2 OSD (nt=120, ntheta=30, ntau=27) — ~1.5 dB gain; available in monitor mode

These are automatically used in high-sensitivity and repeat-scan modes.

### 3. Sync threshold reduction (1.1 → 0.9)
The minimum synchronisation threshold on the final residual pass has been reduced from 1.1 to 0.9. This allows approximately 0.8 dB weaker signals to enter the decode chain on the residual pass (after strong signals have been subtracted), with low false-positive risk.

### 4. Sensitivity in context
| Mode | Approx. sensitivity |
|------|-------------------|
| FT8 standard | −21 dB S/N |
| FT8 low thresholds | −23 dB S/N |
| FT8 subpass (WSJT-Zii) | −25 dB S/N |
| Shannon limit (FT8) | −27.5 dB S/N |

Sensitivity below −27.5 dB is physically impossible for FT8 — it would require more channel capacity than the 50 Hz / 12.64 s / 77-bit protocol provides.

---

## Auto-call (Auto Sequence)

Enable **Auto Seq** to let WSJT-Zii work contacts automatically.

When enabled:
1. Decoded CQ calls matching your filters are automatically answered
2. The QSO sequence (report exchange → RRR → 73) completes without intervention
3. The log entry is written automatically

**Auto-call filters** prevent WSJT-Zii from answering every CQ:
- DXCC entity whitelist / blacklist
- Callsign prefix filter
- Minimum signal threshold
- "Worked before" exclusion

---

## Signal filtering

**Settings → WSJT-Zii → Filters**

### Callsign filtering
Define a list of callsign prefixes or full calls that should (or should not) appear in the decoded messages panel. Useful for monitoring specific DX or for contest exchanges.

### DX entity filter
Show only callsigns from specific DXCC entities, or hide entities you've already worked on this band/mode.

### Threshold filter
Hide decodes below a configurable S/N threshold (e.g. hide all signals below −20 dB to reduce clutter).

---

## Band-hopper

The band-hopper schedules automatic frequency changes at configurable intervals, useful for:
- Automated propagation monitoring across multiple bands
- Unattended WSPR beaconing on a rotating schedule

Configure in **Settings → WSJT-Zii → Band Hopper**:
- List of bands/frequencies to visit
- Dwell time per frequency
- TX enable per band

---

## QRM-stop

WSJT-Zii detects when it is causing mutual QRM with another station and can automatically halt transmission. This activates when:
- You are in a QSO attempt
- Another station's transmission overlaps your frequency
- The decode confirms QRM is occurring

The QRM-stop condition excludes Fox mode (DXpedition operating style) to prevent false halts.

---

## Message tombstoning

Once a message has been decoded and acted upon, WSJT-Zii marks it as "tombstoned" — it will not reappear in the active decode list for the current session. This prevents:
- Already-worked stations cluttering the display
- Auto-call re-initiating QSOs with stations you've already worked
- Duplicate log entries from repeated decode of the same signal

---

## OTP (one-time password) generator

A built-in TOTP generator is included for secure remote station access scenarios. Access via **Tools → OTP Generator**.

---

## Debug / diagnostic mode

Enable detailed logging via **Help → WSJT-Zii Debug**. The debug log records:
- Decoder timing and thread activity
- CAT commands sent and received
- Filter decisions (why a signal was shown/hidden)
- Auto-call state machine transitions

Log is written to `wsjtx_debug.log` in your configuration directory.
