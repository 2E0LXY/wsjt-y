#include "DXStationMap.h"
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QToolTip>
#include <cmath>

static constexpr double DEG = M_PI / 180.0;

DXStationMap::DXStationMap(QWidget *parent)
  : QWidget(parent)
{
    setMinimumSize(280, 200);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Load photorealistic world map background from Qt resources
    m_worldMap.load(":/images/worldmap.jpg");
}

void DXStationMap::setHomeGrid(QString const& grid)
{
    m_homeGrid = grid.toUpper().left(6);
    gridToLatLon(m_homeGrid, m_homeLat, m_homeLon);
    update();
}

void DXStationMap::showStation(QString const& call, QString const& grid,
                                int snr, bool isCQ, bool forMe)
{
    m_selCall = call;
    m_selGrid = grid.toUpper().left(4);
    m_selSNR  = snr;
    gridToLatLon(m_selGrid, m_selLat, m_selLon);
    // freqHz updated by addStation when the same callsign exists
    for (auto const& s : m_stations)
        if (s.call == call) { m_selFreqHz = s.freqHz; break; }

    PlottedStation s;
    s.call = call; s.grid = m_selGrid; s.snr = snr; s.isCQ = isCQ; s.forMe = forMe;
    for (auto &existing : m_stations)
        if (existing.call == call) { existing = s; update(); return; }
    m_stations.append(s);
    update();
}

void DXStationMap::clearStations()
{
    m_stations.clear();
    m_selCall.clear();
    update();
}

void DXStationMap::addStation(PlottedStation const& s)
{
    for (auto &e : m_stations) if (e.call == s.call) { e = s; update(); return; }
    m_stations.append(s);
    update();
}

// Map a (lon, lat) pair to widget pixel coordinates.
// Flat equirectangular projection — simple but familiar for ham radio use.
QPointF DXStationMap::project(double lon, double lat) const
{
    const int h = height(), w = width();
    // Leave a 14px info bar at the bottom
    const int mapH = h - 14;
    const double x = (lon + 180.0) / 360.0 * w;
    const double y = (90.0 - lat) / 180.0 * mapH;
    return {x, y};
}

bool DXStationMap::gridToLatLon(QString const& grid, double &lat, double &lon) const
{
    if (grid.length() < 2) return false;
    const auto g = grid.toUpper();
    lon = (g[0].unicode() - 'A') * 20.0 - 180.0;
    lat = (g[1].unicode() - 'A') * 10.0 - 90.0;
    if (g.length() >= 4) {
        lon += (g[2].unicode() - '0') * 2.0;
        lat += (g[3].unicode() - '0') * 1.0;
    }
    lon += (g.length() >= 4) ? 1.0 : 10.0;
    lat += (g.length() >= 4) ? 0.5 : 5.0;
    return true;
}

double DXStationMap::haversineKm(double lat1, double lon1,
                                  double lat2, double lon2) const
{
    const double R = 6371.0;
    const double dLat = (lat2 - lat1) * DEG;
    const double dLon = (lon2 - lon1) * DEG;
    const double a = std::sin(dLat/2)*std::sin(dLat/2)
                   + std::cos(lat1*DEG)*std::cos(lat2*DEG)
                   * std::sin(dLon/2)*std::sin(dLon/2);
    return R * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1-a));
}

double DXStationMap::bearingDeg(double lat1, double lon1,
                                 double lat2, double lon2) const
{
    const double y = std::sin((lon2-lon1)*DEG) * std::cos(lat2*DEG);
    const double x = std::cos(lat1*DEG)*std::sin(lat2*DEG)
                   - std::sin(lat1*DEG)*std::cos(lat2*DEG)*std::cos((lon2-lon1)*DEG);
    return std::fmod(std::atan2(y,x)/DEG + 360.0, 360.0);
}

void DXStationMap::drawGridSquare(QPainter &p, QString const& grid,
                                   QColor fill, QColor stroke, int strokeW) const
{
    if (grid.length() < 2) return;
    const auto g = grid.toUpper();
    double lon0, lat0, lon1, lat1;
    lon0 = (g[0].unicode()-'A') * 20.0 - 180.0;
    lat0 = (g[1].unicode()-'A') * 10.0 - 90.0;
    if (g.length() >= 4) {
        lon0 += (g[2].unicode()-'0') * 2.0;
        lat0 += (g[3].unicode()-'0') * 1.0;
        lon1 = lon0 + 2.0; lat1 = lat0 + 1.0;
    } else {
        lon1 = lon0 + 20.0; lat1 = lat0 + 10.0;
    }
    const QPointF tl = project(lon0, lat1);
    const QPointF br = project(lon1, lat0);
    p.setBrush(fill);
    p.setPen(QPen(stroke, strokeW));
    p.drawRect(QRectF(tl, br));
}

