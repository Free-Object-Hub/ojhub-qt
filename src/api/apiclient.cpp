#include "apiclient.h"
#include "../lang.h"
#include "../altcha/altcha.h"
#include "../version.h"
#include "../settings.h"
#include "../core/protocol.h"
#include "../domain/account/accountdomain.h"
#include "../domain/catalog/catalogdomain.h"
#include "../domain/catalog/catalogparse.h"
#include "../domain/detail/detaildomain.h"
#include "../domain/profile/profiledomain.h"
#include "../domain/social/socialdomain.h"
#include "../domain/projects/projectsdomain.h"
#include "../domain/vacancies/vacanciesdomain.h"
#include "../domain/news/newsdomain.h"
#include "../domain/portfolio/portfoliodomain.h"

#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>
#include <QDateTime>
#include <QCryptographicHash>
#include <QHttpMultiPart>
#include <QNetworkAccessManager>
#include <QEventLoop>


// --- ALTCHA v1: fetch challenge + pure-C solve ---



#include "../core/session.h"
#include "../core/network.h"
#include "../domain/authoring/formparse.h"
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_session(new Session(this))
    , m_network(new Network(m_session, this))
    , m_account(new AccountDomain(m_session, m_network, this))
    , m_catalog(new CatalogDomain(m_session, m_network, this))
    , m_detail(new DetailDomain(m_session, m_network, this))
    , m_profile(new ProfileDomain(m_session, m_network, this))
    , m_social(new SocialDomain(m_session, m_network, this))
    , m_projects(new ProjectsDomain(m_session, m_network, this))
    , m_vacancies(new VacanciesDomain(m_session, m_network, this))
    , m_news(new NewsDomain(m_session, m_network, this))
    , m_portfolio(new PortfolioDomain(m_session, m_network, this))
{
    connect(m_account, SIGNAL(loginResult(bool,QString,QByteArray)),
            this, SLOT(onAccountLogin(bool,QString,QByteArray)));
    connect(m_account, SIGNAL(registerResult(bool,QString,QByteArray)),
            this, SLOT(onAccountRegister(bool,QString,QByteArray)));
    // The catalog reports through ApiClient's own signals, so the UI connects to ApiClient only.
    connect(m_catalog, SIGNAL(searchFinished(SearchResultList,bool,int)),
            this, SIGNAL(searchFinished(SearchResultList,bool,int)));
    connect(m_catalog, SIGNAL(searchError(QString)), this, SIGNAL(searchError(QString)));
    connect(m_catalog, SIGNAL(networkError(QString)), this, SIGNAL(networkError(QString)));
    connect(m_detail, SIGNAL(detailFinished(DetailData)), this, SIGNAL(detailFinished(DetailData)));
    connect(m_detail, SIGNAL(detailError(QString)), this, SIGNAL(detailError(QString)));
    connect(m_profile, SIGNAL(profileFinished(PublicProfile)), this, SIGNAL(profileFinished(PublicProfile)));
    connect(m_profile, SIGNAL(profileError(QString)), this, SIGNAL(profileError(QString)));
    connect(m_social, SIGNAL(voteAnswered(int,int,bool,QByteArray)),
            this, SLOT(onVoteAnswer(int,int,bool,QByteArray)));
    connect(m_social, SIGNAL(commentsAnswered(int,int,QByteArray)),
            this, SLOT(onCommentsAnswer(int,int,QByteArray)));
    connect(m_social, SIGNAL(subscriptionAnswered(int,bool,QByteArray)),
            this, SLOT(onSubscriptionAnswer(int,bool,QByteArray)));
    connect(m_social, SIGNAL(commentEdited(int,QString)), this, SIGNAL(commentEdited(int,QString)));
    connect(m_social, SIGNAL(commentDeleted(int)), this, SIGNAL(commentDeleted(int)));
    connect(m_social, SIGNAL(profileFieldSaved(int,QString)), this, SIGNAL(profileFieldSaved(int,QString)));
    connect(m_social, SIGNAL(subscriptionsLoaded(QVariantList,int,bool,int)),
            this, SIGNAL(subscriptionsLoaded(QVariantList,int,bool,int)));
    connect(m_social, SIGNAL(actionFinished(int,bool,QString,int)), this, SIGNAL(actionFinished(int,bool,QString,int)));
    connect(m_social, SIGNAL(alarmsLoaded(QVariantList,int,bool)), this, SIGNAL(alarmsLoaded(QVariantList,int,bool)));
    connect(m_social, SIGNAL(alarmLoaded(QVariantMap)), this, SIGNAL(alarmLoaded(QVariantMap)));
    connect(m_social, SIGNAL(alarmDeleted(int)), this, SIGNAL(alarmDeleted(int)));
    connect(m_account, SIGNAL(accountChanged()), this, SIGNAL(accountChanged()));
    connect(m_account, SIGNAL(sessionExpired()), this, SLOT(onSessionExpired()));
    connect(m_projects, SIGNAL(projectFormLoaded(ProjectForm)), this, SIGNAL(projectFormLoaded(ProjectForm)));
    connect(m_projects, SIGNAL(accountRefreshRequested()), this, SLOT(onAccountRefreshRequested()));
    connect(m_projects, SIGNAL(actionFinished(int,bool,QString,int)), this, SIGNAL(actionFinished(int,bool,QString,int)));
    connect(m_vacancies, SIGNAL(vacancyFormLoaded(VacancyForm)), this, SIGNAL(vacancyFormLoaded(VacancyForm)));
    connect(m_vacancies, SIGNAL(vacancyOwnership(int,bool)), this, SIGNAL(vacancyOwnership(int,bool)));
    connect(m_vacancies, SIGNAL(actionFinished(int,bool,QString,int)), this, SIGNAL(actionFinished(int,bool,QString,int)));
    connect(m_news, SIGNAL(newsOwnership(int,bool)), this, SIGNAL(newsOwnership(int,bool)));
    connect(m_news, SIGNAL(actionFinished(int,bool,QString,int)), this, SIGNAL(actionFinished(int,bool,QString,int)));
    connect(m_portfolio, SIGNAL(actionFinished(int,bool,QString,int)), this, SIGNAL(actionFinished(int,bool,QString,int)));
}

