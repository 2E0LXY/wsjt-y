# Getting Started

This guide covers first-run configuration of WSJT-Zii: audio, station details, and radio CAT control.

---

## First Launch

On first launch the **Settings** dialog opens automatically. Work through each tab in order.

---

## Station tab

| Field | What to enter |
|-------|--------------|
| **My Call** | Your full callsign, e.g. `2E0LXY` |
| **My Grid** | Your 6-character Maidenhead locator, e.g. `IO93GV` |
| **IARU Region** | Select your region (1 = Europe/Africa, 2 = Americas, 3 = Asia/Pacific) |

To find your grid locator: https://www.levinecentral.com/ham/grid_square.php

---

## Audio tab

### Sound input (receive)

Select the **audio input device** connected to your radio's audio output (discriminator, speaker, or headphone jack).

> **Important:** Use the radio's **fixed-level** audio output (rear-panel accessory socket or DATA port), not the front headphone jack. The headphone level changes when you adjust the volume control, which will break automatic level calibration.

Recommended input level: the waterfall should show noise floor at around 40–50 dB down from full scale.

### Sound output (transmit)

Select the **audio output device** connected to your radio's microphone or data input.

### Sample rate

Leave at **48000 Hz** unless your soundcard does not support it. 44100 Hz is also supported.

---

## Radio tab

### CAT control (recommended)

CAT control allows WSJT-Zii to automatically switch frequency and report your band to PSK Reporter.

1. Set **Rig** to your transceiver model (e.g. `Icom IC-7300`, `Yaesu FT-991A`, `Kenwood TS-890S`)
2. Set **Serial port** to the COM port (Windows) or `/dev/ttyUSB0` (Linux) of your CAT cable
3. Set **Baud rate**, **Data bits**, **Stop bits**, **Handshake** to match your radio's CAT specification
4. Click **Test CAT** — the button should turn green

### PTT

| Method | When to use |
|--------|------------|
| **CAT** | Best option if your radio supports PTT via CAT |
| **DTR** / **RTS** | Use a serial port signal line; connect to radio PTT input |
| **VOX** | Simplest but no key-up delay control; not recommended |
| **None** | If using a radio with built-in VOX or data mode auto-PTT |

### Split operation

For FT8 contest and DX work, **Split mode: Fake It** is recommended. WSJT-Zii uses the RIT/XIT of the radio to transmit slightly offset from receive, reducing QRM to other stations on the calling frequency.

---

## Frequencies tab

WSJT-Zii ships with the standard WSJT-X frequency table. Common FT8 frequencies:

| Band | FT8 Dial Frequency |
|------|--------------------|
| 160 m | 1.840 MHz |
| 80 m | 3.573 MHz |
| 60 m | 5.357 MHz |
| 40 m | 7.074 MHz |
| 30 m | 10.136 MHz |
| 20 m | 14.074 MHz |
| 17 m | 18.100 MHz |
| 15 m | 21.074 MHz |
| 12 m | 24.915 MHz |
| 10 m | 28.074 MHz |
| 6 m | 50.313 MHz |
| 2 m | 144.174 MHz |

---

## Log setup

WSJT-Zii can log contacts to:

- **ADIF file** — automatically saved to `wsjtx_log.adi` in your log directory
- **Cabrillo** — for contest log export
- **UDP** — real-time export to logging software (N1MM+, Log4OM, etc.)

For UDP logging, go to **Settings → Reporting** and set the server address and port for your logging application.

### N1MM+ integration

In N1MM+, enable **UDP Broadcast → WSJT-X** on port 2237. In WSJT-Zii set **Reporting → N1MM+ Server** to `localhost:2237`.

---

## Transmit power

Set your transmitter to the **minimum power needed** for reliable contacts — typically 5–50 W on HF.

> FT8 is designed for low power. Running 1.5 kW into an omnidirectional antenna is unnecessary and causes QRM to dozens of other stations.

The ALC meter should remain **off or barely deflecting**. ALC compression on FT8 causes spurious emissions.

---

## Next steps

- **[FT8 Operating Guide](FT8-Operating-Guide)** — making your first contact
- **[WSJT-Zii Features](WSJT-Zii-Features)** — auto-call, filters, and band-hopper
- **[Troubleshooting](Troubleshooting)** — if something isn't working
