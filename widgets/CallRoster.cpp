#include "CallRoster.h"
#include <cmath>
#include <algorithm>

CallRoster::CallRoster(QWidget *parent)
    : QWidget(parent)
{
    auto *vl = new QVBoxLayout(this);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(2);

    // Filter bar
    auto *topBar = new QHBoxLayout;
    m_filter = new QLineEdit; m_filter->setPlaceholderText("filter callsign / DXCC / continent…");
    m_filter->setStyleSheet("background:#060b12;border:1px solid #0d2035;color:#a0c8e0;padding:2px 4px;");
    topBar->addWidget(m_filter);
    m_watchInput = new QLineEdit; m_watchInput->setPlaceholderText("watch for callsign…");
    m_watchInput->setToolTip("When this callsign is seen: row highlights yellow, a notification "
                              "pops up, WSJT-Y beeps, then QSYs, enters the callsign, and starts calling.");
    m_watchInput->setMaximumWidth(120);
    m_watchInput->setStyleSheet("background:#241a06;border:1px solid #4a3a10;color:#e0c860;padding:2px 4px;");
    connect(m_watchInput, &QLineEdit::textChanged, this, [this](QString const& t) {
        m_watchCall = t.trimmed().toUpper();
        m_watchAlerted = false;
        rebuild();   // repaint highlight immediately if the call is already visible
    });
    topBar->addWidget(m_watchInput);
    m_stats = new QLabel("0 calls");
    m_stats->setStyleSheet("color:#4a7a9a;font-size:10px;padding:0 4px;");
    topBar->addWidget(m_stats);
    vl->addLayout(topBar);

    // Table — two side-by-side panes so more rows fit in the same height
    auto buildTable = [this]() -> QTableWidget* {
        auto *t = new QTableWidget(0, COL_COUNT, this);
        t->setHorizontalHeaderLabels({"Call","Grid","dB","Hz","km","°","DXCC","Cont","Message"});
        t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        t->horizontalHeader()->setSortIndicatorShown(true);
        t->horizontalHeader()->setStretchLastSection(true);
        t->horizontalHeader()->setSectionsClickable(true);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        // NOT setSortingEnabled(true): Qt's built-in per-header-click sort
        // only reorders rows within a single QTableWidget. With two tables
        // showing a single logical list split in half, that sorted each
        // table independently against whatever subset it happened to hold
        // BEFORE the sort — which is why clicking a header looked broken
        // (each pane sorted itself, not the combined list). We sort
        // m_entries ourselves in rebuild() before the left/right split.
        t->verticalHeader()->hide();
        t->setShowGrid(false);
        t->setAlternatingRowColors(false);
        t->setStyleSheet(
            "QTableWidget{background:#060b12;color:#c0d8e8;font-family:'Courier New';font-size:10px;"
            "selection-background-color:#0d3a5a;border:none;}"
            "QTableWidget::item{padding:1px 4px;border-bottom:1px solid #0a1828;}"
            "QHeaderView::section{background:#0a1828;color:#4a8ab0;font-size:10px;padding:2px 4px;"
            "border-bottom:1px solid #0d2035;border-right:1px solid #0d2035;}");
        t->setColumnWidth(COL_CALL, 72); t->setColumnWidth(COL_GRID, 44);
        t->setColumnWidth(COL_SNR,  30); t->setColumnWidth(COL_FREQ, 42);
        t->setColumnWidth(COL_DIST, 52); t->setColumnWidth(COL_BRG,  32);
        t->setColumnWidth(COL_DXCC, 88); t->setColumnWidth(COL_CONT, 38);
        return t;
    };
    m_tableLeft  = buildTable();
    m_tableRight = buildTable();
    connect(m_tableLeft->horizontalHeader(),  &QHeaderView::sectionClicked, this, &CallRoster::onHeaderClicked);
    connect(m_tableRight->horizontalHeader(), &QHeaderView::sectionClicked, this, &CallRoster::onHeaderClicked);

    auto *tablesRow = new QHBoxLayout;
    tablesRow->setSpacing(2);
    tablesRow->addWidget(m_tableLeft);
    tablesRow->addWidget(m_tableRight);
    vl->addLayout(tablesRow);

    m_summary = new QLabel("—");
    m_summary->setStyleSheet("background:#0a1828;color:#5090b0;font-size:10px;padding:2px 6px;border-top:1px solid #0d2035;");
    vl->addWidget(m_summary);

    connect(m_filter, &QLineEdit::textChanged, this, &CallRoster::rebuild);

    auto onCellClicked = [this](QTableWidget *table) {
        return [this, table](int row, int) {
            auto *ci = table->item(row, COL_CALL);
            if (!ci) return;
            const QString call = ci->text().trimmed();
            if (m_entries.contains(call)) {
                auto const& e = m_entries[call];
                emit callSelected(call, e.freq, e.grid);
            }
        };
    };
    connect(m_tableLeft,  &QTableWidget::cellClicked, this, onCellClicked(m_tableLeft));
    connect(m_tableRight, &QTableWidget::cellClicked, this, onCellClicked(m_tableRight));

    // Expire old entries every 30s
    m_expireTimer = new QTimer(this);
    m_expireTimer->setInterval(30000);
    connect(m_expireTimer, &QTimer::timeout, this, [this]{ expireOld(120); });
    m_expireTimer->start();
}

