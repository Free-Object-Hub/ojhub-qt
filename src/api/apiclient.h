#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <functional>
#include <QUrl>
#include <QVariantMap>
#include <QList>
#include "searchresult.h"
#include <QVariantList>
#include <QVariantMap>
#include <QSet>
#include <QHash>
#include "forms.h"

class Session;
class Network;
class AccountDomain;
class CatalogDomain;
class DetailDomain;
class ProfileDomain;
class SocialDomain;
class ProjectsDomain;
class VacanciesDomain;
class NewsDomain;
class PortfolioDomain;

// Façade for Object Hub Protocol V133. The UI calls this class only.
// Owns the core Session and Network and one object per domain (src/domain/*).
// Domain signals are re-emitted here under the same names (see docs/ARCHITECTURE.md).

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = 0);

    Session *session() const;
    Network *network() const;

    void setBaseUrl(const QString &url);
    QString baseUrl() const;

    void setUserToken(const QString &token);
    void setDeviceToken(const QString &token);
    QString userToken() const;
    QString deviceToken() const;
    bool isLoggedIn() const;

    // Ensure we have a persistent device id (generated once)
    QString ensureDeviceToken();

    // Search
    // channel: 0-3 projects, -1 wiki, -5 vacancies, -10 = all projects
    void search(const QString &name,
                int channel = -10,
                int method = 0,
                int page = 0,
                const QList<int> &tags = QList<int>(),
                const QList<int> &os = QList<int>());

    // Auth: implemented in domain/account (AccountDomain); these forward to it.
    void restoreSession();                       // startup: reload the login saved in ObjectHub.ini
    void login(const QString &username, const QString &password);
    void registerUser(const QString &username, const QString &password, const QString &email);
    void logout();

    // Profile
    void fetchProfile(int userId);

    // News list, paged like search (10 per page, 11th = "more"). Results come back via
    // searchFinished, so the UI handles news and search with one code path.
    void fetchNews(int page);

    // Full vacancy list (no query), paged like search.
    void fetchVacancies(int page);

    // Detail page for a project, vacancy, news item, or one more page of project news.
    void fetchDetail(int kind, int id, int page, int extra = 0);

    // Next page of comments for a project (0), news (3) or vacancy (5).
    void fetchComments(int contentType, int id, int page);

    // Current user id from the last successful login (0 when logged out).
    int userId() const;

    // Edit forms: the server returns the stored values, emitted as projectFormLoaded/vacancyFormLoaded.
    void fetchProjectForm(int channel, int id);
    void fetchVacancyForm(int id);

    // Save. Add when id == 0, edit otherwise. imgFile/banFile, when set, are uploaded
    // instead of the url fields.
    void saveProject(const ProjectForm &f, const QString &imgFile, const QString &banFile);
    void saveVacancy(const VacancyForm &f);

    void removeVacancy(int id);
    void applyVacancy(int id);
    void unapplyVacancy(int applyId);

    // News. Add when form.id == 0, edit otherwise. filePath is an optional picture (add only).
    void saveNews(const NewsForm &f, const QString &filePath);
    void deleteNews(int id);

    // Account data (loginT and likesT): owned projects, likes and subscriptions.
    bool accountLoaded() const;
    bool ownsProject(int projectId) const;
    bool isSubscribed(int projectId) const;
    int likeState(int typeCode, int id) const;   // 1 like, -1 dislike, 0 none
    void loadAccount();                          // GET loginT.php
    void fetchLikesAndSubs();                    // GET likesT.php
    void applyAccountData(const QByteArray &data);

    // Portfolio: add when form.id == 0, edit otherwise. Delete by work id.
    void saveWork(const WorkForm &f);
    void deleteWork(int id);

    // Likes. typeCode: item 0 project, 2 news, 11 vacancy; comment 3 project, 5 news, 12 vacancy.
    void vote(int typeCode, int id, bool dislike);

    // Comments. contentType: 0 project, 3 news, 5 vacancy. postComment reloads the first page.
    void postComment(int contentType, int id, const QString &text);
    void editComment(int commentId, const QString &text);
    void deleteComment(int commentId, int contentType);

    // Parses a full comments answer (array) into the first page. Used by the reply handler.
    SearchResultList commentsFromAnswer(const QByteArray &data, bool *hasMore) const;

    // Profile of the logged-in user. field: 0 nickname, 1 resume, 2 socials.
    void setNickname(const QString &nick);
    void setResume(const QString &text);
    void setSocials(const QString &text);

    // Subscriptions to projects.
    void subscribe(int projectId, bool unsubscribe);
    // page: 0-based. probeId > 0: the answer is used only to find out whether that project is subscribed.
    void fetchSubscriptions(int page, int probeId = 0);

    // Owner checks: vacancy (edit endpoint answers -2 to non-owners) and news (empty answer for owners).
    void probeVacancy(int id);
    void probeNews(int id, int projectId);

    // Notifications (UI only, loaded when the user opens the list).
    void fetchAlarms(int page);
    void fetchAlarm(int id);
    void deleteAlarm(int id);

signals:
    // hasMore: the server returned a 9th result, so another page exists
    void searchFinished(const SearchResultList &results, bool hasMore, int page);
    void searchError(const QString &error);
    void networkError(const QString &error);

    void loginFinished(bool ok, const QString &message);
    void registerFinished(bool ok, const QString &message);
    void logoutFinished();

    void profileFinished(const PublicProfile &profile);
    void profileError(const QString &error);

    void detailFinished(const DetailData &data);
    void detailError(const QString &error);

    void projectFormLoaded(const ProjectForm &form);
    void vacancyFormLoaded(const VacancyForm &form);
    // action: WriteAction. id: object id after the call (new id for add, apply id for apply).
    void actionFinished(int action, bool ok, const QString &message, int id);

    void votesChanged(int typeCode, int id, int likes, int dislikes);
    void commentsUpdated(int contentType, int id, const SearchResultList &comments, bool hasMore);
    void commentEdited(int commentId, const QString &text);
    void commentDeleted(int commentId);

    void profileFieldSaved(int field, const QString &value);
    void subscriptionChanged(int id, bool subscribed);
    void subscriptionsLoaded(const QVariantList &items, int page, bool hasMore, int probeId);
    void alarmsLoaded(const QVariantList &items, int page, bool hasMore);
    void vacancyOwnership(int id, bool owned);
    void newsOwnership(int id, bool owned);
    void alarmLoaded(const QVariantMap &alarm);
    void alarmDeleted(int id);

    void accountChanged();

private slots:
    void onAccountLogin(bool ok, const QString &message, const QByteArray &accountData);
    void onAccountRegister(bool ok, const QString &message, const QByteArray &accountData);
    void onVoteAnswer(int typeCode, int id, bool dislike, const QByteArray &data);
    void onCommentsAnswer(int contentType, int id, const QByteArray &data);
    void onSubscriptionAnswer(int projectId, bool subscribe, const QByteArray &data);
    void onSessionExpired();
    void onAccountRefreshRequested();

private:
    Session *m_session;
    Network *m_network;
    AccountDomain *m_account;
    CatalogDomain *m_catalog;
    DetailDomain *m_detail;
    ProfileDomain *m_profile;
    SocialDomain *m_social;
    ProjectsDomain *m_projects;
    VacanciesDomain *m_vacancies;
    NewsDomain *m_news;
    PortfolioDomain *m_portfolio;

    // Convenience mirrors (same objects as session/network; kept so existing code paths stay short).


    QNetworkRequest makeRequest(const QUrl &url) const;
    QString generateDeviceId() const;
};

#endif // APICLIENT_H
