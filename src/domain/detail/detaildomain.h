#ifndef OH_DOMAIN_DETAIL_DETAILDOMAIN_H
#define OH_DOMAIN_DETAIL_DETAILDOMAIN_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "../../api/searchresult.h"

class Session;
class Network;

// Detail domain: a project, vacancy or news page, and comment pages for them.
class DetailDomain : public QObject
{
    Q_OBJECT
public:
    DetailDomain(Session *session, Network *network, QObject *parent = 0);

    void fetchDetail(int kind, int id, int page, int extra = 0);
    void fetchComments(int contentType, int id, int page);
    // Parses a full comments answer (array) into the first page.
    SearchResultList commentsFromAnswer(const QByteArray &data, bool *hasMore) const;

signals:
    void detailFinished(const DetailData &data);
    void detailError(const QString &error);

private slots:
    void onDetailReplyFinished();

private:
    QNetworkRequest makeRequest(const QUrl &url) const;
    DetailData parseDetailResponse(const QByteArray &data, int kind, int id, int page, int extra) const;

    Session *m_session;
    Network *m_network;
    QNetworkAccessManager *m_nam;
};

#endif
