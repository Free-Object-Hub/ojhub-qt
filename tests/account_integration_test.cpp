// Integration test for the account domain and ApiClient login flow.
// Not part of the .pro build: it links the app objects and starts a fake HTTP server on 127.0.0.1:45987.
// Build: moc account_integration_test.cpp -o test.moc; g++ -std=gnu++11 -fPIC <Qt5 includes> account_integration_test.cpp <app .o files except main.o> -lQt5Core -lQt5Network -lQt5Gui -lQt5Widgets -lGL -lpthread
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QPointer>
#include <cstdio>
#include "api/apiclient.h"
#include "tagnames.h"
#include "api/searchresult.h"
#include "core/session.h"
#include "core/network.h"
#include "domain/account/accountdomain.h"
#include "settings.h"

static QByteArray readIni() {
    QFile f(AppSettings::iniPath());
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();
    return f.readAll();
}

static int fails = 0;
#define CHECK(cond, msg) do { if (cond) printf("  ok   %s\n", msg); else { printf("  FAIL %s\n", msg); fails++; } } while (0)

// ---- fake server ----
static const QByteArray kSalt = "saltyX";
static QByteArray lastSearchPath;
static QByteArray challengeHex() {
    return QCryptographicHash::hash(kSalt + QByteArray("7"), QCryptographicHash::Sha256).toHex();
}
static QByteArray respond(const QByteArray &method, const QByteArray &path, const QByteArray &headers, const QByteArray &body) {
    if (path.startsWith("/server/133/challenge.php"))
        return "{\"algorithm\":\"SHA-256\",\"challenge\":\"" + challengeHex() + "\",\"salt\":\"" + kSalt +
               "\",\"signature\":\"sig\",\"maxNumber\":100}";
    if (path.startsWith("/server/133/search/new.php")) {
        lastSearchPath = path;
        if (path.contains("name=order"))
            return "{\"p7\":{\"ID\":7,\"title\":\"P7\"},\"s3\":{\"ID\":3,\"title\":\"S3\"},\"c12\":{\"ID\":12,\"title\":\"C12\"},\"t1\":{\"ID\":1,\"title\":\"T1\"}}";
        if (path.contains("name=desc")) {
            QByteArray o = "{";
            for (int i = 9; i >= 1; --i) { if (i < 9) o += ","; o += "\"s" + QByteArray::number(i) + "\":{\"ID\":" + QByteArray::number(i) + ",\"title\":\"D" + QByteArray::number(i) + "\"}"; }
            return o + "}";
        }
        if (path.contains("name=denied")) return "Access denied";
        if (path.contains("name=os")) return "{\"c12\":{\"ID\":12,\"title\":\"O\",\"tags\":[1,2],\"os\":[12,2]}}";
        if (path.contains("name=slow")) return "{\"c1\":{\"ID\":1,\"title\":\"Slow\"}}";
        if (path.contains("name=many")) {
            QByteArray o = "{";
            for (int i = 1; i <= 9; ++i) { if (i > 1) o += ","; o += "\"c" + QByteArray::number(i) + "\":{\"ID\":" + QByteArray::number(i) + ",\"title\":\"T" + QByteArray::number(i) + "\"}"; }
            return o + "}";
        }
        return "{\"c12\":{\"ID\":12,\"title\":\"Alpha\",\"short\":\"s1\",\"tags\":\"Retro; Racing\"},"
               "\"s3\":{\"ID\":3,\"title\":\"Beta\",\"short\":\"s2\"}}";
    }
    if (path.startsWith("/server/133/content/newsAll.php")) {
        QByteArray o = "[";
        for (int i = 1; i <= 11; ++i) { if (i > 1) o += ","; o += "{\"ID\":" + QByteArray::number(i) + ",\"title\":\"N" + QByteArray::number(i) + "\"}"; }
        return o + "]";
    }
    if (path.startsWith("/server/133/content/getUser.php")) {
        if (path.contains("id=404")) return "[\"NONE\"]";
        return "{\"ID\":5,\"username\":\"neo\",\"nickname\":\"Neo\",\"resume\":\"hi\",\"socials\":\"x\"}";
    }
    if (path.startsWith("/server/133/content/fetchComms.php")) {
        QByteArray o = "[";
        for (int i = 1; i <= 11; ++i) { if (i > 1) o += ","; o += "{\"ID\":" + QByteArray::number(i) + ",\"text\":\"c" + QByteArray::number(i) + "\",\"username\":\"u\"}"; }
        return o + "]";
    }
    if (path.startsWith("/server/133/send/like.php") || path.startsWith("/server/133/send/dislike.php"))
        return "[5,2]";
    if (path.startsWith("/server/133/send/comment.php")) {
        return "[{\"ID\":1,\"text\":\"hello\",\"username\":\"u\"},{\"ID\":2,\"text\":\"bye\",\"username\":\"u\"}]";
    }
    if (path.startsWith("/server/133/send/commentModify.php")) return "edited text";
    if (path.startsWith("/server/133/delete/comment.php")) return "7";
    if (path.startsWith("/server/133/user/setNickname.php")) return "NewNick";
    if (path.startsWith("/server/133/gdps/sub.php") || path.startsWith("/server/133/gdps/unsub.php")) return "1";
    if (path.startsWith("/server/133/gdps/subs.php"))
        return "[{\"gdpsNum\":1,\"gdpsTitle\":\"P1\"},{\"gdpsNum\":2,\"gdpsTitle\":\"P2\"}]";
    if (path.startsWith("/server/133/vacans/edit.php")) return "{\"ID\":3}";
    if (path.startsWith("/server/133/content/getAlarms.php")) return "[[1,\"A\",1],[2,\"B\",2]]";
    if (path.startsWith("/server/133/send/deleteAlarm.php")) return "{\"ID\":4}";
    if (path.startsWith("/server/133/user/workDelete.php")) return "6";
    if (path.startsWith("/server/133/vacans/getAll.php"))
        return "[[1,\"V1\",\"d1\"],[2,\"V2\",\"d2\"],[3,\"V3\",\"d3\"]]";
    if (path.startsWith("/server/133/user/register.php")) {
        if (!body.contains("altcha=")) return "-3";
        if (body.contains("username=taken")) return "-4";
        return "[{\"ID\":9,\"token\":\"TOK123\"},[]]";
    }
    if (path.startsWith("/server/133/user/login.php")) {
        if (!body.contains("altcha=")) return "-3";
        if (body.contains("password=good")) return "[{\"ID\":5,\"token\":\"TOK123\"},[]]";
        return "-1";
    }
    bool tokOk = headers.contains("user-token: TOK123") || headers.contains("User-Token: TOK123");
    if (path.startsWith("/server/133/loginT.php")) return tokOk ? "[{\"ID\":5}, [{\"c12\":{\"ID\":12}}, {}]]" : "Access denied";
    if (path.startsWith("/server/133/likesT.php")) return tokOk ? "{\"subs\":[]}" : "Access denied";
    return "";
}

