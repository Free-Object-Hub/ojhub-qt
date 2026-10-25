#include "lang.h"

#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

Lang &Lang::instance()
{
    static Lang s;
    return s;
}

Lang::Lang()
{
    loadDefaults();
}

void Lang::loadDefaults()
{
    /* Keys aligned with Object Hub RU.json / GHE */
    m_map.insert(QString::fromLatin1("ojhubname"), QString::fromUtf8("Object Hub"));
    m_map.insert(QString::fromLatin1("projects"), QString::fromUtf8("Projects"));
    m_map.insert(QString::fromLatin1("guides09"), QString::fromUtf8("Wiki"));
    m_map.insert(QString::fromLatin1("vacancies"), QString::fromUtf8("Vacancies"));
    m_map.insert(QString::fromLatin1("findName"), QString::fromUtf8("Title"));
    m_map.insert(QString::fromLatin1("finder-name"), QString::fromUtf8("Search"));
    m_map.insert(QString::fromLatin1("finder-channel"), QString::fromUtf8("What are you looking for?"));
    m_map.insert(QString::fromLatin1("searchCamps"), QString::fromUtf8("Camps"));
    m_map.insert(QString::fromLatin1("searchShows"), QString::fromUtf8("Object shows"));
    m_map.insert(QString::fromLatin1("searchPeres"), QString::fromUtf8("Dubs"));
    m_map.insert(QString::fromLatin1("searchTeles"), QString::fromUtf8("TG channels"));
    m_map.insert(QString::fromLatin1("channelAll"), QString::fromUtf8("All channels"));
    m_map.insert(QString::fromLatin1("otherSort"), QString::fromUtf8("Search method"));
    m_map.insert(QString::fromLatin1("search4"), QString::fromUtf8("Newest"));
    m_map.insert(QString::fromLatin1("mostLike"), QString::fromUtf8("Most liked"));
    m_map.insert(QString::fromLatin1("mostDisl"), QString::fromUtf8("Most disliked"));
    m_map.insert(QString::fromLatin1("search1"), QString::fromUtf8("Most active"));
    m_map.insert(QString::fromLatin1("addedBy"), QString::fromUtf8("Author"));
    m_map.insert(QString::fromLatin1("login"), QString::fromUtf8("Log in"));
    m_map.insert(QString::fromLatin1("register"), QString::fromUtf8("Register"));
    m_map.insert(QString::fromLatin1("logout"), QString::fromUtf8("Log out"));
    m_map.insert(QString::fromLatin1("logout2"), QString::fromUtf8("Log out of account"));
    m_map.insert(QString::fromLatin1("account"), QString::fromUtf8("Account"));
    m_map.insert(QString::fromLatin1("loggedAs"), QString::fromUtf8("Logged in"));
    m_map.insert(QString::fromLatin1("guest"), QString::fromUtf8("Guest"));
    m_map.insert(QString::fromLatin1("profile"), QString::fromUtf8("Profile"));
    m_map.insert(QString::fromLatin1("login01"), QString::fromUtf8("Login or email"));
    m_map.insert(QString::fromLatin1("login02"), QString::fromUtf8("Password"));
    m_map.insert(QString::fromLatin1("login03"), QString::fromUtf8("Email address"));
    m_map.insert(QString::fromLatin1("login06"), QString::fromUtf8("Login"));
    m_map.insert(QString::fromLatin1("otmena"), QString::fromUtf8("Cancel"));
    m_map.insert(QString::fromLatin1("wrongPass"), QString::fromUtf8("Wrong password!"));
    m_map.insert(QString::fromLatin1("accountEmpty"), QString::fromUtf8("This account does not exist!"));
    m_map.insert(QString::fromLatin1("loginClaimed"), QString::fromUtf8("Login or email is already taken!"));
    m_map.insert(QString::fromLatin1("captchaDed"), QString::fromUtf8("Captcha failed!"));
    m_map.insert(QString::fromLatin1("somethingWentWrong"), QString::fromUtf8("Something went wrong"));
    m_map.insert(QString::fromLatin1("WINDOW-logonWindow"), QString::fromUtf8("Log in"));
    m_map.insert(QString::fromLatin1("WINDOW-logon2Window"), QString::fromUtf8("Register"));
    m_map.insert(QString::fromLatin1("noResults"), QString::fromUtf8("Nothing found."));
    m_map.insert(QString::fromLatin1("found"), QString::fromUtf8("Found: %1. Click to open."));
    m_map.insert(QString::fromLatin1("ready"), QString::fromUtf8("Ready"));
    m_map.insert(QString::fromLatin1("searching"), QString::fromUtf8("Searching..."));
    m_map.insert(QString::fromLatin1("error"), QString::fromUtf8("Error"));
    m_map.insert(QString::fromLatin1("noTitle"), QString::fromUtf8("(untitled)"));
    m_map.insert(QString::fromLatin1("noDesc"), QString::fromUtf8("(no description)"));
    m_map.insert(QString::fromLatin1("tags"), QString::fromUtf8("Tags"));
    m_map.insert(QString::fromLatin1("os"), QString::fromUtf8("OS"));
    m_map.insert(QString::fromLatin1("menuTools"), QString::fromUtf8("Tools"));
    m_map.insert(QString::fromLatin1("menuProfile"), QString::fromUtf8("Open profile by ID..."));
    m_map.insert(QString::fromLatin1("fillFields"), QString::fromUtf8("Fill in all fields"));
    m_map.insert(QString::fromLatin1("errFillLogin"), QString::fromUtf8("Enter login and password"));
    m_map.insert(QString::fromLatin1("errFillAll"), QString::fromUtf8("Fill in all fields"));
    m_map.insert(QString::fromLatin1("newsTab"), QString::fromUtf8("News"));
    m_map.insert(QString::fromLatin1("newsHead"), QString::fromUtf8("Project news"));
    m_map.insert(QString::fromLatin1("commentsHead"), QString::fromUtf8("Comments (%1)"));
    m_map.insert(QString::fromLatin1("noComments"), QString::fromUtf8("No comments yet"));
    m_map.insert(QString::fromLatin1("noNews"), QString::fromUtf8("No news yet"));
    m_map.insert(QString::fromLatin1("notFound"), QString::fromUtf8("Item not found on the server"));
    m_map.insert(QString::fromLatin1("worksHead"), QString::fromUtf8("Portfolio"));
    m_map.insert(QString::fromLatin1("addedCamps"), QString::fromUtf8("Added camps"));
    m_map.insert(QString::fromLatin1("addedShows"), QString::fromUtf8("Added shows"));
    m_map.insert(QString::fromLatin1("addedPeres"), QString::fromUtf8("Added dubs"));
    m_map.insert(QString::fromLatin1("profRole"), QString::fromUtf8("Role"));
    m_map.insert(QString::fromLatin1("profId"), QString::fromUtf8("ID"));
    m_map.insert(QString::fromLatin1("profSocials"), QString::fromUtf8("Socials"));
    m_map.insert(QString::fromLatin1("profResume"), QString::fromUtf8("About"));
    m_map.insert(QString::fromLatin1("isActive"), QString::fromUtf8("account active"));
    m_map.insert(QString::fromLatin1("isNotact"), QString::fromUtf8("account not active"));
    m_map.insert(QString::fromLatin1("noWorks"), QString::fromUtf8("No portfolio items yet"));
    m_map.insert(QString::fromLatin1("noItems"), QString::fromUtf8("Nothing here"));
    m_map.insert(QString::fromLatin1("back"), QString::fromUtf8("Back"));
    m_map.insert(QString::fromLatin1("likesLine"), QString::fromUtf8("+%1   −%2   comments: %3"));
    m_map.insert(QString::fromLatin1("tagsLine"), QString::fromUtf8("Tags: %1"));
    m_map.insert(QString::fromLatin1("descEmpty"), QString::fromUtf8("(no description)"));
    // Comments are counted but not loaded: the client has no comments endpoint yet.
    m_map.insert(QString::fromLatin1("commentsSoon"), QString::fromUtf8("Comments (%1) are not loaded yet: no endpoint."));
    m_map.insert(QString::fromLatin1("showMore"), QString::fromUtf8("Show more"));
    m_map.insert(QString::fromLatin1("userIdPrompt"), QString::fromUtf8("User ID:"));
}

QString Lang::tr(const QString &key) const
{
    if (m_map.contains(key))
        return m_map.value(key);
    return key;
}

void Lang::set(const QString &key, const QString &value)
{
    m_map.insert(key, value);
}

bool Lang::loadFromJsonFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    QJsonObject obj = doc.object();
    QStringList keys = obj.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const QString k = keys.at(i);
        QJsonValue v = obj.value(k);
        if (v.isString())
            m_map.insert(k, v.toString());
    }
    return true;
}
