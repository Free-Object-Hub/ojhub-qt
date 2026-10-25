#include "catalogdomain.h"
#include "catalogparse.h"
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

CatalogDomain::CatalogDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
{
}

QNetworkRequest CatalogDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

void CatalogDomain::search(const QString &name, int channel, int method, int page,
                       const QList<int> &tags, const QList<int> &os)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/search/new.php"));
    QUrlQuery query;

    if (!name.isEmpty())
        query.addQueryItem(QString::fromLatin1("name"), name);

    // Always sent. Old GDPS Helper builds omit it and get camps only; -10 = whole catalog.
    query.addQueryItem(QString::fromLatin1("channel"), QString::number(channel));

    query.addQueryItem(QString::fromLatin1("method"), QString::number(method));
    query.addQueryItem(QString::fromLatin1("page"), QString::number(page));

    for (int i = 0; i < tags.size(); ++i)
        query.addQueryItem(QString::fromLatin1("tags[]"), QString::number(tags.at(i)));
    for (int i = 0; i < os.size(); ++i)
        query.addQueryItem(QString::fromLatin1("os[]"), QString::number(os.at(i)));

    url.setQuery(query);

    QNetworkRequest request = makeRequest(url);
    logNetReq("SEARCH", QString::fromLatin1("GET"), url, QByteArray());
    QNetworkReply *reply = m_nam->get(request);
    replaceListReply(reply);
    reply->setProperty("channel", channel);
    reply->setProperty("page", page);

    connect(reply, SIGNAL(finished()), this, SLOT(onSearchReplyFinished()));
}

void CatalogDomain::onSearchReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    // A newer list request has replaced this one: drop the answer without touching the list.
    if (reply != m_listReply.data()) {
        reply->deleteLater();
        return;
    }
    m_listReply = 0;
    reply->deleteLater();

    const QByteArray data = reply->readAll();
    logNetResp("SEARCH", reply, data);

    if (reply->error() != QNetworkReply::NoError) {
        emit searchError(reply->errorString());
        emit networkError(reply->errorString());
        return;
    }

    if (data.trimmed() == "Access denied") {
        // Not an empty result: the request was refused.
        emit searchError(QString::fromUtf8("Access denied"));
        return;
    }

    const int channel = reply->property("channel").toInt();
    const int page = reply->property("page").toInt();
    bool hasMore = false;
    // News pages are 10 items, search/vacancy pages are 8 (matches the JS thresholds).
    const int pageSize = (channel == kChannelNews) ? kNewsPage : kPageSize;
    SearchResultList results = (channel == kChannelNews)
        ? parseNewsList(data, pageSize, &hasMore)
        : parseSearchResponse(data, channel, &hasMore, pageSize);
    emit searchFinished(results, hasMore, page);
}

void CatalogDomain::fetchNews(int page)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/content/newsAll.php"));
    QUrlQuery query;
    query.addQueryItem(QString::fromLatin1("page"), QString::number(page));
    url.setQuery(query);

    QNetworkRequest request = makeRequest(url);
    logNetReq("NEWS", QString::fromLatin1("GET"), url, QByteArray());
    QNetworkReply *reply = m_nam->get(request);
    replaceListReply(reply);
    // Reuses the search reply handler: the channel property selects the page size.
    reply->setProperty("channel", kChannelNews);
    reply->setProperty("page", page);
    connect(reply, SIGNAL(finished()), this, SLOT(onSearchReplyFinished()));
}

// Whole vacancy list, no query (JS globalVacs). Paged like search, 8 per page.
void CatalogDomain::fetchVacancies(int page)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/vacans/getAll.php"));
    QUrlQuery query;
    query.addQueryItem(QString::fromLatin1("page"), QString::number(page));
    url.setQuery(query);

    QNetworkRequest request = makeRequest(url);
    logNetReq("VACS", QString::fromLatin1("GET"), url, QByteArray());
    QNetworkReply *reply = m_nam->get(request);
    replaceListReply(reply);
    reply->setProperty("channel", kChannelVacancies);
    reply->setProperty("page", page);
    connect(reply, SIGNAL(finished()), this, SLOT(onSearchReplyFinished()));
}

SearchResultList CatalogDomain::parseSearchResponse(const QByteArray &data, int channel,
                                                bool *hasMore, int pageSize) const
{
    SearchResultList list;
    if (hasMore)
        *hasMore = false;

    /* openGo fullSearch: empty → "null"; errors often plain text / http.Error */
    const QByteArray trimmed = data.trimmed();
    if (trimmed.isEmpty() || trimmed == "null" || trimmed == "Access denied")
        return list;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "SEARCH JSON parse error:" << parseError.errorString()
                   << "body:" << QString::fromUtf8(trimmed.left(200));
        return list;
    }

    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        // Server order matters: the ranking and the page cut both depend on it.
        QStringList keys = topLevelKeysInOrder(trimmed);
        if (keys.size() != obj.size())
            keys = obj.keys();              // scanner disagreed with the parser: keep it safe

        for (int i = 0; i < keys.size(); ++i) {
            const QString key = keys.at(i);
            QJsonValue val = obj.value(key);
            if (!val.isObject())
                continue;

            // In "all projects" mode the channel of each item comes from its key prefix (c/s/p/t)
        SearchResult r = parseItem(val.toObject(),
                                   channel == kChannelAll ? projectChannelFromKey(key) : channel,
                                   key);

            list.append(r);
        }
    }
    else if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (int i = 0; i < arr.size(); ++i) {
            QJsonValue val = arr.at(i);
            SearchResult r;
            r.channel = channel;

            if (val.isArray()) {
                QJsonArray row = val.toArray();
                if (row.size() > 0)
                    r.id = row.at(0).toInt();
                if (row.size() > 1)
                    r.title = row.at(1).toString();
                if (row.size() > 2)
                    r.shortDesc = row.at(2).toString();
            }
            else if (val.isObject()) {
                QJsonObject item = val.toObject();
                r.id = item.value(QString::fromLatin1("ID")).toInt();
                r.title = item.value(QString::fromLatin1("title")).toString();
                r.shortDesc = item.value(QString::fromLatin1("short")).toString();
                r.raw = item.toVariantMap();
            }

            if (!r.title.isEmpty() || r.id > 0)
                list.append(r);
        }
    }

    // Render at most one page (8). A 9th result is only a signal that more pages exist;
    // it is not rendered here and is fetched with the next page.
    if (list.size() > pageSize) {
        if (hasMore)
            *hasMore = true;
        list = list.mid(0, pageSize);
    }
    return list;
}

void CatalogDomain::replaceListReply(QNetworkReply *reply)
{
    // The UI shows one list at a time. A new request makes any pending one obsolete, so abort it.
    // The aborted reply still emits finished; the handler sees it is no longer current and ignores it.
    QNetworkReply *old = m_listReply.data();
    m_listReply = reply;
    if (old)
        old->abort();
}
