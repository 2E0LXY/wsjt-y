#include "DXStationMap.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QToolTip>
#include <cmath>

static constexpr double DEG = M_PI / 180.0;

DXStationMap::DXStationMap(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(280, 260);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_worldMap.load(":/worldmap.jpg");

    // ── Hamburger — dock position (works when floating too) ──────────────────
    m_menuBtn = new QPushButton("⋮", this);
    m_menuBtn->setFixedSize(22, 22);
    m_menuBtn->setStyleSheet(
        "QPushButton{background:#0a1828;border:1px solid #1a4060;color:#4a8ab0;"
        "font-size:14px;font-weight:bold;border-radius:3px;padding:0;}"
        "QPushButton:hover{background:#0d2840;color:#00c8ff;}");
    m_menuBtn->setCursor(Qt::ArrowCursor);
    m_menuBtn->move(2, 2);
    connect(m_menuBtn, &QPushButton::clicked, this, [this]() {
        QMenu m;
        m.addAction(tr("Pin Left"),  [this]{ emit pinRequested(Qt::LeftDockWidgetArea);  });
        m.addAction(tr("Pin Right"), [this]{ emit pinRequested(Qt::RightDockWidgetArea); });
        m.addAction(tr("Float"),     [this]{ emit pinRequested(Qt::NoDockWidgetArea);    });
        m.exec(QCursor::pos());
    });

    // ── Clear button ─────────────────────────────────────────────────────────
    m_clearBtn = new QPushButton(QString(QChar(0x2715)) + " Clear", this);
    m_clearBtn->setFixedSize(62, 20);
    m_clearBtn->setStyleSheet(
        "QPushButton{background:#0a1828;border:1px solid #1a4060;color:#5090b0;"
        "font-size:9px;border-radius:3px;}"
        "QPushButton:hover{background:#0d2840;color:#00c8ff;}");
    m_clearBtn->setCursor(Qt::ArrowCursor);
    connect(m_clearBtn, &QPushButton::clicked, this, &DXStationMap::clearStations);

    // Animation timer — drives CQ flash + calling-user radar halo
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(500);
    connect(m_animTimer, &QTimer::timeout, this, [this]{ ++m_animFrame; update(); });
    m_animTimer->start();

    // ── QRZ photo label (info panel) ─────────────────────────────────────────
    m_photoLbl = new QLabel(this);
    m_photoLbl->setFixedSize(80, 80);
    m_photoLbl->setStyleSheet("border:1px solid #1a4060;background:#050c18;");
    m_photoLbl->setScaledContents(true);
    m_photoLbl->setAlignment(Qt::AlignCenter);
    m_photoLbl->setVisible(false);

    qRegisterMetaType<QrzRecord>("QrzRecord");
}

void DXStationMap::setQrzLookup(QRZLookup *qrz)
{
    m_qrz = qrz;
    if (m_qrz) connect(m_qrz, &QRZLookup::lookupResult,
                        this, &DXStationMap::onQrzResult);
}

void DXStationMap::onQrzResult(QrzRecord const& r)
{
    m_qrzData = r;
    if (!r.photo.isNull()) {
        m_photoLbl->setPixmap(r.photo);
        m_photoLbl->setVisible(true);
        const int mapH = height() - INFO_H;
        m_photoLbl->move(6, mapH + 6);
    }
    update();
}

void DXStationMap::setMyCall(QString const& call) { m_myCall = call.toUpper(); }

void DXStationMap::setHomeGrid(QString const& grid)
{
    m_homeGrid = grid.toUpper().left(6);
    gridToLatLon(m_homeGrid, m_homeLat, m_homeLon);
    update();
}

void DXStationMap::showStation(QString const& call, QString const& grid, int snr,
                                bool /*isCQ*/, bool /*forMe*/)
{
    m_selCall = call; m_selGrid = grid.toUpper().left(4); m_selSNR = snr;
    gridToLatLon(m_selGrid, m_selLat, m_selLon);
    for (auto const& s : m_stations) if (s.call==call) { m_selFreqHz=s.freqHz; break; }
    // Reset QRZ data + photo for new station
    m_qrzData = QrzRecord{};
    m_photoLbl->setVisible(false);
    if (m_qrz && !call.isEmpty()) m_qrz->lookup(call);
    update();
}

void DXStationMap::clearStations()
{
    m_stations.clear(); m_selCall.clear(); m_qrzData=QrzRecord{};
    m_photoLbl->setVisible(false);
    update();
}

