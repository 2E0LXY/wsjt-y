# Modes Reference

All modes supported by WSJT-Zii with key parameters and recommended uses.

---

## FT8

| Parameter | Value |
|-----------|-------|
| T/R period | 15 seconds |
| Transmission time | 12.64 s |
| Symbol rate | 6.25 baud |
| Modulation | 8-FSK |
| Occupied bandwidth | 50 Hz |
| Payload | 77 bits |
| FEC | (174,91) LDPC |
| Sensitivity (standard) | −21 dB S/N |
| Sensitivity (WSJT-Zii high-sens) | −25 dB S/N |
| Shannon limit | −27.5 dB S/N |
| Typical use | HF DX, 160–2 m |

Standard dial frequencies: see [FT8 Operating Guide](FT8-Operating-Guide#standard-frequencies).

---

## FT4

| Parameter | Value |
|-----------|-------|
| T/R period | 7.5 seconds |
| Transmission time | 4.48 s |
| Symbol rate | 20.833 baud |
| Modulation | 4-FSK |
| Occupied bandwidth | 90 Hz |
| Payload | 77 bits |
| Sensitivity | −17 dB S/N |
| Typical use | Fast contest operation |

FT4 is ~7 dB less sensitive than FT8 but completes a full QSO in 30 seconds (4 exchanges). Used in high-activity contest situations.

---

## JT65

| Parameter | Value |
|-----------|-------|
| T/R period | 60 seconds |
| Symbol rate | 2.692 baud |
| Modulation | 65-FSK + sync |
| Occupied bandwidth | 177 Hz |
| Sensitivity | −25 dB S/N |
| Typical use | HF DX, EME on VHF |

JT65 supports sub-modes (A, B, C) for different bands:
- **JT65A** — HF, 200 Hz BW
- **JT65B** — VHF, 400 Hz BW  
- **JT65C** — microwave EME, 800 Hz BW

---

## JT9

| Parameter | Value |
|-----------|-------|
| T/R period | 60 seconds |
| Modulation | 9-FSK |
| Occupied bandwidth | 15.6 Hz |
| Sensitivity | −27 dB S/N |
| Typical use | Narrowband HF DX, LF/MF |

JT9 is ~2 dB more sensitive than JT65 and 10× narrower bandwidth. Excellent for 30 m and below where spectrum is congested.

Sub-modes JT9A through JT9H vary the symbol rate for different propagation conditions.

---

## JT4

| Parameter | Value |
|-----------|-------|
| T/R period | 60 seconds |
| Modulation | 4-FSK |
| Sub-modes | A (4.375 Hz tone spacing) through G |
| Sensitivity | −33 dB S/N (sub-mode A) |
| Typical use | Microwave EME |

JT4G (315 Hz spacing) is used for 10 GHz+ EME where Doppler spread is large.

---

## WSPR

| Parameter | Value |
|-----------|-------|
| T/R period | 120 seconds |
| Modulation | 4-FSK |
| Occupied bandwidth | 6 Hz |
| Payload | 50 bits (call, grid, power) |
| Sensitivity | −31 dB S/N |
| Typical use | Automated propagation beaconing |

WSPR (Weak Signal Propagation Reporter) is for beaconing and propagation mapping. It does not support two-way QSOs. Spots are automatically uploaded to wsprnet.org.

**Standard WSPR frequencies (USB dial):**

| Band | Frequency |
|------|-----------|
| 160 m | 1.836600 MHz |
| 80 m | 3.568600 MHz |
| 40 m | 7.038600 MHz |
| 30 m | 10.138700 MHz |
| 20 m | 14.095600 MHz |
| 17 m | 18.104600 MHz |
| 15 m | 21.094600 MHz |
| 10 m | 28.124600 MHz |

---

## MSK144

| Parameter | Value |
|-----------|-------|
| T/R period | 15 seconds |
| Modulation | MSK (minimum shift keying) |
| Symbol rate | 2000 baud |
| Sensitivity | −1 dB S/N |
| Typical use | Meteor scatter on 2 m / 70 cm |

MSK144 uses very short bursts (72 ms to 900 ms) to catch brief ionisation trails from meteors. Best used on the Perseids (August) and Geminids (December) meteor showers.

---

## FST4 and FST4W

| Parameter | Value |
|-----------|-------|
| T/R periods | 15, 30, 60, 120, 300, 900, 1800 s |
| Modulation | 4-FSK |
| Sensitivity (1800 s) | −40 dB S/N |
| Typical use | LF/MF/HF very weak signal; WSPR replacement |

FST4W (W = WSPR-like) is a narrowband beacon mode for LF/MF. FST4 supports two-way QSOs with longer T/R periods. The 1800-second period achieves −40 dB — this is the only mode in WSJT-Zii that approaches −40 dB sensitivity, and it requires 30 minutes per QSO.

> **Note:** The −40 dB target is only achievable with FST4 1800 s period. FT8 has a hard Shannon limit of −27.5 dB — no algorithm improvement can break this.

---

## Q65

| Parameter | Value |
|-----------|-------|
| T/R periods | 15, 30, 60, 120, 300 s |
| Modulation | 64-FSK |
| Sensitivity | −25 dB S/N (30 s) |
| Typical use | EME, aircraft scatter, microwave |

Q65 is the recommended mode for two-way EME contacts. It tolerates larger frequency/Doppler spreads than other modes and adapts to different sub-bands.

---

## Echo

A single-mode used to test your EME path by receiving your own signal off the Moon.

---

## Mode selection guidelines

| Scenario | Recommended mode |
|----------|-----------------|
| Everyday HF DX | FT8 |
| Contest QSO rate | FT4 |
| Narrowband 30 m / LF | JT9 |
| EME (any band) | Q65 or JT65 |
| Meteor scatter 2 m | MSK144 |
| Propagation mapping | WSPR |
| Very-weak-signal LF | FST4W (1800 s) |
