#include "tagnames.h"

#include "lang.h"
#include "api/searchresult.h"

namespace {

const TagChannelInfo kInfo[] = {
    { "Camptag", 1, 10, 11, 15, "tags00", "os00" },
    { "Showtag", 1, 9,  12, 15, "tags01", "os01" },
    { "Peretag", 1, 9,  12, 15, "tags02", "os02" },
    { "Teletag", 1, 9,  12, 15, "tags03", "os03" },
    { "Vacstag", 1, 10, 0,  0,  "tags",   ""     },   // vacancies: tags only
};
const int kVacancyIndex = 4;

// -1 for a channel that has no dictionary (news, all projects).
int indexFor(int channel)
{
    if (channel >= 0 && channel <= 3)
        return channel;
    if (channel == kChannelVacancies)
        return kVacancyIndex;
    return -1;
}

} // namespace

const TagChannelInfo &tagChannelInfo(int channel)
{
    const int i = indexFor(channel);
    return kInfo[i >= 0 ? i : 0];
}

QString tagKey(int channel, int id)
{
    const int i = indexFor(channel);
    if (i < 0)
        return QString();
    const TagChannelInfo &c = kInfo[i];
    const bool isTag = id >= c.tagFrom && id <= c.tagTo;
    const bool isOs = c.osFrom > 0 && id >= c.osFrom && id <= c.osTo;
    if (!isTag && !isOs)
        return QString();
    return QString::fromLatin1(c.prefix) + QString::number(id);
}

QString tagLabel(int channel, int id)
{
    const QString key = tagKey(channel, id);
    if (key.isEmpty())
        return QString::number(id);
    const QString text = L(key.toUtf8().constData());
    return text == key ? QString::number(id) : text;   // L() returns the key itself when missing
}

QString tagsText(int channel, const QStringList &tags)
{
    QStringList names;
    foreach (const QString &t, tags) {
        bool ok = false;
        const int id = t.toInt(&ok);
        names << (ok ? tagLabel(channel, id) : t);
    }
    return names.join(QStringLiteral(", "));
}