class FakeServer : public QObject {
    Q_OBJECT
public:
    QTcpServer srv;
    FakeServer() { srv.listen(QHostAddress::LocalHost, 45987); connect(&srv, SIGNAL(newConnection()), this, SLOT(onNew())); }
private slots:
    void onNew() {
        QTcpSocket *s = srv.nextPendingConnection();
        connect(s, SIGNAL(readyRead()), this, SLOT(onRead()));
        connect(s, SIGNAL(disconnected()), s, SLOT(deleteLater()));
    }
    void onRead() {
        QTcpSocket *s = qobject_cast<QTcpSocket*>(sender());
        QByteArray &buf = bufs[s];
        buf += s->readAll();
        int hend = buf.indexOf("\r\n\r\n");
        if (hend < 0) return;
        QByteArray head = buf.left(hend);
        int clen = 0;
        int ci = head.toLower().indexOf("content-length:");
        if (ci >= 0) clen = head.mid(ci + 15, head.indexOf("\r\n", ci) - ci - 15).trimmed().toInt();
        if (buf.size() < hend + 4 + clen) return;
        QByteArray body = buf.mid(hend + 4, clen);
        QByteArray method = head.left(head.indexOf(' '));
        QByteArray path = head.mid(method.size() + 1, head.indexOf(' ', method.size() + 1) - method.size() - 1);
        QByteArray out = respond(method, path, head, body);
        const QByteArray httpReply = "HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(out.size()) + "\r\nConnection: close\r\n\r\n" + out;
        bufs.remove(s);
        if (path.contains("name=slow")) {
            QPointer<QTcpSocket> sp(s);
            QTimer::singleShot(800, this, [sp, httpReply]() {
                if (!sp) return;
                sp->write(httpReply); sp->flush(); sp->disconnectFromHost();
            });
            return;
        }
        s->write(httpReply);
        s->flush(); s->disconnectFromHost();
    }
private:
    QHash<QTcpSocket*, QByteArray> bufs;
};