void DXStationMap::addStation(PlottedStation const& s)
{
    if (!s.grid.isEmpty()) m_callGrid[s.call] = s.grid;  // cache for all-calls plotting
    for (auto &e : m_stations) if (e.call==s.call) { e=s; update(); return; }
    m_stations.append(s);
    update();
}

// Try to plot a callsign that appeared in a non-CQ message using cached grid
void DXStationMap::tryAddCallsign(QString const& call, int freqHz, int snr, bool forMe)
{
    if (call.isEmpty()) return;
    // Already on map?
    for (auto const& s : m_stations) if (s.call==call) return;
    // Have cached grid?
    if (!m_callGrid.contains(call)) return;
    PlottedStation ps;
    ps.call=call; ps.grid=m_callGrid[call]; ps.freqHz=freqHz;
    ps.snr=snr; ps.period=m_currentPeriod; ps.isCQ=false; ps.forMe=forMe;
    m_stations.append(ps);
    update();
}

void DXStationMap::expireStations(int currentPeriod, int maxAge)
{
    m_currentPeriod = currentPeriod;
    for (int i=m_stations.size()-1; i>=0; --i)
        if ((currentPeriod-m_stations[i].period) > maxAge) m_stations.removeAt(i);
    update();
}

// ── Projection ────────────────────────────────────────────────────────────────
QPointF DXStationMap::project(double lon, double lat) const
{
    const int mapH = height() - INFO_H;
    const int w = width();
    // Apply zoom centred on m_panLon/m_panLat
    const double cx = (m_panLon + 180.0) / 360.0 * w;
    const double cy = (90.0 - m_panLat) / 180.0  * mapH;
    const double x  = (lon + 180.0) / 360.0 * w;
    const double y  = (90.0 - lat)  / 180.0  * mapH;
    return { cx + (x - cx) * m_zoom, cy + (y - cy) * m_zoom };
}

bool DXStationMap::gridToLatLon(QString const& grid, double &lat, double &lon) const
{
    if (grid.length()<2) return false;
    const auto g = grid.toUpper();
    lon = (g[0].unicode()-'A')*20.0-180.0;
    lat = (g[1].unicode()-'A')*10.0-90.0;
    if (g.length()>=4) { lon+=(g[2].unicode()-'0')*2.0; lat+=(g[3].unicode()-'0'); }
    lon += g.length()>=4 ? 1.0 : 10.0;
    lat += g.length()>=4 ? 0.5 : 5.0;
    return true;
}

double DXStationMap::haversineKm(double lat1,double lon1,double lat2,double lon2) const
{
    const double R=6371;
    const double dLat=(lat2-lat1)*DEG, dLon=(lon2-lon1)*DEG;
    const double a=sin(dLat/2)*sin(dLat/2)+cos(lat1*DEG)*cos(lat2*DEG)*sin(dLon/2)*sin(dLon/2);
    return R*2*atan2(sqrt(a),sqrt(1-a));
}

double DXStationMap::bearingDeg(double lat1,double lon1,double lat2,double lon2) const
{
    const double y=sin((lon2-lon1)*DEG)*cos(lat2*DEG);
    const double x=cos(lat1*DEG)*sin(lat2*DEG)-sin(lat1*DEG)*cos(lat2*DEG)*cos((lon2-lon1)*DEG);
    return fmod(atan2(y,x)/DEG+360.0,360.0);
}

// ── Drawing helpers ───────────────────────────────────────────────────────────
void DXStationMap::drawGridSquare(QPainter &p, QString const& grid,
                                   QColor fill, QColor stroke, int sw) const
{
    if (grid.length()<2) return;
    const auto g=grid.toUpper();
    double lon0=(g[0].unicode()-'A')*20.0-180.0, lat0=(g[1].unicode()-'A')*10.0-90.0;
    double lon1, lat1;
    if (g.length()>=4) { lon0+=(g[2].unicode()-'0')*2; lat0+=(g[3].unicode()-'0'); lon1=lon0+2; lat1=lat0+1; }
    else               { lon1=lon0+20; lat1=lat0+10; }
    p.setBrush(fill); p.setPen(QPen(stroke,sw));
    p.drawRect(QRectF(project(lon0,lat1),project(lon1,lat0)).normalized());
}

