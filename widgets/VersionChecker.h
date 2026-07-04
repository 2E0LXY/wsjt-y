#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QUrl>

class QNetworkReply;

class VersionChecker : public QObject
{
    Q_OBJECT
public:
    explicit VersionChecker(QString const& currentTag, QObject *parent = nullptr);
    void startChecking();

signals:
    void updateAvailable(QString latestTag, QUrl winUrl, QUrl debUrl);

private slots:
    void poll();
    void replyDone(QNetworkReply *reply);

private:
    static int versionVal(QString const& tag);

    QString               m_current;
    QTimer                m_timer;
    QNetworkAccessManager m_nam;
};

#endif // VERSIONCHECKER_H