Session *ApiClient::session() const
{
    return m_session;
}

Network *ApiClient::network() const
{
    return m_network;
}

void ApiClient::setBaseUrl(const QString &url)
{
    QString u = url;
    if (u.endsWith(QLatin1Char('/')))
        u.chop(1);
    m_session->setBaseUrl(u);
}

QString ApiClient::baseUrl() const
{
    return m_session->baseUrl();
}

void ApiClient::setUserToken(const QString &token)
{
    m_session->setUserToken(token);
}

void ApiClient::setDeviceToken(const QString &token)
{
    m_session->setDeviceToken(token);
}

QString ApiClient::userToken() const
{
    return m_session->userToken();
}

QString ApiClient::deviceToken() const
{
    return m_session->deviceToken();
}

bool ApiClient::isLoggedIn() const
{
    return m_session->isLoggedIn();
}

QString ApiClient::generateDeviceId() const
{
    return AppSettings::deviceId();
}

QString ApiClient::ensureDeviceToken()
{
    const QString id = m_session->ensureDeviceToken();
    return id;
}

QNetworkRequest ApiClient::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

// -------------------- Item parsing helpers --------------------








// -------------------- Search --------------------





// -------------------- News / detail --------------------












// -------------------- Search / news / vacancies (delegated to CatalogDomain) --------------------

void ApiClient::search(const QString &name, int channel, int method, int page,
                       const QList<int> &tags, const QList<int> &os)
{
    m_catalog->search(name, channel, method, page, tags, os);
}

void ApiClient::fetchNews(int page)
{
    m_catalog->fetchNews(page);
}