void DXStationMap::drawArc(QPainter &p,double lat1,double lon1,double lat2,double lon2) const
{
    QPainterPath path; bool first=true;
    for (int i=0;i<=60;++i) {
        const double t=double(i)/60;
        const double d=2*asin(sqrt(pow(sin((lat2-lat1)*DEG/2),2)+cos(lat1*DEG)*cos(lat2*DEG)*pow(sin((lon2-lon1)*DEG/2),2)));
        if (d<1e-6) break;
        const double A=sin((1-t)*d)/sin(d), B=sin(t*d)/sin(d);
        const double x=A*cos(lat1*DEG)*cos(lon1*DEG)+B*cos(lat2*DEG)*cos(lon2*DEG);
        const double y=A*cos(lat1*DEG)*sin(lon1*DEG)+B*cos(lat2*DEG)*sin(lon2*DEG);
        const double z=A*sin(lat1*DEG)+B*sin(lat2*DEG);
        const double la=atan2(z,sqrt(x*x+y*y))/DEG, lo=atan2(y,x)/DEG;
        const QPointF pt=project(lo,la);
        if (first) { path.moveTo(pt); first=false; } else { path.lineTo(pt); }
    }
    QPen pen(QColor(77,200,255),1.5,Qt::DashLine);
    pen.setDashPattern({5.0,3.0});
    p.setPen(pen); p.setBrush(Qt::NoBrush); p.drawPath(path);
}

void DXStationMap::drawHomeMarker(QPainter &p) const
{
    if (m_homeGrid.isEmpty()) return;
    const QPointF pt=project(m_homeLon,m_homeLat);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setBrush(QColor(63,220,100)); p.setPen(QPen(QColor(30,160,60),1));
    p.drawEllipse(pt,5,5);
    p.setFont(QFont("sans-serif",8,QFont::Bold));
    p.setPen(QColor(80,255,120));
    p.drawText(QPointF(pt.x()+7,pt.y()-4), m_homeGrid.left(4));
    p.setRenderHint(QPainter::Antialiasing,false);
}

void DXStationMap::drawStationMarker(QPainter &p,double lat,double lon,
                                      QString const& lbl,QColor col) const
{
    const QPointF pt=project(lon,lat);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setBrush(col); p.setPen(QPen(col.darker(150),1));
    p.drawEllipse(pt,6,6);
    p.setFont(QFont("Courier New",8,QFont::Bold));
    p.setPen(col);
    p.drawText(QPointF(pt.x()+8,pt.y()-5),lbl.left(12));
    p.setRenderHint(QPainter::Antialiasing,false);
}

// ── Rich info panel ───────────────────────────────────────────────────────────
void DXStationMap::drawInfoPanel(QPainter &p) const
{
    const int mapH = height() - INFO_H;
    const int w    = width();

    // Background
    p.fillRect(0, mapH, w, INFO_H, QColor(5, 12, 22));
    p.setPen(QPen(QColor(0, 90, 140), 1));
    p.drawLine(0, mapH, w, mapH);

    if (m_selCall.isEmpty()) {
        p.setFont(QFont("sans-serif", 9));
        p.setPen(QColor(40, 80, 110));
        p.drawText(QRect(0, mapH, w, INFO_H), Qt::AlignCenter,
                   tr("Click a station dot to see details"));
        return;
    }

    // Column layout: left = photo (if QRZ), right = text
    const int photoW = m_qrzData.valid && !m_qrzData.photo.isNull() ? 88 : 0;
    const int textX  = photoW + 8;
    const int textW  = w - textX - 6;

    // ── Callsign (large) ──────────────────────────────────────────────────────
    int ty = mapH + 8;
    p.setFont(QFont("Courier New", 15, QFont::Bold));
    p.setPen(QColor(0, 210, 255));
    p.drawText(QRect(textX, ty, textW, 22), Qt::AlignLeft|Qt::AlignVCenter, m_selCall);

    // QRZ name
    if (m_qrzData.valid && !(m_qrzData.fname+m_qrzData.name).trimmed().isEmpty()) {
        p.setFont(QFont("sans-serif", 9));
        p.setPen(QColor(180,210,230));
        ty += 22;
        p.drawText(QRect(textX, ty, textW, 16), Qt::AlignLeft|Qt::AlignVCenter,
                   (m_qrzData.fname+" "+m_qrzData.name).simplified());
    }

    // Location
    QString loc;
    if (m_qrzData.valid) {
        if (!m_qrzData.addr2.isEmpty()) loc = m_qrzData.addr2;
        if (!m_qrzData.country.isEmpty()) {
            if (!loc.isEmpty()) loc += ", ";
            loc += m_qrzData.country;
        }
    }
    if (!loc.isEmpty()) {
        ty += 16; p.setFont(QFont("sans-serif",9)); p.setPen(QColor(140,180,210));
        p.drawText(QRect(textX,ty,textW,15),Qt::AlignLeft,loc);
    }

    // Grid / SNR / Distance / Bearing — two-column mini-table
    ty += 18;
    const double km  = haversineKm(m_homeLat,m_homeLon,m_selLat,m_selLon);
    const double brg = bearingDeg(m_homeLat,m_homeLon,m_selLat,m_selLon);

    struct Row { QString k, v; };
    QList<Row> rows = {
        {"Grid",    m_selGrid},
        {"SNR",     QString("%1 dB").arg(m_selSNR)},
        {"Dist",    QString("%1 km / %2°").arg(int(km+0.5)).arg(int(brg+0.5))},
    };
    if (m_qrzData.valid && !m_qrzData.email.isEmpty())
        rows.append({"Email", m_qrzData.email});

    p.setFont(QFont("Courier New", 8));
    for (auto const& row : rows) {
        if (ty > mapH + INFO_H - 10) break;
        p.setPen(QColor(50,100,140)); p.drawText(QRect(textX,ty,38,13),Qt::AlignLeft,row.k);
        p.setPen(QColor(160,210,240)); p.drawText(QRect(textX+40,ty,textW-40,13),Qt::AlignLeft,row.v);
        ty += 14;
    }
}

