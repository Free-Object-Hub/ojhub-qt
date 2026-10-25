#ifndef OH_DOMAIN_PROFILE_PROFILEDOMAIN_H
#define OH_DOMAIN_PROFILE_PROFILEDOMAIN_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "../../api/searchresult.h"

class Session;
class Network;

// Profile domain: public profile of any user (the logged-in user's own edits live in social).
class ProfileDomain : public QObject
{
    Q_OBJECT
public:
    ProfileDomain(Session *session, Network *network, QObject *parent = 0);

    void fetchProfile(int userId);

signals:
    void profileFinished(const PublicProfile &profile);
    void profileError(const QString &error);

private slots:
    void onProfileReplyFinished();

private:
    QNetworkRequest makeRequest(const QUrl &url) const;
    PublicProfile parseProfileResponse(const QByteArray &data) const;

    Session *m_session;
    Network *m_network;
    QNetworkAccessManager *m_nam;
};

#endif
