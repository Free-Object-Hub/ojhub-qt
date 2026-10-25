#ifndef OH_DOMAIN_PORTFOLIO_PORTFOLIODOMAIN_H
#define OH_DOMAIN_PORTFOLIO_PORTFOLIODOMAIN_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "../../api/forms.h"

class Session;
class Network;

// Portfolio domain: the user's portfolio items (add, edit, delete).
class PortfolioDomain : public QObject
{
    Q_OBJECT
public:
    PortfolioDomain(Session *session, Network *network, QObject *parent = 0);

    void saveWork(const WorkForm &f);
    void deleteWork(int id);

signals:
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