// ── paintEvent ───────────────────────────────────────────────────────────────
void DXStationMap::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const int w=width(), h=height(), mapH=h-INFO_H;
    if (w<20||mapH<20) return;

    // World map background — apply zoom via inverse projection
    if (!m_worldMap.isNull()) {
        const int iw = m_worldMap.width(), ih = m_worldMap.height();
        if (m_zoom <= 1.01) {
            // World view — full image
            p.drawPixmap(QRect(0,0,w,mapH), m_worldMap, QRect(0,0,iw,ih));
        } else {
            // Zoomed — compute which portion of the world map is visible.
            // Inverse of project(): world_norm = pan_norm + (screen/size - pan_norm)/zoom
            const double pnx = (m_panLon + 180.0) / 360.0;
            const double pny = (90.0 - m_panLat) / 180.0;
            // Visible normalised lat/lon bounds at zoom level
            const double x0n = pnx + (0.0 - pnx) / m_zoom;   // left  edge (screen x=0)
            const double x1n = pnx + (1.0 - pnx) / m_zoom;   // right edge (screen x=w)
            const double y0n = pny + (0.0 - pny) / m_zoom;   // top   edge (screen y=0)
            const double y1n = pny + (1.0 - pny) / m_zoom;   // bot   edge (screen y=mapH)
            // Clamp to [0,1] and convert to image pixels
            const int sx = qBound(0, int(x0n * iw), iw);
            const int sy = qBound(0, int(y0n * ih), ih);
            const int sw = qBound(1, int((x1n - x0n) * iw), iw - sx);
            const int sh = qBound(1, int((y1n - y0n) * ih), ih - sy);
            p.drawPixmap(QRect(0,0,w,mapH), m_worldMap, QRect(sx,sy,sw,sh));
        }
        p.fillRect(0,0,w,mapH,QColor(0,0,0,45));
    } else {
        for (int y=0;y<mapH;++y) {
            const double t=double(y)/mapH, s=4*t*(1-t);
            p.fillRect(0,y,w,1,QColor(int(3+s*14),int(7+s*20),int(18+s*38)));
        }
    }

    // Grid lines — subtle over photo
    const bool hasSat=!m_worldMap.isNull();
    const int cellW=w/18, cellH=mapH/18;
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(hasSat?QColor(80,140,200,40):QColor(18,55,100,200),1));
    for (int i=0;i<=18;++i) { const double lon=i*20.0-180; p.drawLine(project(lon,-90),project(lon,90)); }
    for (int j=0;j<=18;++j) { const double lat=j*10.0-90;  p.drawLine(project(-180,lat),project(180,lat)); }

    // Field labels when no satellite image
    const int fs=qBound(4,cellH/2-1,cellW/3);
    if (!hasSat&&cellH>=12&&fs>=4) {
        p.setFont(QFont("sans-serif",fs)); p.setPen(QColor(30,80,145,170));
        const QFontMetrics fm(p.font());
        for (int fi=0;fi<18;++fi) for (int li=0;li<18;++li) {
            const QPointF c=project(fi*20.0-180+10, li*10.0-90+5);
            if (c.x()<0||c.x()>w||c.y()<0||c.y()>mapH) continue;
            const QString lbl=QString("%1%2").arg(QChar('A'+fi)).arg(QChar('A'+li));
            p.drawText(QPointF(c.x()-fm.horizontalAdvance(lbl)/2.0,c.y()+fm.ascent()/2.0),lbl);
        }
    }

    // Equator / prime meridian
    p.setPen(QPen(QColor(30,90,160,80),1,Qt::DashLine));
    p.drawLine(project(-180,0),project(180,0));
    p.drawLine(project(0,-90),project(0,90));

    // Selected station highlights
    if (!m_selGrid.isEmpty()) {
        drawGridSquare(p,m_selGrid.left(2),QColor(31,111,235,35),QColor(40,130,255,140),1);
        if (m_selGrid.length()>=4)
            drawGridSquare(p,m_selGrid,QColor(77,166,255,60),QColor(100,200,255),2);
    }

    // All station dots with animation
    const double dotR = qMax(3.0, cellW * 0.12);
    p.setRenderHint(QPainter::Antialiasing, true);
    const bool animOn = (m_animFrame & 1);   // toggles every 500ms

    for (auto const& s : m_stations) {
        if (s.call == m_selCall) continue;
        double lat, lon;
        if (!gridToLatLon(s.grid, lat, lon)) continue;
        const QPointF pt = project(lon, lat);

        if (s.forMe) {
            // ── Calling ME: red + pulsing radar halo ──────────────────────────
            if (animOn) {
                // Outer radar ring (expands on alternate frames)
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(QColor(255,60,60,120), 1));
                p.drawEllipse(pt, dotR*3.5, dotR*3.5);
                p.setPen(QPen(QColor(255,80,80,60), 1));
                p.drawEllipse(pt, dotR*5, dotR*5);
            }
            p.setPen(Qt::NoPen);
            p.setBrush(animOn ? QColor(255,80,80) : QColor(220,40,40));
            p.drawEllipse(pt, dotR*1.4, dotR*1.4);
            // White call label
            p.setFont(QFont("Courier New", 8, QFont::Bold));
            p.setPen(QColor(255,200,200));
            p.drawText(QPointF(pt.x()+dotR*1.5, pt.y()-4), s.call.left(12));

        } else if (s.isCQ) {
            // ── CQ: blue, flashing brightness ─────────────────────────────────
            const QColor col = animOn ? QColor(80,180,255,240) : QColor(50,120,200,180);
            p.setBrush(col); p.setPen(Qt::NoPen);
            p.drawEllipse(pt, dotR, dotR);
            p.setFont(QFont("Courier New", 7));
            p.setPen(QColor(160,210,255));
            p.drawText(QPointF(pt.x()+dotR+2, pt.y()-3), s.call.left(10));

        } else {
            // ── Other directed messages ────────────────────────────────────────
            p.setBrush(QColor(90,110,150,160)); p.setPen(Qt::NoPen);
            p.drawEllipse(pt, dotR*0.85, dotR*0.85);
        }
    }
    p.setRenderHint(QPainter::Antialiasing, false);

    // Arc + markers
    if (!m_selGrid.isEmpty()&&!m_homeGrid.isEmpty())
        drawArc(p,m_homeLat,m_homeLon,m_selLat,m_selLon);
    drawHomeMarker(p);
    if (!m_selCall.isEmpty())
        drawStationMarker(p,m_selLat,m_selLon,m_selCall,QColor(255,100,80));

    // Info panel
    drawInfoPanel(p);
}

