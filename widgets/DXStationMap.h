#ifndef DXSTATIONMAP_H
#define DXSTATIONMAP_H

#include <QWidget>
#include <QString>
#include <QList>
#include <QPointF>
#include <QPixmap>

struct PlottedStation {
    QString call;
    QString grid;
    int     snr = 0;
    int     freqHz = 0;   // audio Hz — used for click-to-tune
    bool    isCQ = false;
    bool    forMe = false;
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

signals:
    void stationClicked(QString call, int freqHz, QString grid);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private:
    QString   m_homeGrid;
    double    m_homeLat = 53.0, m_homeLon = -2.0;
    QString   m_selCall, m_selGrid;
    int       m_selSNR = 0, m_selFreqHz = 0;
    double    m_selLat = 0.0, m_selLon = 0.0;
    QList<PlottedStation> m_stations;
    QPixmap   m_worldMap;

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

#endif