void CallRoster::setHomePos(double lat, double lon) { m_homeLat=lat; m_homeLon=lon; }

void CallRoster::addDecode(QString const& call, QString const& grid, int snr,
                            int freqHz, QString const& msg,
                            bool isCQ, bool forMe,
                            QString const& dxcc, QString const& cont)
{
    if (call.isEmpty()) return;
    auto &e = m_entries[call];
    e.call = call; e.grid = grid; e.snr = snr; e.freq = freqHz;
    e.msg = msg; e.isCQ = isCQ; e.forMe = forMe;
    e.dxcc = dxcc; e.continent = cont;
    e.lastSeen = QDateTime::currentDateTimeUtc();
    if (!grid.isEmpty()) {
        // Approximate grid → lat/lon for distance
        auto g = grid.toUpper();
        double lon0 = (g[0].unicode()-'A')*20.0-180.0;
        double lat0 = (g[1].unicode()-'A')*10.0-90.0;
        if (g.length()>=4) { lon0+=(g[2].unicode()-'0')*2; lat0+=(g[3].unicode()-'0'); lon0+=1; lat0+=0.5; }
        else { lon0+=10; lat0+=5; }
        e.distKm  = haversineKm(m_homeLat, m_homeLon, lat0, lon0);
        e.bearing = bearingDeg(m_homeLat, m_homeLon, lat0, lon0);
    }
    if (!m_watchCall.isEmpty() && !m_watchAlerted && call.compare(m_watchCall, Qt::CaseInsensitive) == 0) {
        m_watchAlerted = true;
        emit watchedCallSeen(call, freqHz, grid);
    }
    rebuild();
}

void CallRoster::clearAll() { m_entries.clear(); rebuild(); }

void CallRoster::expireOld(int maxSeconds)
{
    const auto cutoff = QDateTime::currentDateTimeUtc().addSecs(-maxSeconds);
    bool changed = false;
    for (auto it = m_entries.begin(); it != m_entries.end(); ) {
        if (it->lastSeen < cutoff) {
            if (!m_watchCall.isEmpty() && it->call.compare(m_watchCall, Qt::CaseInsensitive) == 0)
                m_watchAlerted = false;
            it = m_entries.erase(it); changed = true;
        }
        else ++it;
    }
    if (changed) rebuild();
}

void CallRoster::addRowTo(QTableWidget *table, RosterEntry const& e)
{
    const int row = table->rowCount();
    table->insertRow(row);

    QColor bg, fg;
    const bool isWatched = !m_watchCall.isEmpty() && e.call.compare(m_watchCall, Qt::CaseInsensitive) == 0;
    if (isWatched)          { bg = QColor(90,75,10);  fg = QColor(255,230,120); }  // watched call — yellow
    else if (e.forMe)       { bg = QColor(80,15,15);  fg = QColor(255,120,120); }  // dark red — calling ME
    else if (e.isCQ)        { bg = QColor(8,25,50);   fg = QColor(100,180,255); }  // dark blue — CQ
    else                    { bg = QColor(5,12,20); }                              // default

    auto item = [&](QString const& txt, Qt::Alignment align = Qt::AlignLeft|Qt::AlignVCenter) {
        auto *it = new QTableWidgetItem(txt);
        it->setBackground(bg);
        it->setTextAlignment(align);
        if (fg.isValid()) it->setForeground(fg);
        return it;
    };

    table->setItem(row, COL_CALL, item(e.call));
    table->setItem(row, COL_GRID, item(e.grid));
    table->setItem(row, COL_SNR,  item(QString::number(e.snr), Qt::AlignRight|Qt::AlignVCenter));
    table->setItem(row, COL_FREQ, item(QString::number(e.freq), Qt::AlignRight|Qt::AlignVCenter));
    table->setItem(row, COL_DIST, item(e.distKm>0 ? QString::number(int(e.distKm)) : QString(),
                                          Qt::AlignRight|Qt::AlignVCenter));
    table->setItem(row, COL_BRG,  item(e.bearing>0 ? QString::number(int(e.bearing))+"°" : QString(),
                                          Qt::AlignRight|Qt::AlignVCenter));
    table->setItem(row, COL_DXCC, item(e.dxcc));
    table->setItem(row, COL_CONT, item(e.continent));
    table->setItem(row, COL_MSG,  item(e.msg));
    table->setRowHeight(row, 16);
}