// ── Events ────────────────────────────────────────────────────────────────────
void DXStationMap::wheelEvent(QWheelEvent *e)
{
    const double delta = e->angleDelta().y() > 0 ? 1.25 : 0.80;
    const int mapH = height() - INFO_H;
    const double w = width();

    // Compute world position under cursor using CURRENT zoom (inverse-project).
    // cnx/cny are normalised world coords (0..1 range).
    const double pnx_old = (m_panLon + 180.0) / 360.0;
    const double pny_old = (90.0 - m_panLat) / 180.0;
    const double sx = e->position().x(), sy = e->position().y();
    const double cnx = pnx_old + (sx / w     - pnx_old) / m_zoom;
    const double cny = pny_old + (sy / mapH   - pny_old) / m_zoom;

    m_zoom = qBound(1.0, m_zoom * delta, 8.0);

    if (m_zoom <= 1.01) {
        m_panLon = 0.0; m_panLat = 20.0;
    } else {
        // Solve for new pan that keeps the cursor on the same world point.
        // From project(): sx/w = pnx_new + (cnx - pnx_new) * zoom_new
        // => pnx_new = (sx/w - cnx * zoom_new) / (1 - zoom_new)
        const double pnx_new = (sx / w     - cnx * m_zoom) / (1.0 - m_zoom);
        const double pny_new = (sy / mapH  - cny * m_zoom) / (1.0 - m_zoom);
        m_panLon = qBound(-180.0, pnx_new * 360.0 - 180.0, 180.0);
        m_panLat = qBound( -85.0, 90.0 - pny_new * 180.0,   85.0);
    }
    update();
}

