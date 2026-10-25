#include "network.h"
#include "session.h"
#include "../version.h"

#include <QEventLoop>
#include <QUrlQuery>
#include <QDebug>
#include <QTimer>

Network::Network(Session *session, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_nam(new OhNetworkAccessManager(this))
{
}

Session *Network::session() const
{
    return m_session;
}

QNetworkAccessManager *Network::nam() const
{
    return m_nam;
}

QNetworkRequest Network::makeRequest(const QUrl &url) const
{
    QNetworkRequest request(url);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
#endif
    if (m_session) {
        if (!m_session->userToken().isEmpty())
            request.setRawHeader("user-token", m_session->userToken().toUtf8());
        if (!m_session->deviceToken().isEmpty())
            request.setRawHeader("device-static", m_session->deviceToken().toUtf8());
    }
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "ObjectHubNativeClient/" OH_VERSION);
    return request;
}

void Network::logReq(const char *tag, const QString &method, const QUrl &url, const QByteArray &body)
{
    qDebug().noquote() << QString::fromLatin1("[NET]") << tag << method << url.toString();
    if (!url.query().isEmpty())
        qDebug().noquote() << QString::fromLatin1("[NET]  query:") << url.query();
    if (!body.isEmpty()) {
        QByteArray red = body;
        QUrlQuery q;
        q.setQuery(QString::fromUtf8(body));
        if (q.hasQueryItem(QString::fromLatin1("password"))) {
            q.removeAllQueryItems(QString::fromLatin1("password"));
            q.addQueryItem(QString::fromLatin1("password"), QString::fromLatin1("***"));
            red = q.query(QUrl::FullyEncoded).toUtf8();
        }
        if (red.size() > 800)
            red = red.left(800) + "...";
        qDebug().noquote() << QString::fromLatin1("[NET]  body:") << QString::fromUtf8(red);
    }
}

void Network::logResp(const char *tag, QNetworkReply *reply, const QByteArray &data)
{
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug().noquote() << QString::fromLatin1("[NET]") << tag
                       << QString::fromLatin1("HTTP") << http
                       << QString::fromLatin1("qterr=") << (int)reply->error()
                       << reply->errorString();
    QByteArray show = data;
    if (show.size() > 2000)
        show = show.left(2000) + "\n...[truncated]...";
    qDebug().noquote() << QString::fromLatin1("[NET]  body:") << QString::fromUtf8(show);
}

QNetworkReply *OhNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request,
                                                     QIODevice *outgoingData)
{
    QNetworkReply *reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
    QTimer *watchdog = new QTimer(reply);   // child of the reply: freed together with it
    watchdog->setSingleShot(true);
    connect(watchdog, SIGNAL(timeout()), reply, SLOT(abort()));
    connect(reply, SIGNAL(finished()), watchdog, SLOT(stop()));
    watchdog->start(op == GetOperation ? 30000 : 120000);
    return reply;
}
