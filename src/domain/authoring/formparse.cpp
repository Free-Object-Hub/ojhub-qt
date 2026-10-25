#include "formparse.h"

#include <QJsonDocument>

// Packed multi-language field "RU\ttext" or plain legacy text; plain text goes under fallback.
QMap<QString, QString> parseMultiLang(const QString &raw, const QString &fallback)
{
    QMap<QString, QString> m;
    const QString t = raw.trimmed();
    if (t.isEmpty())
        return m;
    if (t.startsWith(QLatin1Char('{'))) {
        const QJsonObject o = QJsonDocument::fromJson(t.toUtf8()).object();
        const QStringList keys = o.keys();
        for (int i = 0; i < keys.size(); ++i)
            m.insert(keys.at(i), o.value(keys.at(i)).toString());
        return m;
    }
    m.insert(fallback, raw);
    return m;
}

// Links come as object {name: url} or as a single plain string.
QList<QPair<QString, QString> > parseLinks(const QJsonValue &v)
{
    QList<QPair<QString, QString> > out;
    if (v.isObject()) {
        const QJsonObject o = v.toObject();
        const QStringList keys = o.keys();
        for (int i = 0; i < keys.size(); ++i)
            out.append(qMakePair(keys.at(i), o.value(keys.at(i)).toString()));
    } else if (v.isString() && !v.toString().trimmed().isEmpty()) {
        out.append(qMakePair(QString(), v.toString().trimmed()));
    }
    return out;
}

// Tags are stored as a JSON array text: ["1","5"] or [1,5].
QList<int> idsFromJsonText(const QString &raw)
{
    QList<int> out;
    const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    if (!doc.isArray())
        return out;
    const QJsonArray a = doc.array();
    for (int i = 0; i < a.size(); ++i) {
        const QJsonValue v = a.at(i);
        const int n = v.isDouble() ? v.toInt() : v.toString().toInt();
        if (n > 0 && !out.contains(n))
            out.append(n);
    }
    return out;
}

// GET send/{type}Edit?id= without title: [title, description, short, links, img, ban, tags, os, language]
ProjectForm parseProjectForm(const QJsonArray &a, int channel, int id)
{
    auto at = [&a](int k) -> QJsonValue { return k < a.size() ? a.at(k) : QJsonValue(); };
    ProjectForm f;
    f.channel = channel;
    f.id = id;
    f.title = at(0).toString();
    const QString lang = at(8).toString().isEmpty() ? QStringLiteral("RU") : at(8).toString();
    f.description = parseMultiLang(at(1).toString(), lang);
    f.shortText = parseMultiLang(at(2).toString(), lang);
    f.links = parseLinks(at(3));
    f.img = at(4).toString();
    f.ban = at(5).toString();
    f.tags = idsFromJsonText(at(6).toString());
    f.os = idsFromJsonText(at(7).toString());
    return f;
}

// GET vacans/edit?id= : Vacan object
VacancyForm parseVacancyForm(const QJsonObject &o)
{
    VacancyForm f;
    f.id = o.value(QStringLiteral("ID")).toInt();
    f.gdpsId = o.value(QStringLiteral("gId")).toInt();
    f.title = o.value(QStringLiteral("title")).toString();
    f.text = o.value(QStringLiteral("text")).toString();
    f.shortText = o.value(QStringLiteral("short")).toString();
    f.tags = idsFromJsonText(o.value(QStringLiteral("tags")).toString());
    return f;
}
