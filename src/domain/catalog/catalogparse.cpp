#include "catalogparse.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#include "../../api/searchresult.h"

// Tags arrive either as an array (numbers or strings) or as one delimited string.
// Normalised to: trimmed, no leading '#', lower-case, no empties, no duplicates,
// original order kept. Lower-casing is why "Retro" and "retro" count as one tag.
QStringList normalizeTags(const QJsonValue &v)
{
    QStringList raw;
    if (v.isArray()) {
        QJsonArray a = v.toArray();
        for (int i = 0; i < a.size(); ++i) {
            const QJsonValue e = a.at(i);
            raw << (e.isDouble() ? QString::number(e.toInt()) : e.toString());
        }
    } else if (v.isString()) {
        // ';' and spaces are treated as separators too, since users type tags loosely
        QString s = v.toString();
        s.replace(QLatin1Char(';'), QLatin1Char(','));
        s.replace(QLatin1Char(' '), QLatin1Char(','));
        raw = s.split(QLatin1Char(','));
    }

    QStringList out;
    for (int i = 0; i < raw.size(); ++i) {
        QString t = raw.at(i).trimmed();
        while (t.startsWith(QLatin1Char('#')))
            t.remove(0, 1);
        t = t.toLower();
        if (t.isEmpty() || out.contains(t))
            continue;
        out << t;
    }
    return out;
}

QString firstString(const QJsonObject &o, const QStringList &keys)
{
    for (int i = 0; i < keys.size(); ++i) {
        const QString v = o.value(keys.at(i)).toString();
        if (!v.isEmpty())
            return v;
    }
    return QString();
}

// Shared by search, news and detail. Keys are tried in the order the JS client uses.
SearchResult parseItem(const QJsonObject &item, int channel, const QString &key)
{
    SearchResult r;
    r.id = item.value(QStringLiteral("ID")).toInt(item.value(QStringLiteral("id")).toInt());
    r.title = item.value(QStringLiteral("title")).toString();
    r.shortDesc = item.value(QStringLiteral("short")).toString();
    if (r.shortDesc.isEmpty())
        r.shortDesc = item.value(QStringLiteral("description")).toString();
    r.author = item.value(QStringLiteral("username")).toString();
    if (r.author.isEmpty())
        r.author = item.value(QStringLiteral("author")).toString();
    r.imageUrl = item.value(QStringLiteral("img")).toString();

    // Why both shapes: the server moved to [likes, dislikes, comments]; old responses send an int.
    const QJsonValue lv = item.value(QStringLiteral("likes"));
    if (lv.isArray()) {
        QJsonArray a = lv.toArray();
        r.likes    = a.size() > 0 ? a.at(0).toInt() : 0;
        r.dislikes = a.size() > 1 ? a.at(1).toInt() : 0;
        r.comments = a.size() > 2 ? a.at(2).toInt() : 0;
    } else {
        r.likes = lv.toInt();
    }
    r.tags = normalizeTags(item.value(QStringLiteral("tags")));
    // OS ids are stored in their own field but use the same dictionary and normalisation: merge them.
    foreach (const QString &os, normalizeTags(item.value(QStringLiteral("os")))) {
        if (!r.tags.contains(os))
            r.tags << os;
    }
    r.channel = channel;
    r.typePrefix = key.isEmpty() ? QString() : key.left(1);
    r.raw = item.toVariantMap();
    if (r.title.isEmpty())
        r.title = key;
    return r;
}

void applyLikes(SearchResult &r, const QJsonValue &lv)
{
    if (lv.isArray()) {
        QJsonArray a = lv.toArray();
        r.likes    = a.size() > 0 ? a.at(0).toInt() : 0;
        r.dislikes = a.size() > 1 ? a.at(1).toInt() : 0;
        r.comments = a.size() > 2 ? a.at(2).toInt() : 0;
    } else {
        r.likes = lv.toInt();
    }
}