void ApiClient::fetchVacancies(int page)
{
    m_catalog->fetchVacancies(page);
}

// -------------------- Detail and comments (delegated to DetailDomain) --------------------

void ApiClient::fetchDetail(int kind, int id, int page, int extra)
{
    m_detail->fetchDetail(kind, id, page, extra);
}

void ApiClient::fetchComments(int contentType, int id, int page)
{
    m_detail->fetchComments(contentType, id, page);
}

SearchResultList ApiClient::commentsFromAnswer(const QByteArray &data, bool *hasMore) const
{
    return m_detail->commentsFromAnswer(data, hasMore);
}

// -------------------- Profile (delegated to ProfileDomain) --------------------

void ApiClient::fetchProfile(int userId)
{
    m_profile->fetchProfile(userId);
}

// -------------------- Session slots --------------------

void ApiClient::onSessionExpired()
{
    logout();
}

void ApiClient::onAccountRefreshRequested()
{
    m_account->loadAccount();
}

// -------------------- Social writes (delegated to SocialDomain) --------------------

// -------------------- Login / Register / Logout (delegated to AccountDomain) --------------------

void ApiClient::login(const QString &username, const QString &password)
{
    m_account->login(username, password);
}

void ApiClient::registerUser(const QString &username, const QString &password, const QString &email)
{
    m_account->registerUser(username, password, email);
}

void ApiClient::onAccountLogin(bool ok, const QString &message, const QByteArray &accountData)
{
    if (!ok) {
        emit loginFinished(false, message);
        return;
    }
    applyAccountData(accountData);
    fetchLikesAndSubs();
    emit loginFinished(true, message);
}

void ApiClient::onAccountRegister(bool ok, const QString &message, const QByteArray &accountData)
{
    if (!ok) {
        emit registerFinished(false, message);
        return;
    }
    applyAccountData(accountData);
    fetchLikesAndSubs();
    emit registerFinished(true, message);
}

// Explicit startup step (called by MainWindow after the base URL is final): restores the
// login saved in ObjectHub.ini and reloads owned objects and likes.
void ApiClient::restoreSession()
{
    if (!m_account->restore())
        return;
    loadAccount();
    fetchLikesAndSubs();
}

void ApiClient::logout()
{
    m_account->logout();
    emit logoutFinished();
}

// -------------------- Profile --------------------

int ApiClient::userId() const
{
    return m_session->userId();
}


// ---------- forms (GET) ----------



// ---------- saves (multipart: vacsEdit needs it, the rest accept it) ----------



// ---------- vacancy actions (GET) ----------




// ---------- one handler for every write and form load ----------


// ---------- news ----------



// ---------- likes ----------


// ---------- comments ----------






// ---------- profile fields ----------




// ---------- subscriptions ----------



// ---------- notifications ----------








// ---------- portfolio ----------




// ---------- account state ----------








// ---------- answers that change account state (likes, subscriptions, comments list) ----------

void ApiClient::onVoteAnswer(int typeCode, int id, bool dislike, const QByteArray &data)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray() || doc.array().size() < 2) {
        emit actionFinished(ActVote, false, errorMessage(data), id);
        return;
    }
    // openGo: any existing vote is removed; otherwise the new vote is set.
    // No one-click switch from like to dislike.
    m_account->recordVote(typeCode, id, dislike);
    emit votesChanged(typeCode, id, doc.array().at(0).toInt(), doc.array().at(1).toInt());
}

void ApiClient::onCommentsAnswer(int contentType, int id, const QByteArray &data)
{
    bool more = false;
    const SearchResultList list = commentsFromAnswer(data, &more);
    emit commentsUpdated(contentType, id, list, more);
}

