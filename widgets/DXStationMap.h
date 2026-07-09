#ifndef DXSTATIONMAP_H
#define DXSTATIONMAP_H

#include <QWidget>
#include <QString>
#include <QList>
#include <QPointF>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QMap>
#include <QWheelEvent>
#include <QLabel>
#include <QMenu>
#include "../qrzlookup.h"


struct PlottedStation {
    QString call, grid;
    int snr = 0, freqHz = 0, period = 0;
    bool isCQ = false, forMe = false;
};

class DXStationMap : public QWidget
{
    Q_OBJECT
public:
    explicit DXStationMap(QWidget *parent = nullptr);

    void setHomeGrid(QString const& grid);
    void showStation(QString const& call, QString const& grid, int snr,
                     bool isCQ = false, bool forMe = false);
    void clearStations();
    void addStation(PlottedStation const& s);
    void tryAddCallsign(QString const& call, int freqHz, int snr, bool forMe);
    void setMyCall(QString const& call);
    void expireStations(int currentPeriod, int maxAge = 20);
    void setQrzLookup(QRZLookup *qrz);   // pass in from mainwindow if configured

signals:
    void stationClicked(QString call, int freqHz, QString grid);
    void pinRequested(Qt::DockWidgetArea area);   // hamburger → mainwindow

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private slots:
    void onQrzResult(QrzRecord const& r);

private:
    // ── Map geometry ──────────────────────────────────────────────────────────
    static constexpr int INFO_H = 130;   // bottom info panel height
    QPointF project(double lon, double lat) const;
    bool    gridToLatLon(QString const& grid, double &lat, double &lon) const;
    double  haversineKm(double, double, double, double) const;
    double  bearingDeg(double, double, double, double) const;
    void    drawGridSquare(QPainter &, QString const&, QColor, QColor, int) const;
    void    drawArc(QPainter &, double, double, double, double) const;
    void    drawHomeMarker(QPainter &) const;
    void    drawStationMarker(QPainter &, double, double, QString const&, QColor) const;
    void    drawInfoPanel(QPainter &) const;

    // ── State ─────────────────────────────────────────────────────────────────
    QString   m_homeGrid;
    double    m_homeLat = 53.0, m_homeLon = -2.0;
    QString   m_selCall, m_selGrid;
    int       m_selSNR = 0, m_selFreqHz = 0;
    double    m_selLat = 0.0, m_selLon = 0.0;
    QList<PlottedStation> m_stations;
    int       m_currentPeriod = 0;
    QPixmap   m_worldMap;
    QrzRecord m_qrzData;
    QRZLookup *m_qrz = nullptr;


    // ── Zoom / pan ────────────────────────────────────────────────────────────
    void wheelEvent(QWheelEvent *e) override;
    double  m_zoom = 1.0;
    double  m_panLon = 0.0;
    double  m_panLat = 20.0;
    QPoint  m_dragStartPos;
    double  m_dragPanLon = 0.0;
    double  m_dragPanLat = 20.0;

    // ── Animation ─────────────────────────────────────────────────────────────
    QTimer *m_animTimer = nullptr;
    int     m_animFrame = 0;           // increments every 500ms
    QString m_myCall;                  // used to detect "calling me"

    // ── Callsign→grid cache (so ALL calls can be plotted) ────────────────────
    QMap<QString,QString> m_callGrid;  // call → 4-char grid, built from CQ decodes

    // ── Overlay widgets ───────────────────────────────────────────────────────
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_menuBtn  = nullptr;
    QPushButton *m_homeBtn  = nullptr;   // reset zoom/pan to world view
    QLabel      *m_photoLbl = nullptr;
};

#endif
