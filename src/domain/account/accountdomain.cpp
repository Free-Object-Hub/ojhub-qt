#include "accountdomain.h"
#include "../../lang.h"
#include "../../settings.h"
#include "../../core/session.h"
#include "../../core/network.h"
#include "../../core/protocol.h"
#include "../../api/forms.h"

#include <QUrlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

AccountDomain::AccountDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
    , m_accountLoaded(false)
{
}

QNetworkRequest AccountDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

bool AccountDomain::restore()
{
    QString token;
    int userId = 0;
    AppSettings::loadLogin(token, userId);
    if (token.isEmpty() || userId <= 0)
        return false;
    m_session->setUserToken(token);
    m_session->setUserId(userId);
    return true;
}

static int userIdFromLogin(const QByteArray &data)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray() || doc.array().isEmpty() || !doc.array().at(0).isObject())
        return 0;
    return doc.array().at(0).toObject().value(QStringLiteral("ID")).toInt();
}

// -------------------- ALTCHA (async) --------------------

void AccountDomain::fetchAltcha(const std::function<void(const QString &, const QString &)> &done)
{
    QUrl chUrl(m_session->baseUrl() + QString::fromLatin1("/server/133/challenge.php"));
    logNetReq("CHALLENGE", QString::fromLatin1("GET"), chUrl, QByteArray());
    QNetworkReply *reply = m_nam->get(makeRequest(chUrl));
    // The helper is parented to the reply, so it goes away together with it.
    new AltchaFetch(reply, done);
}

void AccountDomain::login(const QString &username, const QString &password)
{
    m_session->ensureDeviceToken();
    fetchAltcha([this, username, password](const QString &payload, const QString &err) {
        if (!err.isEmpty()) {
            emit loginResult(false, QString::fromUtf8("Could not get ALTCHA: ") + err, QByteArray());
            return;
        }
        // 2) login POST
        QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/user/login.php"));
        QNetworkRequest request = makeRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          QString::fromLatin1("application/x-www-form-urlencoded"));

        QUrlQuery body;
        body.addQueryItem(QString::fromLatin1("username"), username);
        body.addQueryItem(QString::fromLatin1("password"), password);
        body.addQueryItem(QString::fromLatin1("device"), m_session->deviceToken());
        body.addQueryItem(QString::fromLatin1("deviceDynamic"), m_session->deviceToken());
        body.addQueryItem(QString::fromLatin1("altcha"),
                          payload);

        const QByteArray postBody = body.query(QUrl::FullyEncoded).toUtf8();
        logNetReq("LOGIN", QString::fromLatin1("POST"), url, postBody);
        QNetworkReply *reply = m_nam->post(request, postBody);
        connect(reply, SIGNAL(finished()), this, SLOT(onLoginReplyFinished()));
    });
}

void AccountDomain::onLoginReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    const QByteArray data = reply->readAll().trimmed();
    logNetResp("LOGIN", reply, data);

    if (reply->error() != QNetworkReply::NoError) {
        /* openGo: http.Error → text body, e.g. "Login error: ..." */
        emit loginResult(false, reply->errorString() + QString::fromLatin1(" | ") + QString::fromUtf8(data), QByteArray());
        return;
    }

    /*
     * openGo zUserLogin.go:
     *   -1 wrong password
     *   -2 user not found
     *   -3 bad ALTCHA
     * success: InitOjhub → JSON array [private, public]
     *   private ≈ userObject (with "token" when showToken) , [gdps, wiki]
     *   see zLoginT.go InitOjhub / injectToken2
     */
    if (data == "-1") {
        emit loginResult(false, L("wrongPass"), QByteArray());
        return;
    }
    if (data == "-2") {
        emit loginResult(false, L("accountEmpty"), QByteArray());
        return;
    }
    if (data == "-3") {
        emit loginResult(false, L("captchaDed"), QByteArray());
        return;
    }
    if (data == "Access denied") {
        emit loginResult(false, QString::fromLatin1("Access denied"), QByteArray());
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning().noquote() << "[NET] LOGIN JSON parse error:" << err.errorString()
                             << "at" << err.offset;
        emit loginResult(false, QString::fromUtf8(
            "Unexpected server response (not JSON). See [NET] body in the console"), QByteArray());
        return;
    }

    QString token;

    /* Canonical V133 loginT shape: [ privateUserOrFragment, publicCache ] */
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        qDebug() << "[NET] LOGIN array size" << arr.size();
        if (arr.size() > 0 && arr.at(0).isObject()) {
            QJsonObject userObj = arr.at(0).toObject();
            token = userObj.value(QString::fromLatin1("token")).toString();
            qDebug() << "[NET] LOGIN user keys" << userObj.keys();
        }
        if (token.isEmpty())
            token = findTokenRecursive(QJsonValue(arr), 0);
    } else if (doc.isObject()) {
        token = findTokenRecursive(QJsonValue(doc.object()), 0);
    }

    if (token.isEmpty() || token == QString::fromLatin1("false")) {
        qWarning() << "[NET] LOGIN token missing or guest placeholder";
        emit loginResult(false, QString::fromUtf8(
            "InitOjhub JSON is present, but the token is empty (see [NET] body)"), QByteArray());
        return;
    }

    qDebug().noquote() << "[NET] LOGIN token len=" << token.size();
    const int uid = userIdFromLogin(data);
    m_session->setUserToken(token);
    m_session->setUserId(uid);
    AppSettings::saveLogin(token, uid);
    emit loginResult(true, L("loggedAs"), data);
}