void DXStationMap::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if (m_clearBtn) m_clearBtn->move(width()-66, 2);
    if (m_menuBtn)  m_menuBtn->move(2, 2);
    if (m_photoLbl) m_photoLbl->move(6, height()-INFO_H+6);
    update();
}

void DXStationMap::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) setCursor(Qt::ArrowCursor);
    QWidget::mouseReleaseEvent(e);
}

void DXStationMap::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        // Start pan drag
        m_dragStartPos = e->pos();
        m_dragPanLon   = m_panLon;
        m_dragPanLat   = m_panLat;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (e->button()!=Qt::LeftButton) return;
    const int mapH=height()-INFO_H;
    if (e->pos().y()>mapH) return;   // click in info panel — ignore for dot selection

    double minDist=18; PlottedStation const *found=nullptr;
    for (auto const& s:m_stations) {
        double lat,lon; if (!gridToLatLon(s.grid,lat,lon)) continue;
        const QPointF pt=project(lon,lat);
        const double d=hypot(e->pos().x()-pt.x(),e->pos().y()-pt.y());
        if (d<minDist) { minDist=d; found=&s; }
    }
    if (found) {
        m_selCall=found->call; m_selGrid=found->grid;
        m_selSNR=found->snr;  m_selFreqHz=found->freqHz;
        gridToLatLon(m_selGrid,m_selLat,m_selLon);
        m_qrzData=QrzRecord{}; m_photoLbl->setVisible(false);
        if (m_qrz) m_qrz->lookup(found->call);
        update();
        emit stationClicked(found->call,found->freqHz,found->grid);
    }
}

void DXStationMap::mouseDoubleClickEvent(QMouseEvent *) { clearStations(); }

void DXStationMap::mouseMoveEvent(QMouseEvent *e)
{
    const int mapH=height()-INFO_H;

    if (e->buttons() & Qt::RightButton) {
        // Drag-to-pan: one screen-pixel movement = 1/zoom world-pixels
        const double dx = e->pos().x() - m_dragStartPos.x();
        const double dy = e->pos().y() - m_dragStartPos.y();
        const double lonPerPx  = 360.0 / width()  / m_zoom;
        const double latPerPx  = 180.0 / mapH      / m_zoom;
        m_panLon = qBound(-180.0, m_dragPanLon - dx * lonPerPx, 180.0);
        m_panLat = qBound( -85.0, m_dragPanLat + dy * latPerPx,  85.0);
        update();
        return;
    }

    if (e->pos().y()>=mapH) return;
    const double lon=(double(e->pos().x())/width())*360.0-180.0;
    const double lat=90.0-(double(e->pos().y())/mapH)*180.0;
    const int fi=int((lon+180)/20), li=int((lat+90)/10);
    if (fi>=0&&fi<18&&li>=0&&li<18) {
        QString lbl; lbl+=QChar('A'+fi); lbl+=QChar('A'+li);
        lbl+=QChar('0'+int(fmod(lon+180,20)/2));
        lbl+=QChar('0'+int(fmod(lat+90, 10)));
        QToolTip::showText(e->globalPos(),lbl,this);
    }
    QWidget::mouseMoveEvent(e);
}