void ApiClient::onSubscriptionAnswer(int projectId, bool subscribe, const QByteArray &data)
{
    if (data == "1" || data == "-1")
        m_account->recordSubscription(projectId, subscribe);
    if (data == "1") {
        emit subscriptionChanged(projectId, subscribe);
        return;
    }
    QString msg;
    if (data == "-1")
        msg = subscribe ? QString::fromUtf8("You are already subscribed")       // LANGXXX
                        : QString::fromUtf8("You are not subscribed");        // LANGXXX
    else
        msg = errorMessage(data);
    emit actionFinished(subscribe ? ActSub : ActUnsub, false, msg, projectId);
}

// ---------- forwarders to the social domain ----------

void ApiClient::vote(int typeCode, int id, bool dislike)
{
    m_social->vote(typeCode, id, dislike);
}

void ApiClient::postComment(int contentType, int id, const QString &text)
{
    m_social->postComment(contentType, id, text);
}

void ApiClient::editComment(int commentId, const QString &text)
{
    m_social->editComment(commentId, text);
}

void ApiClient::deleteComment(int commentId, int contentType)
{
    m_social->deleteComment(commentId, contentType);
}

void ApiClient::setNickname(const QString &nick)
{
    m_social->setNickname(nick);
}

void ApiClient::setResume(const QString &text)
{
    m_social->setResume(text);
}

void ApiClient::setSocials(const QString &text)
{
    m_social->setSocials(text);
}

void ApiClient::subscribe(int projectId, bool unsubscribe)
{
    m_social->subscribe(projectId, unsubscribe);
}

void ApiClient::fetchSubscriptions(int page, int probeId)
{
    m_social->fetchSubscriptions(page, probeId);
}

// ---------- forwarders: authoring, account data, notifications ----------

void ApiClient::fetchProjectForm(int channel, int id)
{
    m_projects->fetchProjectForm(channel, id);
}

void ApiClient::saveProject(const ProjectForm &f, const QString &imgFile, const QString &banFile)
{
    m_projects->saveProject(f, imgFile, banFile);
}

void ApiClient::fetchVacancyForm(int id)
{
    m_vacancies->fetchVacancyForm(id);
}

void ApiClient::saveVacancy(const VacancyForm &f)
{
    m_vacancies->saveVacancy(f);
}

void ApiClient::removeVacancy(int id)
{
    m_vacancies->removeVacancy(id);
}

void ApiClient::applyVacancy(int id)
{
    m_vacancies->applyVacancy(id);
}

void ApiClient::unapplyVacancy(int applyId)
{
    m_vacancies->unapplyVacancy(applyId);
}

void ApiClient::probeVacancy(int id)
{
    m_vacancies->probeVacancy(id);
}

void ApiClient::saveNews(const NewsForm &f, const QString &filePath)
{
    m_news->saveNews(f, filePath);
}

void ApiClient::deleteNews(int id)
{
    m_news->deleteNews(id);
}

void ApiClient::probeNews(int id, int projectId)
{
    m_news->probeNews(id, projectId);
}

void ApiClient::saveWork(const WorkForm &f)
{
    m_portfolio->saveWork(f);
}

void ApiClient::deleteWork(int id)
{
    m_portfolio->deleteWork(id);
}

void ApiClient::fetchAlarms(int page)
{
    m_social->fetchAlarms(page);
}

void ApiClient::fetchAlarm(int id)
{
    m_social->fetchAlarm(id);
}

void ApiClient::deleteAlarm(int id)
{
    m_social->deleteAlarm(id);
}

bool ApiClient::accountLoaded() const
{
    return m_account->accountLoaded();
}

bool ApiClient::ownsProject(int projectId) const
{
    return m_account->ownsProject(projectId);
}

bool ApiClient::isSubscribed(int projectId) const
{
    return m_account->isSubscribed(projectId);
}

int ApiClient::likeState(int typeCode, int id) const
{
    return m_account->likeState(typeCode, id);
}

void ApiClient::applyAccountData(const QByteArray &data)
{
    m_account->applyAccountData(data);
}

void ApiClient::loadAccount()
{
    m_account->loadAccount();
}

void ApiClient::fetchLikesAndSubs()
{
    m_account->fetchLikesAndSubs();
}