// News item, as the JS client reads it: an array
//   [ID, title, text, author, username, gdpsId, gdpsTitle, date, likes, isLiked, hasFile, gdpsImg]
// gdpsId carries the project channel as its first letter: c=camp, s=show, p=pere, t=tele.
SearchResult parseNewsItem(const QJsonValue &v)
{
    SearchResult r;
    r.channel = kChannelNews;
    QString gdpsId;
    QJsonValue likes;

    if (v.isArray()) {
        QJsonArray a = v.toArray();
        auto at = [&a](int k) -> QJsonValue { return k < a.size() ? a.at(k) : QJsonValue(); };
        r.id = at(0).toInt();
        r.title = at(1).toString();
        r.shortDesc = at(2).toString();
        r.author = at(4).toString();
        r.raw.insert(QStringLiteral("authorId"), at(3).toInt());
        gdpsId = at(5).toString();
        r.raw.insert(QStringLiteral("gdpsTitle"), at(6).toString());
        r.raw.insert(QStringLiteral("date"), at(7).toDouble());
        likes = at(8);
        r.raw.insert(QStringLiteral("hasFile"), at(10).toString());
        r.imageUrl = at(11).toString();
    } else if (v.isObject()) {
        const QJsonObject o = v.toObject();
        r.id = o.value(QStringLiteral("ID")).toInt(o.value(QStringLiteral("id")).toInt());
        r.title = o.value(QStringLiteral("title")).toString();
        r.shortDesc = o.value(QStringLiteral("text")).toString();
        r.author = o.value(QStringLiteral("username")).toString();
        gdpsId = o.value(QStringLiteral("gdpsId")).toString();
        r.raw = o.toVariantMap();
        likes = o.value(QStringLiteral("likes"));
        r.imageUrl = o.value(QStringLiteral("gdpsImg")).toString();
    }

    applyLikes(r, likes);
    r.raw.insert(QStringLiteral("gdpsId"), gdpsId);
    // Project id is the number after the channel letter; the detail page uses it to link out.
    r.raw.insert(QStringLiteral("gdpsNum"), gdpsId.mid(1).toInt());
    int projChannel = 0;
    if (!gdpsId.isEmpty()) {
        switch (gdpsId.at(0).toLatin1()) {
        case 's': projChannel = 1; break;
        case 'p': projChannel = 2; break;
        case 't': projChannel = 3; break;
        default:  projChannel = 0; break;
        }
    }
    r.raw.insert(QStringLiteral("gdpsChannel"), projChannel);
    if (r.title.isEmpty())
        r.title = QStringLiteral("#%1").arg(r.id);
    return r;
}

// News list (news.php / newsAll.php): object or array of items, paged with a sentinel.
SearchResultList parseNewsList(const QByteArray &data, int pageSize, bool *hasMore)
{
    SearchResultList list;
    if (hasMore)
        *hasMore = false;

    const QByteArray t = data.trimmed();
    if (t.isEmpty() || t == "null" || t == "{}" || t == "[\"NONE\"]")
        return list;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(t, &err);
    if (err.error != QJsonParseError::NoError)
        return list;

    if (doc.isObject()) {
        QJsonObject o = doc.object();
        QStringList keys = o.keys();
        for (int i = 0; i < keys.size(); ++i)
            list.append(parseNewsItem(o.value(keys.at(i))));
    } else if (doc.isArray()) {
        QJsonArray a = doc.array();
        for (int i = 0; i < a.size(); ++i)
            list.append(parseNewsItem(a.at(i)));
    }

    if (list.size() > pageSize) {
        if (hasMore)
            *hasMore = true;
        list = list.mid(0, pageSize);
    }
    return list;
}

void truncateTo(SearchResultList &list, int n, bool *more)
{
    if (more)
        *more = false;
    if (list.size() > n) {
        if (more)
            *more = true;
        list = list.mid(0, n);
    }
}

int projectChannelFromKey(const QString &key)
{
    if (key.isEmpty())
        return 0;
    switch (key.at(0).toLatin1()) {
    case 's': return 1;
    case 'p': return 2;
    case 't': return 3;
    default:  return 0;
    }
}

// ---------- key order ----------

static void skipJsonString(const QByteArray &s, int &i)
{
    ++i;                                    // opening quote
    while (i < s.size()) {
        if (s.at(i) == '\\') {
            i += 2;                         // escaped character
        } else if (s.at(i) == '"') {
            ++i;
            return;
        } else {
            ++i;
        }
    }
}

static void skipJsonWhitespace(const QByteArray &s, int &i)
{
    while (i < s.size() && (s.at(i) == ' ' || s.at(i) == '\t' || s.at(i) == '\r' || s.at(i) == '\n'))
        ++i;
}

static void skipJsonValue(const QByteArray &s, int &i)
{
    if (i >= s.size())
        return;
    if (s.at(i) == '"') {
        skipJsonString(s, i);
        return;
    }
    if (s.at(i) == '{' || s.at(i) == '[') {
        int depth = 0;
        while (i < s.size()) {
            const char ch = s.at(i);
            if (ch == '"') {
                skipJsonString(s, i);
                continue;
            }
            if (ch == '{' || ch == '[')
                ++depth;
            else if (ch == '}' || ch == ']')
                --depth;
            ++i;
            if (depth == 0)
                return;
        }
        return;
    }
    // number, true, false, null
    while (i < s.size() && s.at(i) != ',' && s.at(i) != '}' && s.at(i) != ']')
        ++i;
}

QStringList topLevelKeysInOrder(const QByteArray &json)
{
    QStringList keys;
    int i = json.indexOf('{');
    if (i < 0)
        return keys;
    ++i;
    while (true) {
        skipJsonWhitespace(json, i);
        if (i >= json.size() || json.at(i) == '}')
            break;
        if (json.at(i) == ',') {
            ++i;
            continue;
        }
        if (json.at(i) != '"')
            break;                          // malformed: stop, the caller falls back to keys()
        const int start = i;
        skipJsonString(json, i);
        const QByteArray rawKey = json.mid(start, i - start);
        const QJsonArray decoded = QJsonDocument::fromJson("[" + rawKey + "]").array();
        if (decoded.isEmpty())
            break;
        keys << decoded.at(0).toString();
        skipJsonWhitespace(json, i);
        if (i >= json.size() || json.at(i) != ':')
            break;
        ++i;
        skipJsonWhitespace(json, i);
        skipJsonValue(json, i);
    }
    return keys;
}
