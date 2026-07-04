#include "VersionChecker.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// "v3.2.0" or "3.2.0"  →  30200
int VersionChecker::versionVal(QString const& tag)
{
    QString t = tag.startsWith('v') ? tag.mid(1) : tag;
    auto p = t.split('.');
    return p.value(0).toInt()*10000 + p.value(1).toInt()*100 + p.value(2).toInt();
}

VersionChecker::VersionChecker(QString const& currentTag, QObject *parent)
    : QObject(parent), m_current(currentTag)
{
    connect(&m_nam, &QNetworkAccessManager::finished, this, &VersionChecker::replyDone);
    m_timer.setInterval(3600 * 1000);   // hourly
    connect(&m_timer, &QTimer::timeout, this, &VersionChecker::poll);
}

void VersionChecker::startChecking()
{
    QTimer::singleShot(8000, this, &VersionChecker::poll);   // first check 8 s after startup
    m_timer.start();
}

void VersionChecker::poll()
{
    QNetworkRequest req {QUrl{"https://api.github.com/repos/2E0LXY/wsjt-zii/releases/latest"}};
    req.setRawHeader("User-Agent",  "WSJT-Y-AutoUpdater");
    req.setRawHeader("Accept",      "application/vnd.github+json");
    m_nam.get(req);
}

void VersionChecker::replyDone(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) return;

    auto obj = QJsonDocument::fromJson(reply->readAll()).object();
    QString tag = obj["tag_name"].toString();
    if (tag.isEmpty()) return;

    if (versionVal(tag) > versionVal(m_current)) {
        QUrl winUrl, debUrl;
        for (auto const& a : obj["assets"].toArray()) {
            auto asset = a.toObject();
            QString name = asset["name"].toString();
            QUrl    url  = QUrl{asset["browser_download_url"].toString()};
            if (name.endsWith(".exe")) winUrl = url;
            if (name.endsWith(".deb")) debUrl = url;
        }
        emit updateAvailable(tag, winUrl, debUrl);
    }
}
