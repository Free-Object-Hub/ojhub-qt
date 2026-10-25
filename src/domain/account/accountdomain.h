#ifndef OH_DOMAIN_ACCOUNT_ACCOUNTDOMAIN_H
#define OH_DOMAIN_ACCOUNT_ACCOUNTDOMAIN_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QSet>
#include <QHash>
#include <QStringList>
#include <functional>
#include <QNetworkReply>

class Session;
class Network;

// One ALTCHA challenge request. Owned by its QNetworkReply, which it outlives only briefly.
class AltchaFetch : public QObject
{
    Q_OBJECT
public:
    AltchaFetch(QNetworkReply *reply, const std::function<void(const QString &, const QString &)> &done);

private slots:
    void onFinished();

private:
    std::function<void(const QString &, const QString &)> m_done;
};

// Account domain: login, registration, saved login (ObjectHub.ini), logout, and the ALTCHA
// challenge they need. Knows nothing about UI or about owned/liked objects: it reports results
// via signals and ApiClient decides what to load next.
class AccountDomain : public QObject
{
    Q_OBJECT
public:
    AccountDomain(Session *session, Network *network, QObject *parent = 0);

    // Puts the token and userId saved in ObjectHub.ini into the session. True if one was found.
    bool restore();

    void login(const QString &username, const QString &password);
    void registerUser(const QString &username, const QString &password, const QString &email);
    void logout();

    // Account data (loginT and likesT): owned projects, likes and subscriptions.
    bool accountLoaded() const;
    bool ownsProject(int projectId) const;
    bool isSubscribed(int projectId) const;
    int likeState(int typeCode, int id) const;   // 1 like, -1 dislike, 0 none
    void applyAccountData(const QByteArray &data);
    void loadAccount();                          // GET loginT.php
    void fetchLikesAndSubs();                    // GET likesT.php
    void recordVote(int typeCode, int id, bool dislike);
    void recordSubscription(int projectId, bool subscribe);

signals:
    // accountData is the raw login/register answer, used by ApiClient to load owned objects.
    void loginResult(bool ok, const QString &message, const QByteArray &accountData);
    void accountChanged();
    void actionFinished(int action, bool ok, const QString &message, int id);
    // The server no longer accepts the saved token: ApiClient drops the login.
    void sessionExpired();
    void registerResult(bool ok, const QString &message, const QByteArray &accountData);

private slots:
    void onLoginReplyFinished();
    void onRegisterReplyFinished();
    void onAccountReplyFinished();

private:
    void fetchAltcha(const std::function<void(const QString &, const QString &)> &done);
    QNetworkRequest makeRequest(const QUrl &url) const;

    Session *m_session;
    Network *m_network;
    QNetworkAccessManager *m_nam;

    QSet<int> m_owned;
    QSet<int> m_subs;
    QHash<QString, QHash<int, int> > m_likes;
    bool m_accountLoaded;
};

#endif