void CallRoster::onHeaderClicked(int col)
{
    if (col == COL_MSG) return;   // free text — not a meaningful sort key
    if (m_sortColumn == col)
        m_sortOrder = (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    else {
        m_sortColumn = col;
        m_sortOrder = Qt::AscendingOrder;
    }
    rebuild();
}

void CallRoster::rebuild()
{
    const QString flt = m_filter->text().trimmed().toUpper();
    m_tableLeft->setRowCount(0);
    m_tableRight->setRowCount(0);

    QList<RosterEntry const*> shown;
    int cqCount=0, forMeCount=0;
    for (auto const& e : m_entries) {
        if (!flt.isEmpty() &&
            !e.call.contains(flt, Qt::CaseInsensitive) &&
            !e.dxcc.contains(flt, Qt::CaseInsensitive) &&
            !e.continent.contains(flt, Qt::CaseInsensitive))
            continue;
        shown.append(&e);
        if (e.forMe) ++forMeCount;
        if (e.isCQ)  ++cqCount;
    }

    // Sort the WHOLE combined list first — the left/right split below is
    // purely a display device (reads top-to-bottom-left then top-to-bottom-
    // right, like a newspaper column) and must operate on an already-sorted
    // list, or each pane ends up independently "sorted" against a different
    // subset and the overall table looks unsorted.
    if (m_sortColumn >= 0) {
        const int col = m_sortColumn;
        const bool asc = (m_sortOrder == Qt::AscendingOrder);
        std::stable_sort(shown.begin(), shown.end(), [col](RosterEntry const* a, RosterEntry const* b) {
            switch (col) {
            case COL_CALL: return a->call.compare(b->call, Qt::CaseInsensitive) < 0;
            case COL_GRID: return a->grid.compare(b->grid, Qt::CaseInsensitive) < 0;
            case COL_SNR:  return a->snr < b->snr;
            case COL_FREQ: return a->freq < b->freq;
            case COL_DIST: return a->distKm < b->distKm;
            case COL_BRG:  return a->bearing < b->bearing;
            case COL_DXCC: return a->dxcc.compare(b->dxcc, Qt::CaseInsensitive) < 0;
            case COL_CONT: return a->continent.compare(b->continent, Qt::CaseInsensitive) < 0;
            default:       return false;
            }
        });
        // Sort ascending above, then reverse for descending — negating the
        // comparator directly would flip the relative order of equal
        // elements too, which stable_sort is specifically meant to preserve.
        if (!asc) std::reverse(shown.begin(), shown.end());
    }

    for (auto *table : { m_tableLeft, m_tableRight }) {
        table->horizontalHeader()->setSortIndicatorShown(m_sortColumn >= 0);
        if (m_sortColumn >= 0) table->horizontalHeader()->setSortIndicator(m_sortColumn, m_sortOrder);
    }

    // Fill left column top-to-bottom, then right — same reading order as a
    // two-column newspaper page — so the same number of rows fits in half
    // the height each pane would otherwise need.
    const int leftCount = (shown.size() + 1) / 2;
    for (int i = 0; i < shown.size(); ++i)
        addRowTo(i < leftCount ? m_tableLeft : m_tableRight, *shown[i]);

    QString stat = QString("%1 calls").arg(m_entries.size());
    if (cqCount)   stat += QString("  %1 CQ").arg(cqCount);
    if (forMeCount) stat += QString("  ⚡ %1 calling YOU").arg(forMeCount);
    m_stats->setText(stat);
    if (forMeCount) m_stats->setStyleSheet("color:#ff8080;font-size:10px;font-weight:bold;padding:0 4px;");
    else            m_stats->setStyleSheet("color:#4a7a9a;font-size:10px;padding:0 4px;");

    // Most distant / strongest / weakest, across everything currently shown
    if (shown.isEmpty()) {
        m_summary->setText("—");
    } else {
        RosterEntry const *farthest=nullptr, *strongest=nullptr, *weakest=nullptr;
        for (auto *e : shown) {
            if (e->distKm > 0 && (!farthest || e->distKm > farthest->distKm)) farthest = e;
            if (!strongest || e->snr > strongest->snr) strongest = e;
            if (!weakest   || e->snr < weakest->snr)   weakest   = e;
        }
        QStringList parts;
        if (farthest)  parts << QString("Most distant: %1 (%2 km)").arg(farthest->call).arg(int(farthest->distKm));
        if (strongest) parts << QString("Strongest: %1 (%2 dB)").arg(strongest->call).arg(strongest->snr);
        if (weakest)   parts << QString("Weakest: %1 (%2 dB)").arg(weakest->call).arg(weakest->snr);
        m_summary->setText(parts.join("    "));
    }
}

double CallRoster::haversineKm(double lat1, double lon1, double lat2, double lon2) const
{
    const double R=6371;
    const double dLat=(lat2-lat1)*DEG, dLon=(lon2-lon1)*DEG;
    const double a=sin(dLat/2)*sin(dLat/2)+cos(lat1*DEG)*cos(lat2*DEG)*sin(dLon/2)*sin(dLon/2);
    return R*2*atan2(sqrt(a),sqrt(1-a));
}
double CallRoster::bearingDeg(double lat1, double lon1, double lat2, double lon2) const
{
    const double y=sin((lon2-lon1)*DEG)*cos(lat2*DEG);
    const double x=cos(lat1*DEG)*sin(lat2*DEG)-sin(lat1*DEG)*cos(lat2*DEG)*cos((lon2-lon1)*DEG);
    return fmod(atan2(y,x)/DEG+360.0,360.0);
}
