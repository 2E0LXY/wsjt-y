#ifndef DXSTATIONMAP_H
#define DXSTATIONMAP_H

// DXStationMap — a dockable Maidenhead grid + great-circle map for WSJT-Zii.
//
// Pure QPainter implementation; no extra dependencies beyond Qt5 Base.
// The map is a flat Mercator-equivalent projection of the 18×18 Maidenhead
// field grid.  When a decoded callsign is selected, it:
//   • highlights the station's field (2-char) and minor square (4-char)
//   • draws a great-circle arc from the home QTH
//   • shows distance, bearing, and dBm in the info bar
//
// Wire-up (from mainwindow):
//   m_dxMap->setHomeGrid(m_config.my_grid());
//   m_dxMap->showStation("CT5JWG", "IN50", -7);   // call, grid, SNR

#include <QWidget>
#include <QString>
#include <QList>
#include <QPointF>
#include <QVector>

struct PlottedStation {
    QString call;
    QString grid;       // 4-char Maidenhead
    int     snr;
    bool    isCQ;
    bool    forMe;
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

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private:
    QString m_homeGrid;
    double  m_homeLat = 53.0, m_homeLon = -2.0;

    QString m_selCall, m_selGrid;
    int     m_selSNR = 0;
    double  m_selLat = 0.0, m_selLon = 0.0;

    QList<PlottedStation> m_stations;

    QPointF project(double lon, double lat) const;
    bool    gridToLatLon(QString const& grid, double &lat, double &lon) const;
    double  haversineKm(double lat1, double lon1, double lat2, double lon2) const;
    double  bearingDeg(double lat1, double lon1, double lat2, double lon2) const;
    void    drawGridSquare(QPainter &p, QString const& grid,
                           QColor fill, QColor stroke, int strokeW) const;
    void    drawArc(QPainter &p, double lat1, double lon1,
                    double lat2, double lon2) const;
    void    drawHomeMarker(QPainter &p) const;
    void    drawStationMarker(QPainter &p, double lat, double lon,
                              QString const& label, QColor col) const;
    void    drawInfoBar(QPainter &p) const;
};

#endif // DXSTATIONMAP_H
