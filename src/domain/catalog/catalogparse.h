#ifndef OH_DOMAIN_CATALOG_CATALOGPARSE_H
#define OH_DOMAIN_CATALOG_CATALOGPARSE_H

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>
#include "../../api/searchresult.h"

// Parsing of catalog items, shared by the catalog (lists) and the detail page.
enum { kPageSize = 8, kNewsPage = 10 };   // items per page: search/vacancies, news

QStringList normalizeTags(const QJsonValue &v);
QString firstString(const QJsonObject &o, const QStringList &keys);
SearchResult parseItem(const QJsonObject &item, int channel, const QString &key);
void applyLikes(SearchResult &r, const QJsonValue &lv);
SearchResult parseNewsItem(const QJsonValue &v);
SearchResultList parseNewsList(const QByteArray &data, int pageSize, bool *hasMore);
void truncateTo(SearchResultList &list, int n, bool *more);
int projectChannelFromKey(const QString &key);

// Top-level keys of a JSON object in the order they appear in the text.
// QJsonObject sorts keys alphabetically, which would throw away the server's ranking.
QStringList topLevelKeysInOrder(const QByteArray &json);

#endif
