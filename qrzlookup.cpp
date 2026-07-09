#include "qrzlookup.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QXmlStreamReader>
#include <QDebug>

constexpr char QRZLookup::URL[];

QRZLookup::QRZLookup(QString const& user, QString const& pass, QObject *parent)
    : QObject(parent), m_user(user), m_pass(pass)
{
    connect(&m_sessionNam, &QNetworkAccessManager::finished, this, &QRZLookup::onSessionReply);
    connect(&m_lookupNam,  &QNetworkAccessManager::finished, this, &QRZLookup::onLookupReply);
    connect(&m_photoNam,   &QNetworkAccessManager::finished, this, &QRZLookup::onPhotoReply);
    if (!m_user.isEmpty()) startSession();
}

void QRZLookup::startSession()
{
    QUrl url{URL};
    QUrlQuery q;
    q.addQueryItem("username", m_user);
    q.addQueryItem("password", m_pass);
    q.addQueryItem("agent",    "WSJT-Y-3.2");
    url.setQuery(q);
    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::UserAgentHeader, "WSJT-Y");
    auto *r = m_sessionNam.get(req); (void)r;
}

void QRZLookup::onSessionReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "QRZ session error:" << reply->errorString();
        return;
    }
    const auto data = reply->readAll();
    m_sessionKey = tagText(data, "Key");
    if (m_sessionKey.isEmpty()) {
        const auto err = tagText(data, "Error");
        qDebug() << "QRZ session failed:" << err;
        return;
    }
    qDebug() << "QRZ session key obtained, pending call:" << m_pendingCall;
    if (!m_pendingCall.isEmpty()) {
        auto c = m_pendingCall; m_pendingCall.clear(); lookup(c);
    }
}

void QRZLookup::lookup(QString const& call)
{
    if (m_user.isEmpty() || call.isEmpty()) return;
    if (m_sessionKey.isEmpty()) { m_pendingCall = call; startSession(); return; }
    QUrl url{URL};
    QUrlQuery q;
    q.addQueryItem("s",        m_sessionKey);
    q.addQueryItem("callsign", call.toUpper());
    url.setQuery(q);
    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::UserAgentHeader, "WSJT-Y");
    auto *r = m_lookupNam.get(req);  (void)r;
}

void QRZLookup::onLookupReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "QRZ lookup error:" << reply->errorString();
        return;
    }
    const auto data = reply->readAll();

    // Session expiry
    if (tagText(data,"Error").contains("Session")) {
        m_sessionKey.clear();
        if (!m_pendingCall.isEmpty()) { auto c=m_pendingCall; m_pendingCall.clear(); lookup(c); }
        return;
    }

    QrzRecord r;
    QXmlStreamReader xml{data};
    while (!xml.atEnd() && !xml.hasError()) {
        auto tok = xml.readNext();
        if (tok == QXmlStreamReader::StartElement) {
            const auto tag = xml.name().toString();
            if      (tag=="call")    r.call    = xml.readElementText();
            else if (tag=="fname")   r.fname   = xml.readElementText();
            else if (tag=="name")    r.name    = xml.readElementText();
            else if (tag=="addr1")   r.addr1   = xml.readElementText();
            else if (tag=="addr2")   r.addr2   = xml.readElementText();
            else if (tag=="state")   r.state   = xml.readElementText();
            else if (tag=="country") r.country = xml.readElementText();
            else if (tag=="county")  r.county  = xml.readElementText();
            else if (tag=="grid")    r.grid    = xml.readElementText();
            else if (tag=="email")   r.email   = xml.readElementText();
            else if (tag=="bio")     r.bio     = xml.readElementText();
            else if (tag=="image")   r.imageUrl= QUrl{xml.readElementText()};
            else if (tag=="Error")   { qDebug()<<"QRZ callsign error:"<<xml.readElementText(); return; }
        }
    }
    r.valid = !r.call.isEmpty();
    if (!r.valid) return;

    qDebug() << "QRZ record:" << r.call << r.fname << r.name << r.country;

    if (r.imageUrl.isValid() && !r.imageUrl.isEmpty()) {
        // Force HTTP for the photo URL (HTTPS requires OpenSSL on Windows;
        // files.qrz.com serves images on plain HTTP too)
        QUrl photoUrl = r.imageUrl;
        if (photoUrl.scheme() == "https") photoUrl.setScheme("http");
        QNetworkRequest req{photoUrl};
        req.setHeader(QNetworkRequest::UserAgentHeader,"WSJT-Y");
        m_photoNam.get(req);
        m_pendingRecord = r;
    } else {
        emit lookupResult(r);
    }
}

void QRZLookup::onPhotoReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError)
        m_pendingRecord.photo.loadFromData(reply->readAll());
    emit lookupResult(m_pendingRecord);
    m_pendingRecord = QrzRecord{};
}

QString QRZLookup::tagText(QByteArray const& data, QString const& tag)
{
    QXmlStreamReader xml{data};
    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNext() == QXmlStreamReader::StartElement
                && xml.name().toString() == tag)
            return xml.readElementText();
    }
    return {};
}
