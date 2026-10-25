#ifndef OH_DOMAIN_NEWS_NEWSDOMAIN_H
#define OH_DOMAIN_NEWS_NEWSDOMAIN_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "../../api/forms.h"

class Session;
class Network;

// News domain: save, delete, and the owner check for a news item.
class NewsDomain : public QObject
{
    Q_OBJECT
public:
    NewsDomain(Session *session, Network *network, QObject *parent = 0);

    void saveNews(const NewsForm &f, const QString &filePath);
    void deleteNews(int id);
    void probeNews(int id, int projectId);

signals:
    void newsOwnership(int id, bool owned);
    void actionFinished(int action, bool ok, const QString &message, int id);

private slots:
    void onReplyFinished();

private:
    QNetworkRequest makeRequest(const QUrl &url) const;

    Session *m_session;
    Network *m_network;
    QNetworkAccessManager *m_nam;
};

#endif
