#ifndef CALLROSTER_H
#define CALLROSTER_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QHeaderView>
#include <QTimer>
#include <QMap>
#include <cmath>
#include <QDateTime>

struct RosterEntry {
    QString  call, grid, msg, dxcc, continent;
    int      snr = 0, freq = 0;
    double   distKm = 0, bearing = 0;
    bool     isCQ = false, forMe = false;
    QDateTime lastSeen;
};

class CallRoster : public QWidget
{
    Q_OBJECT
public:
    explicit CallRoster(QWidget *parent = nullptr);

    void setHomePos(double lat, double lon);
    void addDecode(QString const& call, QString const& grid, int snr, int freqHz,
                   QString const& msg, bool isCQ, bool forMe,
                   QString const& dxcc = QString{}, QString const& cont = QString{});
    void clearAll();
    void expireOld(int maxSeconds = 120);   // remove entries older than maxSeconds

signals:
    void callSelected(QString call, int freqHz, QString grid);

private:
    enum Col { COL_CALL=0, COL_GRID, COL_SNR, COL_FREQ, COL_DIST, COL_BRG,
               COL_DXCC, COL_CONT, COL_MSG, COL_COUNT };

    void rebuild();
    double haversineKm(double, double, double, double) const;
    double bearingDeg(double, double, double, double) const;

    QTableWidget *m_table;
    QLineEdit    *m_filter;
    QLabel       *m_stats;
    QTimer       *m_expireTimer;

    QMap<QString, RosterEntry> m_entries;
    double m_homeLat = 53.0, m_homeLon = -2.0;

    static constexpr double DEG = M_PI / 180.0;
};

#endif
