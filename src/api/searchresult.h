#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include <QString>
#include <QVariantMap>
#include <QList>
#include <QMetaType>
#include <QStringList>

struct SearchResult
{
    int     id;
    QString title;
    QString shortDesc;
    QString author;
    QString imageUrl;
    // likes/dislikes/comments come from the API field "likes" = [likes, dislikes, comments].
    // A bare number (old responses) only fills likes.
    int     likes;
    int     dislikes;
    int     comments;
    QStringList tags;   // normalised, see normalizeTags() in apiclient.cpp
    int     channel;
    QString typePrefix;
    QVariantMap raw;

    SearchResult()
        : id(0), likes(0), dislikes(0), comments(0), channel(0)
    {}
};

typedef QList<SearchResult> SearchResultList;

// Channel codes used in SearchResult::channel and in search()/fetch calls.
// Projects use 0..3 (camp, show, pere, tele); -10 means "all projects" in search.
enum {
    kChannelAll        = -10,
    kChannelVacancies  = -5,
    kChannelNews       = -20   // not an API channel: marks news items in the client
};

// What a detail request is for. Shared by the reply handler and the UI.
enum DetailKind {
    DetailProject      = 0,   // camp.php?id=          project + first news page + first comments page
    DetailVacancy      = 1,   // vacsC.php?id=         vacancy + first comments page
    DetailNews         = 2,   // newsC.php?id=         one news item + first comments page
    DetailProjectNews  = 3,   // news.php?id=&page=    next page of a project's news
    DetailProfile      = 4,   // getUser.php?id=       user profile (stored in the frame's history)
    DetailProfileWorks = 5,   // works.php?id=         portfolio
    DetailProfileAdded = 6,   // getAdded{Camps,Shows,Peres}.php?id=  projects added by a user
    DetailCommentsPage = 7    // fetchComms.php?id&type&page  next page of comments
};

// Result of a detail request. Which fields are filled depends on `kind`.
struct DetailData
{
    int kind;
    int id;
    int page;
    int contentType;          // comments: 0 project, 3 news, 5 vacancy (same codes as the JS client)
    bool found;               // false when the server answered ["NONE"] or the body did not parse
    SearchResult main;
    SearchResultList news;
    bool hasMoreNews;
    SearchResultList comments;
    bool hasMoreComments;
    int commentsPage;
    SearchResultList items;   // portfolio works or added projects

    DetailData()
        : kind(0), id(0), page(0), contentType(-1), found(false),
          hasMoreNews(false), hasMoreComments(false), commentsPage(0)
    {}
};

struct PublicProfile
{
    int     userId;
    QString username;
    QString nickname;
    QString resume;
    QString socials;
    QVariantMap raw;

    PublicProfile()
        : userId(0)
    {}

    QString displayName() const
    {
        if (!nickname.isEmpty())
            return nickname;
        return username;
    }
};

Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(SearchResultList)
Q_DECLARE_METATYPE(PublicProfile)
Q_DECLARE_METATYPE(DetailData)

#endif // SEARCHRESULT_H
