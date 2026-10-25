#ifndef TAGNAMES_H
#define TAGNAMES_H

#include <QString>
#include <QStringList>

// The tag dictionary of the site (the JS tables Tags, Os and TagsVacs), in one place.
// Tags and OS share one numeric space per channel and go through the same normalisation;
// only their storage differs. The dictionary turns an id into a translated name.

struct TagChannelInfo {
    const char *prefix;      // L10n key prefix: Camptag, Showtag, Peretag, Teletag, Vacstag
    int tagFrom, tagTo;      // tag ids
    int osFrom, osTo;        // OS ids (0 = the channel has no OS group)
    const char *tagsTitle;   // L10n key of the tags group title
    const char *osTitle;     // L10n key of the OS group title
};

// channel: 0 camp, 1 show, 2 pere, 3 tele, kChannelVacancies for vacancies.
// An unknown channel falls back to camp, the same way the picker always did.
const TagChannelInfo &tagChannelInfo(int channel);

// "Camptag12" for a known id of the channel, "" when the id is not in the dictionary.
QString tagKey(int channel, int id);

// The translated name, or the bare number when there is no translation.
QString tagLabel(int channel, int id);

// "Name1, Name2" for a normalised tag list; values that are not numbers are shown as they are.
QString tagsText(int channel, const QStringList &tags);

#endif // TAGNAMES_H