void DXStationMap::drawArc(QPainter &p, double lat1, double lon1,
                            double lat2, double lon2) const
{
    // Approximate great circle as 60 interpolated points (slerp)
    QPainterPath path;
    bool first = true;
    for (int i = 0; i <= 60; ++i) {
        const double t = double(i) / 60.0;
        const double d = 2.0 * std::asin(std::sqrt(
            std::pow(std::sin((lat2-lat1)*DEG/2),2) +
            std::cos(lat1*DEG)*std::cos(lat2*DEG)*
            std::pow(std::sin((lon2-lon1)*DEG/2),2)));
        if (d < 1e-6) { path.lineTo(project(lon2,lat2)); break; }
        const double A = std::sin((1-t)*d)/std::sin(d);
        const double B = std::sin(t*d)/std::sin(d);
        const double x = A*std::cos(lat1*DEG)*std::cos(lon1*DEG)+B*std::cos(lat2*DEG)*std::cos(lon2*DEG);
        const double y = A*std::cos(lat1*DEG)*std::sin(lon1*DEG)+B*std::cos(lat2*DEG)*std::sin(lon2*DEG);
        const double z = A*std::sin(lat1*DEG)+B*std::sin(lat2*DEG);
        const double lat = std::atan2(z,std::sqrt(x*x+y*y))/DEG;
        const double lon = std::atan2(y,x)/DEG;
        const QPointF pt = project(lon, lat);
        if (first) { path.moveTo(pt); first = false; } else { path.lineTo(pt); }
    }
    QPen arcPen(QColor(77,166,255), 1.5, Qt::DashLine);
    arcPen.setDashPattern({4.0, 3.0});
    p.setPen(arcPen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}

void DXStationMap::drawHomeMarker(QPainter &p) const
{
    if (m_homeGrid.isEmpty()) return;
    const QPointF pt = project(m_homeLon, m_homeLat);
    p.setBrush(QColor(63,185,80));
    p.setPen(QPen(QColor(46,160,67), 1));
    p.drawEllipse(pt, 4, 4);
    p.setPen(QColor(63,185,80));
    p.setFont(QFont("sans-serif", 7));
    p.drawText(int(pt.x())+6, int(pt.y())-3, m_homeGrid.left(4));
}

void DXStationMap::drawStationMarker(QPainter &p, double lat, double lon,
                                      QString const& label, QColor col) const
{
    const QPointF pt = project(lon, lat);
    p.setBrush(col);
    p.setPen(QPen(col.darker(140), 1));
    p.drawEllipse(pt, 5, 5);
    p.setPen(col);
    p.setFont(QFont("Courier New", 7, QFont::Bold));
    p.drawText(int(pt.x())+7, int(pt.y())-4, label.left(12));
}

void DXStationMap::drawInfoBar(QPainter &p) const
{
    const int h = height(), w = width();
    p.fillRect(0, h-14, w, 14, QColor(15,20,28));
    p.setPen(QColor(90,100,120));
    p.drawLine(0, h-14, w, h-14);
    p.setFont(QFont("sans-serif", 7));

    if (!m_selCall.isEmpty()) {
        const double km   = haversineKm(m_homeLat, m_homeLon, m_selLat, m_selLon);
        const double brg  = bearingDeg(m_homeLat, m_homeLon, m_selLat, m_selLon);
        const QString txt = QString("%1  %2  Az:%3°  %4km  SNR:%5dB")
            .arg(m_selCall, -9)
            .arg(m_selGrid, -4)
            .arg(int(brg), 3)
            .arg(int(km + 0.5))
            .arg(m_selSNR);
        p.setPen(QColor(200,210,230));
        p.drawText(6, h-3, txt);
    } else {
        p.setPen(QColor(60,70,90));
        p.drawText(6, h-3, "click a callsign to plot on map");
    }
}

void DXStationMap::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);  // off for text clarity

    const int w = width(), h = height(), mapH = h - 14;
    if (w < 20 || mapH < 20) return;

    // ── Background: photorealistic world map ─────────────────────────────────
    if (!m_worldMap.isNull()) {
        p.drawPixmap(QRect(0, 0, w, mapH),
                     m_worldMap,
                     QRect(0, 0, m_worldMap.width(), m_worldMap.height()));
        // Dark overlay so overlaid elements remain readable
        p.fillRect(0, 0, w, mapH, QColor(0, 0, 0, 60));
    } else {
        for (int y = 0; y < mapH; ++y) {
            const double t = double(y) / mapH;
            const double s = 4.0 * t * (1.0 - t);
            p.fillRect(0, y, w, 1, QColor(int(3+s*14), int(7+s*20), int(18+s*38)));
        }
    }

    // ── Grid lines ───────────────────────────────────────────────────────────
    const int cellW = w / 18;
    const int cellH = mapH / 18;
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(18, 55, 100, 200), 1));
    for (int i = 0; i <= 18; ++i) {
        const double lon = i*20.0-180.0;
        p.drawLine(project(lon,-90), project(lon,90));
    }
    for (int j = 0; j <= 18; ++j) {
        const double lat = j*10.0-90.0;
        p.drawLine(project(-180,lat), project(180,lat));
    }

    // ── Field labels — only when cells are tall enough ───────────────────────
    // Font size strictly bounded by cell height so labels never overlap.
    const int fontSize = qBound(4, cellH/2 - 1, cellW/3);
    if (cellH >= 12 && fontSize >= 4) {
        p.setFont(QFont("sans-serif", fontSize));
        p.setPen(QColor(30, 80, 145, 170));
        const QFontMetrics fm(p.font());
        for (int fi = 0; fi < 18; ++fi) {
            for (int li = 0; li < 18; ++li) {
                const QPointF c = project(fi*20.0-180.0+10.0, li*10.0-90.0+5.0);
                if (c.x() < 0 || c.x() > w || c.y() < 0 || c.y() > mapH) continue;
                const QString lbl = QString("%1%2").arg(QChar('A'+fi)).arg(QChar('A'+li));
                const int tw = fm.horizontalAdvance(lbl);
                p.drawText(QPointF(c.x()-tw/2.0, c.y()+fm.ascent()/2.0), lbl);
            }
        }
    }

    // ── Equator / prime meridian ─────────────────────────────────────────────
    p.setPen(QPen(QColor(30, 90, 160, 100), 1, Qt::DashLine));
    p.drawLine(project(-180,0), project(180,0));
    p.drawLine(project(0,-90), project(0,90));

    // ── Selected station field + square highlight ────────────────────────────
    if (!m_selGrid.isEmpty()) {
        drawGridSquare(p, m_selGrid.left(2), QColor(31,111,235,40), QColor(40,130,255,150), 1);
        if (m_selGrid.length() >= 4)
            drawGridSquare(p, m_selGrid.left(4), QColor(77,166,255,65), QColor(100,200,255), 2);
    }

    // ── All accumulated station dots ─────────────────────────────────────────
    const double dotR = qMax(2.0, cellW * 0.10);
    for (auto const& s : m_stations) {
        if (s.call == m_selCall) continue;
        double lat, lon;
        if (!gridToLatLon(s.grid, lat, lon)) continue;
        QColor col = s.forMe ? QColor(80,220,120,220) :
                     s.isCQ  ? QColor(80,160,255,220) :
                                QColor(90,110,150,170);
        const QPointF pt = project(lon, lat);
        p.setBrush(col); p.setPen(Qt::NoPen);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawEllipse(pt, dotR, dotR);
        p.setRenderHint(QPainter::Antialiasing, false);
    }

    // ── Great-circle arc ─────────────────────────────────────────────────────
    if (!m_selGrid.isEmpty() && !m_homeGrid.isEmpty())
        drawArc(p, m_homeLat, m_homeLon, m_selLat, m_selLon);

    // ── Home QTH ─────────────────────────────────────────────────────────────
    drawHomeMarker(p);

    // ── Selected station ─────────────────────────────────────────────────────
    if (!m_selCall.isEmpty())
        drawStationMarker(p, m_selLat, m_selLon, m_selCall, QColor(255,100,80));

    drawInfoBar(p);
}


