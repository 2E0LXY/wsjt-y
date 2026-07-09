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
    m_stats = new QLabel("0 calls");
    m_stats->setStyleSheet("color:#4a7a9a;font-size:10px;padding:0 4px;");
    topBar->addWidget(m_stats);
    vl->addLayout(topBar);

    // Table
    m_table = new QTableWidget(0, COL_COUNT, this);
    m_table->setHorizontalHeaderLabels({"Call","Grid","dB","Hz","km","°","DXCC","Cont","Message"});
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_table->horizontalHeader()->setSortIndicatorShown(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionsClickable(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    m_table->verticalHeader()->hide();
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(false);
    m_table->setStyleSheet(
        "QTableWidget{background:#060b12;color:#c0d8e8;font-family:'Courier New';font-size:10px;"
        "selection-background-color:#0d3a5a;border:none;}"
        "QTableWidget::item{padding:1px 4px;border-bottom:1px solid #0a1828;}"
        "QHeaderView::section{background:#0a1828;color:#4a8ab0;font-size:10px;padding:2px 4px;"
        "border-bottom:1px solid #0d2035;border-right:1px solid #0d2035;}");

    // Column widths
    m_table->setColumnWidth(COL_CALL, 72); m_table->setColumnWidth(COL_GRID, 44);
    m_table->setColumnWidth(COL_SNR,  30); m_table->setColumnWidth(COL_FREQ, 42);
    m_table->setColumnWidth(COL_DIST, 52); m_table->setColumnWidth(COL_BRG,  32);
    m_table->setColumnWidth(COL_DXCC, 88); m_table->setColumnWidth(COL_CONT, 38);

    vl->addWidget(m_table);

    connect(m_filter, &QLineEdit::textChanged, this, &CallRoster::rebuild);

    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int) {
        auto *ci = m_table->item(row, COL_CALL);
        if (!ci) return;
        const QString call = ci->text().trimmed();
        if (m_entries.contains(call)) {
            auto const& e = m_entries[call];
            emit callSelected(call, e.freq, e.grid);
        }
    });

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
    rebuild();
}

void CallRoster::clearAll() { m_entries.clear(); rebuild(); }

void CallRoster::expireOld(int maxSeconds)
{
    const auto cutoff = QDateTime::currentDateTimeUtc().addSecs(-maxSeconds);
    bool changed = false;
    for (auto it = m_entries.begin(); it != m_entries.end(); ) {
        if (it->lastSeen < cutoff) { it = m_entries.erase(it); changed = true; }
        else ++it;
    }
    if (changed) rebuild();
}

void CallRoster::rebuild()
{
    const QString flt = m_filter->text().trimmed().toUpper();
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    int cqCount=0, forMeCount=0;
    for (auto const& e : m_entries) {
        if (!flt.isEmpty() &&
            !e.call.contains(flt, Qt::CaseInsensitive) &&
            !e.dxcc.contains(flt, Qt::CaseInsensitive) &&
            !e.continent.contains(flt, Qt::CaseInsensitive))
            continue;

        const int row = m_table->rowCount();
        m_table->insertRow(row);

        // Row background colour
        QColor bg;
        if (e.forMe)      bg = QColor(80,15,15);     // dark red — calling ME
        else if (e.isCQ)  bg = QColor(8,25,50);      // dark blue — CQ
        else              bg = QColor(5,12,20);       // default

        auto item = [&](QString const& txt, Qt::Alignment align = Qt::AlignLeft|Qt::AlignVCenter) {
            auto *it = new QTableWidgetItem(txt);
            it->setBackground(bg);
            it->setTextAlignment(align);
            if (e.forMe)     it->setForeground(QColor(255,120,120));
            else if (e.isCQ) it->setForeground(QColor(100,180,255));
            return it;
        };

        m_table->setItem(row, COL_CALL, item(e.call));
        m_table->setItem(row, COL_GRID, item(e.grid));
        m_table->setItem(row, COL_SNR,  item(QString::number(e.snr), Qt::AlignRight|Qt::AlignVCenter));
        m_table->setItem(row, COL_FREQ, item(QString::number(e.freq), Qt::AlignRight|Qt::AlignVCenter));
        m_table->setItem(row, COL_DIST, item(e.distKm>0 ? QString::number(int(e.distKm)) : QString(),
                                              Qt::AlignRight|Qt::AlignVCenter));
        m_table->setItem(row, COL_BRG,  item(e.bearing>0 ? QString::number(int(e.bearing))+"°" : QString(),
                                              Qt::AlignRight|Qt::AlignVCenter));
        m_table->setItem(row, COL_DXCC, item(e.dxcc));
        m_table->setItem(row, COL_CONT, item(e.continent));
        m_table->setItem(row, COL_MSG,  item(e.msg));
        m_table->setRowHeight(row, 16);

        if (e.forMe) ++forMeCount;
        if (e.isCQ)  ++cqCount;
    }

    m_table->setSortingEnabled(true);

    QString stat = QString("%1 calls").arg(m_entries.size());
    if (cqCount)   stat += QString("  %1 CQ").arg(cqCount);
    if (forMeCount) stat += QString("  ⚡ %1 calling YOU").arg(forMeCount);
    m_stats->setText(stat);
    if (forMeCount) m_stats->setStyleSheet("color:#ff8080;font-size:10px;font-weight:bold;padding:0 4px;");
    else            m_stats->setStyleSheet("color:#4a7a9a;font-size:10px;padding:0 4px;");
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
