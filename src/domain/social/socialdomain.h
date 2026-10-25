#ifndef OH_DOMAIN_SOCIAL_SOCIALDOMAIN_H
#define OH_DOMAIN_SOCIAL_SOCIALDOMAIN_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

class Session;
class Network;

// Social domain: votes, comments (write), profile fields, subscriptions. Builds the requests and
// parses the answers. Answers that change account state (likes, subscriptions) are passed on raw:
// the account state still lives in ApiClient until the account domain takes it over.
class SocialDomain : public QObject
{
    Q_OBJECT
public:
    SocialDomain(Session *session, Network *network, QObject *parent = 0);

    // Likes. typeCode: item 0 project, 2 news, 11 vacancy; comment 3 project, 5 news, 12 vacancy.
    void vote(int typeCode, int id, bool dislike);
    // Comments. contentType: 0 project, 3 news, 5 vacancy.
    void postComment(int contentType, int id, const QString &text);
    void editComment(int commentId, const QString &text);
    void deleteComment(int commentId, int contentType);
    // Profile of the logged-in user. field: 0 nickname, 1 resume, 2 socials.
    void setNickname(const QString &nick);
    void setResume(const QString &text);
    void setSocials(const QString &text);
    // Subscriptions to projects. page: 0-based.
    void subscribe(int projectId, bool unsubscribe);
    void fetchSubscriptions(int page, int probeId = 0);

    // Notifications: the list, one notification (opening it marks it read), and deletion.
    void fetchAlarms(int page);
    void fetchAlarm(int id);
    void deleteAlarm(int id);

signals:
    void voteAnswered(int typeCode, int id, bool dislike, const QByteArray &data);
    void commentsAnswered(int contentType, int id, const QByteArray &data);
    void subscriptionAnswered(int projectId, bool subscribe, const QByteArray &data);
    void commentEdited(int commentId, const QString &text);
    void commentDeleted(int commentId);
    void profileFieldSaved(int field, const QString &value);
    void subscriptionsLoaded(const QVariantList &items, int page, bool hasMore, int probeId);
    void alarmsLoaded(const QVariantList &items, int page, bool hasMore);
    void alarmLoaded(const QVariantMap &alarm);
    void alarmDeleted(int id);
    void actionFinished(int action, bool ok, const QString &message, int id);

private slots:
    void onSocialReplyFinished();

private:
    QNetworkRequest makeRequest(const QUrl &url) const;

    Session *m_session;
    Network *m_network;
    QNetworkAccessManager *m_nam;
};

#endif
