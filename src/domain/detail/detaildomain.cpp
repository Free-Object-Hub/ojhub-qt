#include "detaildomain.h"
#include "../catalog/catalogparse.h"
#include "../../core/session.h"
#include "../../core/network.h"
#include "../../core/protocol.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

DetailDomain::DetailDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
{
}

QNetworkRequest DetailDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

// Comments are shown 10 per page; the 11th item means "there is a next page".
static const int kCommentPage = 10;
// Comment type code the JS client uses for each item kind: project 0, news 3, vacancy 5.
static int commentTypeOfKind(int kind)
{
    switch (kind) {
    case DetailProject: return 0;
    case DetailNews:    return 3;
    case DetailVacancy: return 5;
    default:            return -1;
    }
}
// Work item: [id, gdpsTitle, gdpsChannel, workTitle, text, tags, linkOrGdpsId, owner, date]
static SearchResult parseWork(const QJsonArray &a)
{
    auto at = [&a](int k) -> QJsonValue { return k < a.size() ? a.at(k) : QJsonValue(); };
    SearchResult r;
    r.id = at(0).toInt();
    r.title = at(3).toString();
    r.shortDesc = at(4).toString();
    r.tags = normalizeTags(at(5));
    r.raw.insert(QStringLiteral("gdpsTitle"), at(1).toVariant());
    r.raw.insert(QStringLiteral("gdpsChannel"), at(2).toInt());
    r.raw.insert(QStringLiteral("link"), at(6).toVariant());
    r.raw.insert(QStringLiteral("date"), at(8).toString());
    return r;
}
// Comment, as the JS client reads it: an array
//   [id, username, text, userId, userRole, [likes, dislikes], date]
// Object form is accepted too, in case some responses use named keys.
static SearchResult parseComment(const QJsonValue &v)
{
    SearchResult r;
    if (v.isArray()) {
        QJsonArray a = v.toArray();
        auto at = [&a](int k) -> QJsonValue { return k < a.size() ? a.at(k) : QJsonValue(); };
        r.id = at(0).toInt();
        r.author = at(1).toString();
        r.shortDesc = at(2).toString();
        applyLikes(r, at(5));
        r.raw.insert(QStringLiteral("userId"), at(3).toInt());
        r.raw.insert(QStringLiteral("role"), at(4).toInt());
        r.raw.insert(QStringLiteral("date"), at(6).toString());
    } else if (v.isObject()) {
        const QJsonObject o = v.toObject();
        r.id = o.value(QStringLiteral("ID")).toInt(o.value(QStringLiteral("id")).toInt());
        r.author = firstString(o, QStringList() << QStringLiteral("username")
                                                << QStringLiteral("author")
                                                << QStringLiteral("name"));
        r.shortDesc = firstString(o, QStringList() << QStringLiteral("text")
                                                   << QStringLiteral("comment")
                                                   << QStringLiteral("message")
                                                   << QStringLiteral("content")
                                                   << QStringLiteral("body"));
        applyLikes(r, o.value(QStringLiteral("likes")));
        r.raw = o.toVariantMap();
    }
    return r;
}
static SearchResultList parseComments(const QJsonValue &v)
{
    SearchResultList out;
    if (v.isObject()) {
        QJsonObject o = v.toObject();
        QStringList keys = o.keys();
        for (int i = 0; i < keys.size(); ++i)
            out.append(parseComment(o.value(keys.at(i))));
    } else if (v.isArray()) {
        QJsonArray a = v.toArray();
        for (int i = 0; i < a.size(); ++i)
            out.append(parseComment(a.at(i)));
    }
    return out;
}