void DXStationMap::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    update();
}

void DXStationMap::mouseMoveEvent(QMouseEvent *e)
{
    // Show field square label as tooltip
    const double lon = (double(e->pos().x()) / width()) * 360.0 - 180.0;
    const double lat = 90.0 - (double(e->pos().y()) / (height()-14)) * 180.0;
    if (lat >= -90 && lat <= 90) {
        const int fi = int((lon + 180.0) / 20.0);
        const int li = int((lat + 90.0) / 10.0);
        if (fi >= 0 && fi < 18 && li >= 0 && li < 18) {
            QString field;
            field.append(QChar('A' + fi));
            field.append(QChar('A' + li));
            const int sqi = int(std::fmod(lon + 180.0, 20.0) / 2.0);
            const int sqj = int(std::fmod(lat + 90.0, 10.0));
            field.append(QChar('0' + sqi));
            field.append(QChar('0' + sqj));
            QToolTip::showText(e->globalPos(), field, this);
        }
    }
    QWidget::mouseMoveEvent(e);
}

void DXStationMap::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    // Find nearest station dot within 18px
    double minDist = 18.0;
    PlottedStation const *found = nullptr;
    for (auto const& s : m_stations) {
        double lat, lon;
        if (!gridToLatLon(s.grid, lat, lon)) continue;
        const QPointF pt = project(lon, lat);
        const double dx = e->pos().x() - pt.x();
        const double dy = e->pos().y() - pt.y();
        const double dist = std::sqrt(dx*dx + dy*dy);
        if (dist < minDist) { minDist = dist; found = &s; }
    }
    if (found) {
        m_selCall   = found->call;
        m_selGrid   = found->grid;
        m_selSNR    = found->snr;
        m_selFreqHz = found->freqHz;
        gridToLatLon(m_selGrid, m_selLat, m_selLon);
        update();
        emit stationClicked(found->call, found->freqHz, found->grid);
    }
}

void DXStationMap::mouseDoubleClickEvent(QMouseEvent *)
{
    clearStations();
}
