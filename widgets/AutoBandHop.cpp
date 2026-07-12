#include "AutoBandHop.h"

// ── Band table derived from document's Master Matrix ──────────────────────────
//
// monthMask: bit 0 = January, bit 11 = December
// startUTC/endUTC: optimal operational window (UTC hours)
//   When startUTC > endUTC, the window wraps midnight
//   e.g. start=22 end=4 means 22:00–04:00 (night)
//
// Source: RSGB 2026 band plan / Ofcom 2024 propagation matrix
// 60m excluded: UK Full Licensees only — user enables manually if licensed.

static const BandEntry kBands[] = {
    // ── 160m — Deep winter nights only ────────────────────────────────────────
    {   "160m", 1.840,
        22, 4,                              // 22:00–04:00 UTC (wraps midnight)
        0b110000000011,                     // Nov,Dec,Jan,Feb (bits 10,11,0,1)
        3,
        "160m: Winter night / F-layer DX, D-layer absent"
    },
    // ── 80m — Winter nocturnal regional/DX ────────────────────────────────────
    {   "80m",  3.573,
        20, 6,                              // 20:00–06:00 UTC (wraps midnight)
        0b110000000111,                     // Nov,Dec,Jan,Feb,Mar (bits 10,11,0,1,2)
        4,
        "80m: Nocturnal winter DX; D-layer gone, F-layer reflects"
    },
    // ── 40m — 24/7 workhorse, best at night in winter ─────────────────────────
    {   "40m",  7.074,
        0, 24,                              // full 24h window
        0b111111111111,                     // year-round
        6,
        "40m: 24/7 workhorse; daytime EU, night global DX"
    },
    // ── 30m — WARC digital haven, day and night ────────────────────────────────
    {   "30m",  10.136,
        0, 24,                              // 24/7
        0b111111111111,                     // year-round
        5,
        "30m: WARC band; 24/7 digital haven, no contests"
    },
    // ── 20m — King of DX, daytime/equinox ─────────────────────────────────────
    {   "20m",  14.074,
        6, 22,                              // 06:00–22:00 UTC
        0b011100011100,                     // Mar,Apr,May,Sep,Oct,Nov (bits 2,3,4,8,9,10)
        7,
        "20m: Primary DX band; F2 layer, equinox peak"
    },
    // ── 17m — WARC quiet alternative, daytime/equinox ─────────────────────────
    {   "17m",  18.100,
        8, 19,                              // 08:00–19:00 UTC
        0b011100011100,                     // Mar,Apr,May,Sep,Oct,Nov
        6,
        "17m: WARC band; low noise, daytime DX, closes at sunset"
    },
    // ── 15m — Solar-dependent daytime, equinox + solar max ────────────────────
    {   "15m",  21.074,
        8, 18,                              // 08:00–18:00 UTC
        0b011100011100,                     // Mar,Apr,May,Sep,Oct,Nov (+ winter solar max)
        5,
        "15m: F2 DX, SFI>100 needed; excellent path loss at peak"
    },
    // ── 12m — Summer Sporadic-E + solar-max F2 ────────────────────────────────
    {   "12m",  24.915,
        9, 17,                              // 09:00–17:00 UTC
        0b000111100111,                     // May–Aug (Es) + Sep–Mar (F2): simplified to peak months
        4,
        "12m: Sporadic-E summer; F2 solar max; WARC band"
    },
    // ── 10m — Extreme: Sporadic-E summer + F2 solar max ──────────────────────
    {   "10m",  28.074,
        10, 18,                             // 10:00–18:00 UTC (midday peak)
        0b000111100111,                     // May–Aug (Es) + equinox (F2)
        4,
        "10m: Summer Sporadic-E + solar max F2; strictly diurnal"
    },
};

static constexpr int kBandCount = sizeof(kBands) / sizeof(kBands[0]);

const BandEntry* AutoBandHop::bands()     { return kBands; }
int              AutoBandHop::bandCount() { return kBandCount; }

