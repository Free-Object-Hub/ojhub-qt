#include "socialdomain.h"
#include "../../lang.h"
#include "../../api/forms.h"
#include "../../core/session.h"
#include "../../core/network.h"
#include "../../core/protocol.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

SocialDomain::SocialDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
{
}

QNetworkRequest SocialDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

void SocialDomain::vote(int typeCode, int id, bool dislike)
{
    // LikeSend/DislSend read ide and type from the query. Calling again removes the vote.
    QUrl url(m_session->baseUrl() + QString::fromLatin1(dislike ? "/server/133/send/dislike.php"
                                                     : "/server/133/send/like.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("ide"), QString::number(id));
    q.addQueryItem(QStringLiteral("type"), QString::number(typeCode));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->post(makeRequest(url), QByteArray());
    reply->setProperty("dislike", dislike);
    tagReply(reply, ActVote, id, typeCode);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::postComment(int contentType, int id, const QString &text)
{
    // CommentSend calls ParseForm before reading fields: urlencoded body.
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/send/comment.php"));
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("ide"), QString::number(id));
    body.addQueryItem(QStringLiteral("type"), QString::number(contentType));
    body.addQueryItem(QStringLiteral("text"), text);
    QNetworkRequest req = makeRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QVariant(QString::fromLatin1("application/x-www-form-urlencoded")));
    QNetworkReply *reply = m_nam->post(req, body.query(QUrl::FullyEncoded).toUtf8());
    tagReply(reply, ActPostComment, id, contentType);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::editComment(int commentId, const QString &text)
{
    // CommentModify calls ParseForm before reading fields: urlencoded body.
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/send/commentModify.php"));
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("id"), QString::number(commentId));
    body.addQueryItem(QStringLiteral("text"), text);
    QNetworkRequest req = makeRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QVariant(QString::fromLatin1("application/x-www-form-urlencoded")));
    QNetworkReply *reply = m_nam->post(req, body.query(QUrl::FullyEncoded).toUtf8());
    tagReply(reply, ActEditComment, commentId, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::deleteComment(int commentId, int contentType)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/delete/comment.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("ide"), QString::number(commentId));
    q.addQueryItem(QStringLiteral("type"), QString::number(contentType));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActDeleteComment, commentId, contentType);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::setNickname(const QString &nick)
{
    // SetNicknameHandler reads the query parameter "name"
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/user/setNickname.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("name"), nick);
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActSaveNick, 0, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::setResume(const QString &text)
{
    // POST urlencoded: FormValue("name"). Newlines must arrive as the two characters \n.
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/user/setResume.php"));
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("name"), text);
    QNetworkRequest req = makeRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QVariant(QString::fromLatin1("application/x-www-form-urlencoded")));
    QNetworkReply *reply = m_nam->post(req, body.query(QUrl::FullyEncoded).toUtf8());
    tagReply(reply, ActSaveResume, 0, 1);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::setSocials(const QString &text)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/user/setSocials.php"));
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("name"), text);
    QNetworkRequest req = makeRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QVariant(QString::fromLatin1("application/x-www-form-urlencoded")));
    QNetworkReply *reply = m_nam->post(req, body.query(QUrl::FullyEncoded).toUtf8());
    tagReply(reply, ActSaveSocials, 0, 2);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::subscribe(int projectId, bool unsubscribe)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1(unsubscribe ? "/server/133/gdps/unsub.php"
                                                         : "/server/133/gdps/sub.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), QString::number(projectId));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, unsubscribe ? ActUnsub : ActSub, projectId, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::fetchSubscriptions(int page, int probeId)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/gdps/subs.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActLoadSubs, probeId, page);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}
void SocialDomain::onSocialReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    const int action = reply->property("action").toInt();
    const int id = reply->property("id").toInt();
    const int channel = reply->property("channel").toInt();
    const bool dislike = reply->property("dislike").toBool();
    const QByteArray data = reply->readAll().trimmed();

    if (reply->error() != QNetworkReply::NoError) {
        emit actionFinished(action, false, reply->errorString(), id);
        return;
    }

    switch (action) {
    case ActVote:
        emit voteAnswered(channel, id, dislike, data);   // state update happens in ApiClient
        return;
    case ActPostComment:
        if (data.startsWith('[')) {
            emit commentsAnswered(channel, id, data);
            return;
        }
        if (data == "-4") {
            emit actionFinished(action, false, L("commSizeFail"), id);
            return;
        }
        emit actionFinished(action, false, errorMessage(data), id);
        return;
    case ActEditComment:
        if (data == "-4") {
            emit actionFinished(action, false, L("commSizeFail"), id);
            return;
        }
        if (data.isEmpty() || data.startsWith('-') || data == "Access denied") {
            emit actionFinished(action, false, errorMessage(data), id);
            return;
        }
        emit commentEdited(id, QString::fromUtf8(data));
        return;
    case ActDeleteComment:
        if (data == QByteArray::number(id)) {
            emit commentDeleted(id);
            return;
        }
        emit actionFinished(action, false, serverErrorText(QStringLiteral("-2")), id);
        return;
    case ActSaveNick:
    case ActSaveResume:
    case ActSaveSocials:
        if (data.isEmpty() || data == "Access denied" || data.startsWith('-')) {
            emit actionFinished(action, false, errorMessage(data), id);
            return;
        }
        emit profileFieldSaved(channel, QString::fromUtf8(data));
        return;
    case ActSub:
    case ActUnsub:
        emit subscriptionAnswered(id, action == ActSub, data);   // state update happens in ApiClient
        return;
    case ActLoadSubs: {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isArray()) {
            emit actionFinished(action, false, errorMessage(data), id);
            return;
        }
        QVariantList out;
        const QJsonArray a = doc.array();
        for (int i = 0; i < a.size(); ++i) {
            if (a.at(i).isObject())
                out.append(a.at(i).toObject().toVariantMap());
        }
        // The server returns 21 rows per page (LIMIT 21): the 21st means there is more.
        const bool hasMore = out.size() > 20;
        while (out.size() > 20)
            out.removeLast();
        emit subscriptionsLoaded(out, channel, hasMore, id);
        return;
    }
    case ActLoadAlarms: {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isArray()) {
            emit actionFinished(action, false, errorMessage(data), id);
            return;
        }
        // Rows are [id, title, public]; public 1 = unread, 2 = read
        QVariantList out;
        const QJsonArray a = doc.array();
        for (int i = 0; i < a.size(); ++i) {
            if (!a.at(i).isArray())
                continue;
            const QJsonArray row = a.at(i).toArray();
            QVariantMap m;
            m.insert(QStringLiteral("ID"), row.size() > 0 ? row.at(0).toInt() : 0);
            m.insert(QStringLiteral("title"), row.size() > 1 ? row.at(1).toString() : QString());
            m.insert(QStringLiteral("public"), row.size() > 2 ? row.at(2).toInt() : 0);
            out.append(m);
        }
        // Server returns 11 rows per page (LIMIT 11): the 11th means there is more.
        const bool hasMore = out.size() > 10;
        while (out.size() > 10)
            out.removeLast();
        emit alarmsLoaded(out, channel, hasMore);
        return;
    }
    case ActLoadAlarm: {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject() || !doc.object().contains(QStringLiteral("ID"))) {
            emit actionFinished(action, false, QString::fromUtf8("Notification not found"), id);   // LANGXXX
            return;
        }
        emit alarmLoaded(doc.object().toVariantMap());
        return;
    }
    case ActDeleteAlarm: {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject() || !doc.object().contains(QStringLiteral("ID"))) {
            emit actionFinished(action, false, QString::fromUtf8("Could not delete"), id);   // LANGXXX
            return;
        }
        emit alarmDeleted(id);
        return;
    }
    default:
        return;
    }
}

void SocialDomain::fetchAlarms(int page)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/content/getAlarms.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActLoadAlarms, 0, page);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}

void SocialDomain::fetchAlarm(int id)
{
    // Opening an alarm marks it read on the server (UpdateAlarm)
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/content/getAlarm.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), QString::number(id));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActLoadAlarm, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}

void SocialDomain::deleteAlarm(int id)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/send/deleteAlarm.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), QString::number(id));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActDeleteAlarm, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onSocialReplyFinished()));
}

