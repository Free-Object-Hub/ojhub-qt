#include "profiledomain.h"
#include "../../core/session.h"
#include "../../core/network.h"
#include "../../core/protocol.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ProfileDomain::ProfileDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
{
}

QNetworkRequest ProfileDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

PublicProfile ProfileDomain::parseProfileResponse(const QByteArray &data) const
{
    PublicProfile p;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError)
        return p;

    if (!doc.isObject())
        return p;

    QJsonObject obj = doc.object();
    p.raw = obj.toVariantMap();

    p.userId = obj.value(QString::fromLatin1("ID")).toInt(
                   obj.value(QString::fromLatin1("id")).toInt(
                       obj.value(QString::fromLatin1("userId")).toInt()));
    p.username = obj.value(QString::fromLatin1("username")).toString();
    p.nickname = obj.value(QString::fromLatin1("nickname")).toString();
    p.resume = obj.value(QString::fromLatin1("resume")).toString();
    p.socials = obj.value(QString::fromLatin1("socials")).toString();

    return p;
}
void ProfileDomain::fetchProfile(int userId)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/content/getUser.php"));
    QUrlQuery query;
    query.addQueryItem(QString::fromLatin1("id"), QString::number(userId));
    url.setQuery(query);

    QNetworkRequest request = makeRequest(url);
    logNetReq("PROFILE", QString::fromLatin1("GET"), url, QByteArray());
    QNetworkReply *reply = m_nam->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(onProfileReplyFinished()));
}
void ProfileDomain::onProfileReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    const QByteArray data = reply->readAll().trimmed();
    logNetResp("PROFILE", reply, data);

    if (reply->error() != QNetworkReply::NoError) {
        emit profileError(reply->errorString());
        return;
    }
    if (data == "[\"NONE\"]" || data == "Access denied") {
        emit profileError(data == QByteArray("Access denied")
            ? QString::fromLatin1("Access denied")
            : QString::fromUtf8("User not found"));
        return;
    }

    PublicProfile p = parseProfileResponse(data);
    if (p.userId == 0 && p.username.isEmpty()) {
        emit profileError(QString::fromUtf8("Could not parse the profile"));
        return;
    }
    emit profileFinished(p);
}