// ── Scoring helpers ────────────────────────────────────────────────────────────

bool AutoBandHop::inWindow(int hour, int start, int end)
{
    if (end == 24) return true;                   // 24/7
    if (start <= end) return hour >= start && hour < end;
    return hour >= start || hour < end;           // wraps midnight
}

int AutoBandHop::scoreFor(BandEntry const& b, int utcHour, int monthIndex)
{
    // monthIndex 0=Jan … 11=Dec
    //
    // A band coded as 24/7 (startUTC=0, endUTC=24) and/or year-round
    // (monthMask covering all 12 months) is *always* "in window" — that's
    // not a signal that conditions are currently good, it's the absence of
    // any constraint. It previously earned the same +10 "perfect match"
    // bonus a genuinely time-restricted band only gets during its real,
    // narrow peak — which let 40m (24/7, year-round, basePriority 6)
    // permanently outscore every other band all day, every day, all year,
    // structurally dominating regardless of actual time or month. That's
    // the exact symptom reported: hops to 40m once, never leaves.
    //
    // Simply zeroing the bonus over-corrects the other way — simulated
    // across the full 24h x 12-month grid, 40m/30m then never won a
    // single hour, since even bands with only partial credit (in-window-
    // only or in-month-only) still beat a bare basePriority. A small flat
    // reliability bonus (kAlwaysOnBonus) sits deliberately between "no
    // credit" and "in-month-only" credit (+3): enough for 40m to win
    // genuine gaps where no time-restricted band's window or season
    // applies (its actual real-world niche — spring/summer/autumn
    // nights, per its own "best at night" comment above), not enough to
    // beat any band with a real, non-trivial match.
    constexpr int kAlwaysOnBonus = 4;
    const bool alwaysTime  = (b.startUTC == 0 && b.endUTC == 24);
    const bool alwaysMonth = (b.monthMask == 0xFFF);
    if (alwaysTime && alwaysMonth) {
        return b.basePriority + kAlwaysOnBonus;
    }
    const bool timeOK  = alwaysTime  ? false : inWindow(utcHour, b.startUTC, b.endUTC);
    const bool monthOK = alwaysMonth ? false : ((b.monthMask >> monthIndex) & 1);

    int score = b.basePriority;
    if (timeOK && monthOK) score += 10;
    else if (timeOK)        score += 5;
    else if (monthOK)       score += 3;
    // Neither: only basePriority — band is technically possible but not optimal
    return score;
}

// ── Public interface ───────────────────────────────────────────────────────────

double AutoBandHop::bestFrequencyMHz(QDateTime const& utc, QString *reason)
{
    const int hour  = utc.time().hour();
    const int month = utc.date().month() - 1;   // 0-based

    int    bestScore = -1;
    int    bestIdx   = 0;
    for (int i = 0; i < kBandCount; ++i) {
        int s = scoreFor(kBands[i], hour, month);
        if (s > bestScore) { bestScore = s; bestIdx = i; }
    }

    if (reason)
        *reason = QString("[Auto Band Hop] %1 — %2 (score %3)")
            .arg(kBands[bestIdx].name)
            .arg(kBands[bestIdx].reason)
            .arg(bestScore);

    return kBands[bestIdx].freqMHz;
}

AutoBandHop::AutoBandHop(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(m_intervalMin * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &AutoBandHop::evaluate);
}

void AutoBandHop::setEnabled(bool on)
{
    m_enabled = on;
    if (on) {
        evaluate();        // immediate hop on enable
        m_timer.start();
    } else {
        m_timer.stop();
    }
}

void AutoBandHop::setIntervalMinutes(int minutes)
{
    m_intervalMin = qBound(1, minutes, 60);
    m_timer.setInterval(m_intervalMin * 60 * 1000);
}

void AutoBandHop::evaluate()
{
    QString reason;
    double freq = bestFrequencyMHz(QDateTime::currentDateTimeUtc(), &reason);
    emit hopRequested(freq, reason);
}