void AccountDomain::registerUser(const QString &username, const QString &password, const QString &email)
{
    m_session->ensureDeviceToken();
    fetchAltcha([this, username, password, email](const QString &payload, const QString &err) {
        if (!err.isEmpty()) {
            emit registerResult(false, QString::fromUtf8("Could not get ALTCHA: ") + err, QByteArray());
            return;
        }
        QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/user/register.php"));
        QNetworkRequest request = makeRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          QString::fromLatin1("application/x-www-form-urlencoded"));

        QUrlQuery body;
        body.addQueryItem(QString::fromLatin1("username"), username);
        body.addQueryItem(QString::fromLatin1("password"), password);
        body.addQueryItem(QString::fromLatin1("email"), email);
        body.addQueryItem(QString::fromLatin1("device"), m_session->deviceToken());
        body.addQueryItem(QString::fromLatin1("deviceDynamic"), m_session->deviceToken());
        body.addQueryItem(QString::fromLatin1("altcha"),
                          payload);

        const QByteArray postBody = body.query(QUrl::FullyEncoded).toUtf8();
        logNetReq("REGISTER", QString::fromLatin1("POST"), url, postBody);
        QNetworkReply *reply = m_nam->post(request, postBody);
        connect(reply, SIGNAL(finished()), this, SLOT(onRegisterReplyFinished()));
    });
}

void AccountDomain::onRegisterReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    const QByteArray data = reply->readAll().trimmed();
    logNetResp("REGISTER", reply, data);

    if (reply->error() != QNetworkReply::NoError) {
        emit registerResult(false, reply->errorString(), QByteArray());
        return;
    }

    /*
     * openGo userRegister:
     *   -2 bad ALTCHA
     *   -4 empty fields / taken / invalid email / internal gen fail
     * success: same InitOjhub array as login
     */
    if (data == "-2") {
        emit registerResult(false, L("captchaDed"), QByteArray());
        return;
    }
    if (data == "-4") {
        emit registerResult(false, L("loginClaimed"), QByteArray());
        return;
    }
    if (data == "Access denied") {
        emit registerResult(false, QString::fromLatin1("Access denied"), QByteArray());
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        emit registerResult(false, QString::fromUtf8(
            "Unexpected response (not JSON). See [NET] body"), QByteArray());
        return;
    }

    QString token;
    if (doc.isArray() && doc.array().size() > 0 && doc.array().at(0).isObject())
        token = doc.array().at(0).toObject().value(QString::fromLatin1("token")).toString();
    if (token.isEmpty())
        token = findTokenRecursive(doc.isArray() ? QJsonValue(doc.array())
                                                 : QJsonValue(doc.object()), 0);

    if (token.isEmpty() || token == QString::fromLatin1("false")) {
        emit registerResult(false, QString::fromUtf8(
            "Registration answered without a token (see [NET] body)"), QByteArray());
        return;
    }

    const int uid = userIdFromLogin(data);
    m_session->setUserToken(token);
    m_session->setUserId(uid);
    AppSettings::saveLogin(token, uid);
    emit registerResult(true, L("loggedAs"), data);
}

void AccountDomain::logout()
{
    AppSettings::clearLogin();
    m_session->clearUser();
    m_owned.clear();
    m_subs.clear();
    m_likes.clear();
    m_accountLoaded = false;
}

// ---------- account state: owned projects, likes, subscriptions ----------

static QString likeCategoryOf(int typeCode)
{
    switch (typeCode) {
    case 0:  return QStringLiteral("p");
    case 2:  return QStringLiteral("n");
    case 11: return QStringLiteral("v");
    case 3:
    case 5:
    case 12: return QStringLiteral("c");
    default: return QString();
    }
}