DetailData DetailDomain::parseDetailResponse(const QByteArray &data, int kind, int id, int page, int extra) const
{
    DetailData d;
    d.kind = kind;
    d.id = id;
    d.page = page;

    const QByteArray trimmed = data.trimmed();
    // The JS client treats '["NONE"]' as "no such item" and so do we.
    if (trimmed.isEmpty() || trimmed == "[\"NONE\"]" || trimmed == "null")
        return d;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return d;

    switch (kind) {
    case DetailProjectNews:
        // Same shape as the news list: an object keyed by news id.
        d.main.id = id;   // keep main.id in sync with d.id for stale-reply checks
        d.news = parseNewsList(trimmed, kNewsPage, &d.hasMoreNews);
        d.found = true;
        return d;

    case DetailCommentsPage: {
        // The reply is the comments list itself, not wrapped in gdps.
        const QJsonValue rootVal = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
        d.main.id = id;   // keep main.id in sync with d.id for stale-reply checks
        d.contentType = extra;
        d.comments = parseComments(rootVal);
        truncateTo(d.comments, kCommentPage, &d.hasMoreComments);
        d.commentsPage = page;
        d.found = true;
        return d;
    }

    case DetailProfile: {
        if (!doc.isObject())
            return d;
        const QJsonObject o = doc.object();
        d.main.id = id;
        d.main.title = o.value(QStringLiteral("username")).toString();
        d.main.author = d.main.title;
        d.main.shortDesc = o.value(QStringLiteral("resume")).toString();
        d.main.raw = o.toVariantMap();
        d.found = !o.isEmpty();
        return d;
    }

    case DetailProfileWorks: {
        // Array: [userObject, work, work, ...]. Works are arrays; the user object comes first.
        if (!doc.isArray())
            return d;
        const QJsonArray a = doc.array();
        for (int i = 0; i < a.size(); ++i) {
            const QJsonValue e = a.at(i);
            if (e.isObject()) {
                const QJsonObject u = e.toObject();
                d.main.id = id;
                d.main.title = u.value(QStringLiteral("username")).toString();
                d.main.author = d.main.title;
            } else if (e.isArray()) {
                d.items.append(parseWork(e.toArray()));
            }
        }
        d.found = true;
        return d;
    }

    case DetailProfileAdded: {
        // Array: [username(string), project, project, ...]
        if (!doc.isArray())
            return d;
        const QJsonArray a = doc.array();
        d.main.id = id;
        d.main.title = a.size() > 0 ? a.at(0).toString() : QString();
        d.main.author = d.main.title;
        // Second element is an object keyed "g<id>", not a list of projects.
        if (a.size() > 1 && a.at(1).isObject()) {
            const QJsonObject items = a.at(1).toObject();
            const QStringList keys = items.keys();
            for (int i = 0; i < keys.size(); ++i) {
                const QJsonObject o = items.value(keys.at(i)).toObject();
                d.items.append(parseItem(o, o.value(QStringLiteral("channel")).toInt(), keys.at(i)));
            }
        }
        d.found = true;
        return d;
    }

    default:
        break;
    }

    // Project, vacancy, news: { "gdpsel": {...}, "comments": ..., "news": ... }
    if (!doc.isObject())
        return d;
    const QJsonObject root = doc.object();
    const QJsonObject gdps = root.value(QStringLiteral("gdps")).toObject();
    QString prefix;
    if (kind == DetailVacancy)
        prefix = QStringLiteral("v");
    else if (kind == DetailNews)
        prefix = QStringLiteral("n");
    const QString key = prefix + QString::number(id);

    if (kind == DetailNews) {
        // newsC.php answers {"gdps": {"n<id>": [array]}, "comments": [...]}. The item is an array, not an object.
        const QJsonValue nv = gdps.value(key);
        if (!nv.isArray() && !nv.isObject())
            return d;
        d.main = parseNewsItem(nv);
        if (d.main.id == 0)
            d.main.id = id;
        d.found = true;
        d.contentType = commentTypeOfKind(kind);
        d.comments = parseComments(root.value(QStringLiteral("comments")));
        truncateTo(d.comments, kCommentPage, &d.hasMoreComments);
        d.commentsPage = 0;
        return d;
    }

    const QJsonObject item = gdps.value(key).isObject() ? gdps.value(key).toObject() : gdps;
    if (item.isEmpty())
        return d;

    const int channel = (kind == DetailVacancy) ? kChannelVacancies
                      : (kind == DetailNews)    ? kChannelNews
                      : item.value(QStringLiteral("channel")).toInt();
    d.main = (kind == DetailNews) ? parseNewsItem(item) : parseItem(item, channel, key);
    if (d.main.id == 0)
        d.main.id = id;
    d.found = true;

    d.contentType = commentTypeOfKind(kind);
    d.comments = parseComments(root.value(QStringLiteral("comments")));
    truncateTo(d.comments, kCommentPage, &d.hasMoreComments);
    d.commentsPage = 0;

    if (kind == DetailProject) {
        const QJsonValue nv = root.value(QStringLiteral("news"));
        if (nv.isArray() || nv.isObject()) {
            // GetLocalNews answers an array; re-serialise and reuse the list parser.
            const QByteArray newsJson = nv.isArray()
                ? QJsonDocument(nv.toArray()).toJson(QJsonDocument::Compact)
                : QJsonDocument(nv.toObject()).toJson(QJsonDocument::Compact);
            d.news = parseNewsList(newsJson, kNewsPage, &d.hasMoreNews);
        }
    }
    return d;
}
void DetailDomain::fetchDetail(int kind, int id, int page, int extra)
{
    QString path;
    QUrlQuery query;
    switch (kind) {
    case DetailProject:
        path = QString::fromLatin1("/server/133/content/camp.php");
        break;
    case DetailVacancy:
        path = QString::fromLatin1("/server/133/content/vacsC.php");
        break;
    case DetailNews:
        path = QString::fromLatin1("/server/133/content/newsC.php");
        break;
    case DetailProjectNews:
        path = QString::fromLatin1("/server/133/content/news.php");
        query.addQueryItem(QString::fromLatin1("page"), QString::number(page));
        break;
    case DetailProfile:
        path = QString::fromLatin1("/server/133/content/getUser.php");
        break;
    case DetailProfileWorks:
        path = QString::fromLatin1("/server/133/user/works.php");
        break;
    case DetailProfileAdded:
        // extra = project channel: 0 camps, 1 shows, 2 peres
        if (extra == 1)
            path = QString::fromLatin1("/server/133/content/getAddedShows.php");
        else if (extra == 2)
            path = QString::fromLatin1("/server/133/content/getAddedPeres.php");
        else if (extra == 3)
            path = QString::fromLatin1("/server/133/content/getAddedTeles.php");
        else
            path = QString::fromLatin1("/server/133/content/getAddedCamps.php");
        break;
    default:
        return;
    }
    query.addQueryItem(QString::fromLatin1("id"), QString::number(id));

    QUrl url(m_session->baseUrl() + path);
    url.setQuery(query);

    QNetworkRequest request = makeRequest(url);
    logNetReq("DETAIL", QString::fromLatin1("GET"), url, QByteArray());
    QNetworkReply *reply = m_nam->get(request);
    reply->setProperty("kind", kind);
    reply->setProperty("id", id);
    reply->setProperty("page", page);
    reply->setProperty("extra", extra);
    connect(reply, SIGNAL(finished()), this, SLOT(onDetailReplyFinished()));
}
void DetailDomain::onDetailReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    const QByteArray data = reply->readAll();
    logNetResp("DETAIL", reply, data);

    if (reply->error() != QNetworkReply::NoError) {
        emit detailError(reply->errorString());
        return;
    }

    emit detailFinished(parseDetailResponse(data,
                                            reply->property("kind").toInt(),
                                            reply->property("id").toInt(),
                                            reply->property("page").toInt(),
                                            reply->property("extra").toInt()));
}
void DetailDomain::fetchComments(int contentType, int id, int page)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/content/fetchComms.php"));
    QUrlQuery query;
    query.addQueryItem(QString::fromLatin1("id"), QString::number(id));
    query.addQueryItem(QString::fromLatin1("type"), QString::number(contentType));
    query.addQueryItem(QString::fromLatin1("page"), QString::number(page));
    url.setQuery(query);

    QNetworkRequest request = makeRequest(url);
    logNetReq("COMMENTS", QString::fromLatin1("GET"), url, QByteArray());
    QNetworkReply *reply = m_nam->get(request);
    reply->setProperty("kind", static_cast<int>(DetailCommentsPage));
    reply->setProperty("id", id);
    reply->setProperty("page", page);
    reply->setProperty("extra", contentType);
    connect(reply, SIGNAL(finished()), this, SLOT(onDetailReplyFinished()));
}
SearchResultList DetailDomain::commentsFromAnswer(const QByteArray &data, bool *hasMore) const
{
    SearchResultList list;
    if (hasMore)
        *hasMore = false;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return list;
    list = parseComments(QJsonValue(doc.array()));
    truncateTo(list, kCommentPage, hasMore);
    return list;
}
