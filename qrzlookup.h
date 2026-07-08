#ifndef QRZLOOKUP_H
#define QRZLOOKUP_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QPixmap>

struct QrzRecord {
    QString call, fname, name;
    QString addr1, addr2, state, country, county;
    QString grid, email, bio;
    QUrl    imageUrl;
    QPixmap photo;
    bool    valid = false;
};

class QRZLookup : public QObject
{
    Q_OBJECT
public:
    explicit QRZLookup(QString const& user, QString const& pass, QObject *parent = nullptr);

    void lookup(QString const& call);
    bool ready() const { return !m_sessionKey.isEmpty(); }

signals:
    void lookupResult(QrzRecord record);

private slots:
    void onSessionReply(QNetworkReply *reply);
    void onLookupReply(QNetworkReply *reply);
    void onPhotoReply(QNetworkReply *reply);

private:
    void startSession();
    QString tagText(QByteArray const& data, QString const& tag);

    QString m_user, m_pass, m_sessionKey, m_pendingCall;
    QrzRecord m_pendingRecord;
    QNetworkAccessManager m_sessionNam, m_lookupNam, m_photoNam;
    static constexpr char URL[] = "http://xmldata.qrz.com/xml/current/";
};

#endif
