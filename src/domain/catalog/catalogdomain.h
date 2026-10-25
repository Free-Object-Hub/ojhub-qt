#ifndef OH_DOMAIN_CATALOG_CATALOGDOMAIN_H
#define OH_DOMAIN_CATALOG_CATALOGDOMAIN_H

#include <QObject>
#include <QString>
#include <QList>
#include <QByteArray>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QPointer>

#include "../../api/searchresult.h"

class Session;
class Network;

// Catalog domain: project search, the news list and the vacancy list. All three use the same
// request/reply path; the channel selects the endpoint and the parser.
class CatalogDomain : public QObject
{
    Q_OBJECT
public:
    CatalogDomain(Session *session, Network *network, QObject *parent = 0);

    // channel: 0-3 projects, -1 wiki, -5 vacancies, -10 = all projects
    void search(const QString &name,
                int channel = -10,
                int method = 0,
                int page = 0,
                const QList<int> &tags = QList<int>(),
                const QList<int> &os = QList<int>());
    void fetchNews(int page);
    void fetchVacancies(int page);

signals:
    void searchFinished(const SearchResultList &results, bool hasMore, int page);
    void searchError(const QString &error);
    void networkError(const QString &error);

private slots:
    void onSearchReplyFinished();

private:
    void replaceListReply(QNetworkReply *reply);
    QNetworkRequest makeRequest(const QUrl &url) const;
    SearchResultList parseSearchResponse(const QByteArray &data, int channel, bool *hasMore,
                                         int pageSize = 8) const;

    // The one pending search / news / vacancy request. Older ones are aborted when a new one starts.
    QPointer<QNetworkReply> m_listReply;

    Session *m_session;
    Network *m_network;
    QNetworkAccessManager *m_nam;
};

#endif