void AccountDomain::recordVote(int typeCode, int id, bool dislike)
{
    // openGo: any existing vote is removed; otherwise the new vote is set.
    // No one-click switch from like to dislike.
    const QString cat = likeCategoryOf(typeCode);
    if (cat.isEmpty())
        return;
    QHash<int, int> &h = m_likes[cat];
    if (h.contains(id))
        h.remove(id);
    else
        h.insert(id, dislike ? -1 : 1);
}

void AccountDomain::recordSubscription(int projectId, bool subscribe)
{
    if (subscribe)
        m_subs.insert(projectId);
    else
        m_subs.remove(projectId);
}

bool AccountDomain::accountLoaded() const
{
    return m_accountLoaded;
}

bool AccountDomain::ownsProject(int projectId) const
{
    return projectId > 0 && m_owned.contains(projectId);
}

bool AccountDomain::isSubscribed(int projectId) const
{
    return m_subs.contains(projectId);
}

int AccountDomain::likeState(int typeCode, int id) const
{
    const QString cat = likeCategoryOf(typeCode);
    return cat.isEmpty() ? 0 : m_likes.value(cat).value(id, 0);
}

// loginT / login answer: [user, [projectsMap, wikisMap], ...]. Projects are keyed "c12", "s34", ...
void AccountDomain::applyAccountData(const QByteArray &data)
{
    m_owned.clear();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return;
    const QJsonArray a = doc.array();
    if (a.size() < 2 || !a.at(1).isArray())
        return;
    const QJsonArray parts = a.at(1).toArray();
    if (parts.isEmpty() || !parts.at(0).isObject())
        return;
    const QJsonObject projects = parts.at(0).toObject();
    const QStringList keys = projects.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const int pid = projects.value(keys.at(i)).toObject().value(QStringLiteral("ID")).toInt();
        if (pid > 0)
            m_owned.insert(pid);
    }
}

void AccountDomain::loadAccount()
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/loginT.php"));
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActLoadAccount, 0, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onAccountReplyFinished()));
}

void AccountDomain::fetchLikesAndSubs()
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/likesT.php"));
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActLoadLikes, 0, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onAccountReplyFinished()));
}

void AccountDomain::onAccountReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    const int action = reply->property("action").toInt();
    const int id = reply->property("id").toInt();
    const QByteArray data = reply->readAll().trimmed();
    qDebug().noquote() << "[NET] WRITE action" << action << "id" << id
                       << "HTTP" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                       << QString::fromUtf8(data.left(300));

    if (reply->error() != QNetworkReply::NoError) {
        emit actionFinished(action, false, reply->errorString(), id);
        return;
    }

    switch (action) {
    case ActLoadLikes: {
        if (data == QByteArray("Access denied")) {
            // Saved token is no longer valid: drop the login instead of pretending we are in.
            emit sessionExpired();
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject())
            return;   // not logged in, or an error: stays unloaded
        const QJsonObject o = doc.object();
        m_likes.clear();
        m_subs.clear();
        const QStringList keys = o.keys();
        for (int k = 0; k < keys.size(); ++k) {
            const QString key = keys.at(k);
            const QJsonValue v = o.value(key);
            if (!v.isArray())
                continue;
            const QJsonArray arr = v.toArray();
            if (key == QLatin1String("subs")) {
                for (int i = 0; i < arr.size(); ++i)
                    m_subs.insert(arr.at(i).toInt());
                continue;
            }
            QHash<int, int> &cat = m_likes[key];
            for (int i = 0; i < arr.size(); ++i) {
                const int raw = arr.at(i).toInt();
                if (raw != 0)
                    cat.insert(qAbs(raw), raw < 0 ? 1 : -1);   // server: -id like, +id dislike
            }
        }
        m_accountLoaded = true;
        emit accountChanged();
        return;
    }
    case ActLoadAccount: {
        if (QJsonDocument::fromJson(data).isArray()) {
            applyAccountData(data);
            emit accountChanged();
        }
        return;
    }
    default:
        return;
    }
}

// ---------- ALTCHA reply handling ----------

AltchaFetch::AltchaFetch(QNetworkReply *reply, const std::function<void(const QString &, const QString &)> &done)
    : QObject(reply)
    , m_done(done)
{
    connect(reply, SIGNAL(finished()), this, SLOT(onFinished()));
}

void AltchaFetch::onFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(parent());
    if (!reply)
        return;
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        m_done(QString(), reply->errorString());
        return;
    }
    const QByteArray chJson = reply->readAll();
    logNetResp("CHALLENGE", reply, chJson);

    char altcha_b64[1024];
    const int bl = solveAltchaFromJson(chJson, altcha_b64, (int)sizeof(altcha_b64));
    if (bl < 0) {
        m_done(QString(), QString::fromUtf8("Could not solve ALTCHA (code %1)").arg(bl));
        return;
    }
    m_done(QString::fromLatin1(altcha_b64), QString());
}