// Runs the loop until the named signal fires or the timeout passes.
static void waitFor(QObject *sender, const char *sig, int ms = 5000) {
    QEventLoop loop;
    QObject::connect(sender, sig, &loop, SLOT(quit()));
    QTimer::singleShot(ms, &loop, SLOT(quit()));
    loop.exec();
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QFile::remove(AppSettings::iniPath());
    FakeServer server;
    const QString base = "http://127.0.0.1:45987";

    printf("== 1. login with good password, token saved\n");
    {
        ApiClient c; c.setBaseUrl(base);
        bool ok = false;
        QObject::connect(&c, &ApiClient::loginFinished, [&](bool o, const QString &){ ok = o; });
        c.login("u", "good");
        waitFor(&c, SIGNAL(loginFinished(bool,QString)));
        CHECK(ok, "login succeeds");
        CHECK(c.isLoggedIn(), "isLoggedIn after login");
        CHECK(c.userId() == 5, "userId parsed from answer");
        QByteArray ini = readIni();
        CHECK(ini.contains("token=TOK123"), "token written to ObjectHub.ini");
        CHECK(ini.contains("userId=5"), "userId written to ObjectHub.ini");
        QEventLoop settle; QTimer::singleShot(300, &settle, SLOT(quit())); settle.exec();
    }

    printf("== 2. wrong password with no saved login\n");
    {
        AppSettings::clearLogin();
        ApiClient c; c.setBaseUrl(base);
        bool ok = true;
        QObject::connect(&c, &ApiClient::loginFinished, [&](bool o, const QString &){ ok = o; });
        c.login("u", "bad");
        waitFor(&c, SIGNAL(loginFinished(bool,QString)));
        CHECK(!ok, "wrong password reported as failure");
        CHECK(!c.isLoggedIn(), "not logged in after failed login");
        CHECK(readIni().indexOf("TOK123") < 0, "failed login saves nothing");
    }

    printf("== 3. restart: login is restored from ini and account data reloads\n");
    {
        ApiClient first; first.setBaseUrl(base);
        first.login("u", "good");
        waitFor(&first, SIGNAL(loginFinished(bool,QString)));
        QEventLoop settle; QTimer::singleShot(300, &settle, SLOT(quit())); settle.exec();
    }
    {
        ApiClient c; c.setBaseUrl(base);
        c.restoreSession();
        CHECK(c.isLoggedIn(), "restored session is logged in");
        CHECK(c.userId() == 5, "restored userId");
        QEventLoop wait; QTimer::singleShot(500, &wait, SLOT(quit())); wait.exec();
        CHECK(c.accountLoaded(), "account data loaded after restore");
    }

    printf("== 4. restart with an expired token -> logged out, ini cleaned\n");
    {
        AppSettings::saveLogin("EXPIRED", 5);
        ApiClient c; c.setBaseUrl(base);
        c.restoreSession();
        QEventLoop wait; QTimer::singleShot(800, &wait, SLOT(quit())); wait.exec();
        CHECK(!c.isLoggedIn(), "expired token drops the login");
        CHECK(readIni().indexOf("EXPIRED") < 0, "expired login removed from ini");
    }

    printf("== 5. logout clears state and ini\n");
    {
        ApiClient c; c.setBaseUrl(base);
        c.login("u", "good");
        waitFor(&c, SIGNAL(loginFinished(bool,QString)));
        CHECK(c.isLoggedIn(), "logged in before logout");
        c.logout();
        CHECK(!c.isLoggedIn(), "not logged in after logout");
        CHECK(readIni().indexOf("TOK123") < 0, "token removed from ini on logout");
    }

    printf("== 6. registration refused by the server is a failure, not a session\n");
    {
        ApiClient c; c.setBaseUrl(base);
        bool ok = true;
        QObject::connect(&c, &ApiClient::registerFinished, [&](bool o, const QString &){ ok = o; });
        c.registerUser("taken", "good", "e@x");   // server answers -4: name already taken
        waitFor(&c, SIGNAL(registerFinished(bool,QString)));
        CHECK(!ok, "taken name is reported as failure");
        CHECK(!c.isLoggedIn(), "not logged in after failed register");
    }

    printf("== 7. successful registration logs in and saves the login\n");
    {
        AppSettings::clearLogin();
        ApiClient c; c.setBaseUrl(base);
        bool ok = false;
        QObject::connect(&c, &ApiClient::registerFinished, [&](bool o, const QString &){ ok = o; });
        c.registerUser("new", "good", "e@x");
        waitFor(&c, SIGNAL(registerFinished(bool,QString)));
        CHECK(ok, "registration reports success");
        CHECK(c.isLoggedIn(), "registered account is logged in");
        CHECK(c.userId() == 9, "userId from registration answer");
        CHECK(readIni().contains("userId=9"), "registered login saved to ObjectHub.ini");
        QEventLoop settle; QTimer::singleShot(300, &settle, SLOT(quit())); settle.exec();
    }


    printf("== 8. catalog: search parses items, channels from keys, tags normalised\n");
    {
        ApiClient c; c.setBaseUrl(base);
        SearchResultList got; bool more = true; int page = -1; bool fired = false;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &r, bool m, int p){ got = r; more = m; page = p; fired = true; });
        c.search("alpha", -10, 0, 0);
        waitFor(&c, SIGNAL(searchFinished(SearchResultList,bool,int)));
        CHECK(fired, "search reply reaches the UI signal");
        CHECK(got.size() == 2, "two items parsed");
        CHECK(!more, "no next page for 2 items");
        CHECK(page == 0, "page passed through");
        CHECK(got.size() == 2 && got[0].id == 12 && got[0].title == "Alpha", "first item parsed");
        CHECK(got.size() == 2 && got[0].tags == QStringList() << "retro" << "racing", "tags normalised to lower-case list");
        CHECK(got.size() == 2 && got[1].channel == 1, "channel taken from key prefix s -> 1");
        CHECK(lastSearchPath.contains("name=alpha"), "query text sent to the server");
    }

    printf("== 9. catalog: more than one page sets hasMore\n");
    {
        ApiClient c; c.setBaseUrl(base);
        SearchResultList got; bool more = false; bool fired = false;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &r, bool m, int){ got = r; more = m; fired = true; });
        c.search("many", -10, 0, 0);
        waitFor(&c, SIGNAL(searchFinished(SearchResultList,bool,int)));
        CHECK(fired, "reply received");
        CHECK(got.size() == 8, "page cut to 8 items");
        CHECK(more, "hasMore set when a 9th item exists");
    }

    printf("== 10. catalog: news page is 10 items and hasMore when 11 exist\n");
    {
        ApiClient c; c.setBaseUrl(base);
        SearchResultList got; bool more = false; bool fired = false;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &r, bool m, int){ got = r; more = m; fired = true; });
        c.fetchNews(0);
        waitFor(&c, SIGNAL(searchFinished(SearchResultList,bool,int)));
        CHECK(fired, "news reply received");
        CHECK(got.size() == 10, "news page cut to 10 items");
        CHECK(more, "hasMore for 11 news items");
    }

    printf("== 11. catalog: vacancies list\n");
    {
        ApiClient c; c.setBaseUrl(base);
        SearchResultList got; bool fired = false;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &r, bool, int){ got = r; fired = true; });
        c.fetchVacancies(0);
        waitFor(&c, SIGNAL(searchFinished(SearchResultList,bool,int)));
        CHECK(fired, "vacancy reply received");
        CHECK(got.size() == 3, "three vacancies parsed");
        CHECK(got.size() == 3 && got[0].channel == kChannelVacancies, "vacancies marked with the vacancy channel");
    }


    printf("== 12. profile: parsed fields, and the 'not found' answer\n");
    {
        ApiClient c; c.setBaseUrl(base);
        PublicProfile got; bool fired = false;
        QObject::connect(&c, &ApiClient::profileFinished, [&](const PublicProfile &p){ got = p; fired = true; });
        c.fetchProfile(5);
        waitFor(&c, SIGNAL(profileFinished(PublicProfile)));
        CHECK(fired, "profile reply reaches the UI signal");
        CHECK(got.userId == 5 && got.username == "neo", "id and username parsed");
        CHECK(got.nickname == "Neo" && got.resume == "hi" && got.socials == "x", "nickname, resume, socials parsed");

        QString err; bool errFired = false;
        QObject::connect(&c, &ApiClient::profileError, [&](const QString &e){ err = e; errFired = true; });
        c.fetchProfile(404);
        waitFor(&c, SIGNAL(profileError(QString)));
        CHECK(errFired && err == "User not found", "[\"NONE\"] reported as user not found");
    }

    printf("== 13. comments: first page of 10, hasMore for 11 comments\n");
    {
        ApiClient c; c.setBaseUrl(base);
        DetailData got; bool fired = false;
        QObject::connect(&c, &ApiClient::detailFinished, [&](const DetailData &d){ got = d; fired = true; });
        c.fetchComments(0, 12, 0);
        waitFor(&c, SIGNAL(detailFinished(DetailData)));
        CHECK(fired, "comments reply reaches the UI signal");
        CHECK(got.comments.size() == 10, "comments cut to one page of 10");
        CHECK(got.hasMoreComments, "hasMoreComments for 11 comments");
        CHECK(got.commentsPage == 0 && got.contentType == 0, "page and content type carried through");
    }


    printf("== 14. social: like toggles the state and reports counts\n");
    {
        ApiClient c; c.setBaseUrl(base);
        bool fired = false; int likes = -1, dislikes = -1;
        QObject::connect(&c, &ApiClient::votesChanged, [&](int, int, int l, int d){ fired = true; likes = l; dislikes = d; });
        c.vote(0, 42, false);
        waitFor(&c, SIGNAL(votesChanged(int,int,int,int)));
        CHECK(fired && likes == 5 && dislikes == 2, "vote answer counts reach the UI");
        CHECK(c.likeState(0, 42) == 1, "like recorded in the account state");
        c.vote(0, 42, false);
        QEventLoop settle; QTimer::singleShot(300, &settle, SLOT(quit())); settle.exec();
        CHECK(c.likeState(0, 42) == 0, "second vote on the same item removes it");
    }

    printf("== 15. social: comment post returns the first page; edit and delete report back\n");
    {
        ApiClient c; c.setBaseUrl(base);
        int updated = 0; bool more = true; bool edited = false; bool deleted = false;
        QObject::connect(&c, &ApiClient::commentsUpdated, [&](int, int, const SearchResultList &l, bool m){ updated = l.size(); more = m; });
        QObject::connect(&c, &ApiClient::commentEdited, [&](int, const QString &t){ edited = (t == "edited text"); });
        QObject::connect(&c, &ApiClient::commentDeleted, [&](int id){ deleted = (id == 7); });
        c.postComment(0, 12, "hello");
        waitFor(&c, SIGNAL(commentsUpdated(int,int,SearchResultList,bool)));
        CHECK(updated == 2 && !more, "posted comment list parsed and passed on");
        c.editComment(7, "new text");
        waitFor(&c, SIGNAL(commentEdited(int,QString)));
        CHECK(edited, "edited text reported");
        c.deleteComment(7, 0);
        waitFor(&c, SIGNAL(commentDeleted(int)));
        CHECK(deleted, "deletion reported for the same id");
    }

    printf("== 16. social: profile field save\n");
    {
        ApiClient c; c.setBaseUrl(base);
        int field = -1; QString value;
        QObject::connect(&c, &ApiClient::profileFieldSaved, [&](int f, const QString &v){ field = f; value = v; });
        c.setNickname("NewNick");
        waitFor(&c, SIGNAL(profileFieldSaved(int,QString)));
        CHECK(field == 0 && value == "NewNick", "nickname saved and reported as field 0");
    }

    printf("== 17. social: subscribe updates state, list pages parse\n");
    {
        ApiClient c; c.setBaseUrl(base);
        bool subChanged = false; bool listed = false; int count = -1;
        QObject::connect(&c, &ApiClient::subscriptionChanged, [&](int id, bool s){ subChanged = (id == 9 && s); });
        QObject::connect(&c, &ApiClient::subscriptionsLoaded, [&](const QVariantList &items, int, bool, int){ listed = true; count = items.size(); });
        c.subscribe(9, false);
        waitFor(&c, SIGNAL(subscriptionChanged(int,bool)));
        CHECK(subChanged, "subscription change reported");
        CHECK(c.isSubscribed(9), "subscription recorded in the account state");
        c.subscribe(9, true);
        QEventLoop settle; QTimer::singleShot(300, &settle, SLOT(quit())); settle.exec();
        CHECK(!c.isSubscribed(9), "unsubscribe removes it from the state");
        c.fetchSubscriptions(0);
        waitFor(&c, SIGNAL(subscriptionsLoaded(QVariantList,int,bool,int)));
        CHECK(listed && count == 2, "subscription list parsed");
    }


    printf("== 18. account: owned projects come from loginT, accountChanged fires\n");
    {
        ApiClient c; c.setBaseUrl(base);
        c.login("u", "good");
        waitFor(&c, SIGNAL(loginFinished(bool,QString)));
        bool changed = false;
        QObject::connect(&c, &ApiClient::accountChanged, [&]{ changed = true; });
        c.loadAccount();
        // Both loginT and likesT reply with accountChanged; wait until loginT has been applied.
        QEventLoop settle; QTimer::singleShot(600, &settle, SLOT(quit())); settle.exec();
        CHECK(changed, "accountChanged emitted after loading account data");
        CHECK(c.ownsProject(12), "project 12 is owned");
        CHECK(!c.ownsProject(13), "project 13 is not owned");
        CHECK(c.accountLoaded(), "accountLoaded true");
    }

    printf("== 19. vacancy owner probe\n");
    {
        ApiClient c; c.setBaseUrl(base);
        int probed = -1; bool owned = false;
        QObject::connect(&c, &ApiClient::vacancyOwnership, [&](int id, bool o){ probed = id; owned = o; });
        c.probeVacancy(3);
        waitFor(&c, SIGNAL(vacancyOwnership(int,bool)));
        CHECK(probed == 3 && owned, "owner answer on vacancy 3 reported as owned");
    }

    printf("== 20. notifications: list parsed, deletion reported\n");
    {
        ApiClient c; c.setBaseUrl(base);
        QVariantList items; bool more = true; bool listed = false;
        QObject::connect(&c, &ApiClient::alarmsLoaded, [&](const QVariantList &l, int, bool m){ items = l; more = m; listed = true; });
        c.fetchAlarms(0);
        waitFor(&c, SIGNAL(alarmsLoaded(QVariantList,int,bool)));
        CHECK(listed && items.size() == 2, "two notifications parsed");
        CHECK(!more, "no next page for 2 notifications");
        CHECK(items.size() == 2 && items[0].toMap().value("title").toString() == "A", "title read from the row");
        int deleted = -1;
        QObject::connect(&c, &ApiClient::alarmDeleted, [&](int id){ deleted = id; });
        c.deleteAlarm(4);
        waitFor(&c, SIGNAL(alarmDeleted(int)));
        CHECK(deleted == 4, "deletion reported for notification 4");
    }

    printf("== 21. portfolio: delete reports success with the id\n");
    {
        ApiClient c; c.setBaseUrl(base);
        bool ok = false; int id = -1;
        QObject::connect(&c, &ApiClient::actionFinished, [&](int, bool o, const QString &, int i){ ok = o; id = i; });
        c.deleteWork(6);
        waitFor(&c, SIGNAL(actionFinished(int,bool,QString,int)));
        CHECK(ok && id == 6, "portfolio item 6 deleted and reported");
    }


    printf("== 22. search keeps the server's order (object answers)\n");
    {
        ApiClient c; c.setBaseUrl(base);
        QStringList order; bool fired = false;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &r, bool, int){
            fired = true; for (int i = 0; i < r.size(); ++i) order << QString::number(r[i].id); });
        c.search("order", -10, 0, 0);
        waitFor(&c, SIGNAL(searchFinished(SearchResultList,bool,int)));
        CHECK(fired, "search reply received");
        CHECK(order.join(",") == "7,3,12,1", "server order p7,s3,c12,t1 kept (was alphabetical before 1.0.1)");
    }

    printf("== 23. a full page keeps its first 8 in server order; the 9th is only the has-more marker\n");
    {
        ApiClient c; c.setBaseUrl(base);
        QStringList order; bool more = false; bool fired = false;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &r, bool m, int){
            fired = true; more = m; for (int i = 0; i < r.size(); ++i) order << QString::number(r[i].id); });
        c.search("desc", -10, 0, 0);
        waitFor(&c, SIGNAL(searchFinished(SearchResultList,bool,int)));
        CHECK(fired, "reply received");
        CHECK(order.join(",") == "9,8,7,6,5,4,3,2", "first 8 by server rank; id 1 (the marker) is dropped, not id 2");
        CHECK(more, "hasMore set by the 9th row");
    }

    printf("== 24. a newer search makes the older one obsolete\n");
    {
        ApiClient c; c.setBaseUrl(base);
        int finished = 0, errors = 0; QStringList titles;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &r, bool, int){
            ++finished; for (int i = 0; i < r.size(); ++i) titles << r[i].title; });
        QObject::connect(&c, &ApiClient::searchError, [&](const QString &){ ++errors; });
        c.search("slow", -10, 0, 0);     // answer arrives after 800 ms
        c.search("fast", -10, 0, 0);     // replaces it immediately
        QEventLoop wait; QTimer::singleShot(1500, &wait, SLOT(quit())); wait.exec();
        CHECK(finished == 1, "exactly one result set reaches the UI");
        CHECK(titles.join(",") == "Alpha,Beta", "the newer search's results are shown");
        CHECK(!titles.contains("Slow"), "the obsolete answer never overwrites the list");
        CHECK(errors == 0, "aborting the old request is not reported as an error");
    }

    printf("== 25. 'Access denied' is an error, not an empty result\n");
    {
        ApiClient c; c.setBaseUrl(base);
        int finished = 0; QString err;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &, bool, int){ ++finished; });
        QObject::connect(&c, &ApiClient::searchError, [&](const QString &e){ err = e; });
        c.search("denied", -10, 0, 0);
        waitFor(&c, SIGNAL(searchError(QString)));
        CHECK(err == "Access denied", "refusal reported as an error");
        CHECK(finished == 0, "no empty result set is shown for a refusal");
    }


    printf("== 26. OS ids are merged into the item's tags, once each\n");
    {
        ApiClient c; c.setBaseUrl(base);
        SearchResultList got; bool fired = false;
        QObject::connect(&c, &ApiClient::searchFinished, [&](const SearchResultList &r, bool, int){ got = r; fired = true; });
        c.search("os", -10, 0, 0);
        waitFor(&c, SIGNAL(searchFinished(SearchResultList,bool,int)));
        CHECK(fired && got.size() == 1, "one item parsed");
        CHECK(got.size() == 1 && got[0].tags == (QStringList() << "1" << "2" << "12"),
              "tags 1,2 and OS 12,2 give 1,2,12 (the duplicate 2 is dropped)");
    }

    printf("== 27. tag dictionary: ranges and keys\n");
    {
        CHECK(tagKey(0, 1) == "Camptag1", "camp tag 1");
        CHECK(tagKey(0, 11) == "Camptag11", "camp OS 11 is in the dictionary");
        CHECK(tagKey(0, 16) == "", "camp id 16 does not exist");
        CHECK(tagKey(1, 10) == "", "show has no tag 10");
        CHECK(tagKey(1, 12) == "Showtag12", "show OS 12");
        CHECK(tagKey(1, 11) == "", "show has no id 11");
        CHECK(tagKey(2, 3) == "Peretag3", "pere uses its own keys (same texts as show)");
        CHECK(tagKey(3, 9) == "Teletag9", "tele tag 9");
        CHECK(tagKey(kChannelVacancies, 10) == "Vacstag10", "vacancy tag 10");
        CHECK(tagKey(kChannelVacancies, 11) == "", "vacancies have no id 11");
        CHECK(tagKey(kChannelNews, 1) == "", "news have no dictionary");
        CHECK(tagKey(kChannelAll, 1) == "", "all projects have no dictionary");
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails ? 1 : 0;
}
#include "test.moc"
