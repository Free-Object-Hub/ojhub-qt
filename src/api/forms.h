#ifndef FORMS_H
#define FORMS_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QPair>
#include <QMetaType>

// What a write or form-load request is for. Sent back in actionFinished().
enum WriteAction {
    ActLoadProject = 1,
    ActLoadVacancy,
    ActSaveProject,
    ActSaveVacancy,
    ActRemoveVacancy,
    ActApply,
    ActUnapply,
    ActSaveNews,
    ActDeleteNews,
    ActVote,
    ActPostComment,
    ActEditComment,
    ActDeleteComment,
    ActSaveNick,
    ActSaveResume,
    ActSaveSocials,
    ActSub,
    ActUnsub,
    ActLoadSubs,
    ActLoadAlarms,
    ActLoadAlarm,
    ActDeleteAlarm,
    ActProbeVacancy,
    ActProbeNews,
    ActSaveWork,
    ActDeleteWork,
    ActLoadLikes,
    ActLoadAccount
};

// Project (camp 0, show 1, pere 2, tele 3) as the edit form needs it.
// description and shortText are keyed by language: RU, EN, UA.
struct ProjectForm {
    int channel;
    int id;                                  // 0 = new project
    QString title;
    QMap<QString, QString> description;
    QMap<QString, QString> shortText;        // max 200 chars per language
    QList<QPair<QString, QString> > links;   // (name, url)
    QString img;                             // url or existing link
    QString ban;
    QList<int> tags;
    QList<int> os;

    ProjectForm() : channel(0), id(0) {}
};

struct VacancyForm {
    int id;          // 0 = new vacancy
    int gdpsId;      // project the vacancy belongs to
    QString title;
    QString text;
    QString shortText;
    QList<int> tags;

    VacancyForm() : id(0), gdpsId(0) {}
};

struct NewsForm {
    int id;          // 0 = new news
    int gdpsId;      // project id (number, no channel letter)
    QString title;
    QString text;

    NewsForm() : id(0), gdpsId(0) {}
};

// Portfolio item. linkType 0 = project from the catalog (link is its id), 1 = external url.
struct WorkForm {
    int id;          // 0 = new
    QString title;   // max 100
    QString text;    // max 2000
    int linkType;
    QString link;    // project id or url, max 255

    WorkForm() : id(0), linkType(0) {}
};

Q_DECLARE_METATYPE(ProjectForm)
Q_DECLARE_METATYPE(VacancyForm)

// "1,5 12;7" -> [1, 5, 12, 7]. Keeps order, drops zeros, duplicates and junk.
inline QList<int> parseIdCsv(const QString &text)
{
    QString s = text;
    s.replace(QLatin1Char(';'), QLatin1Char(','));
    s.replace(QLatin1Char(' '), QLatin1Char(','));
    const QStringList parts = s.split(QLatin1Char(','));
    QList<int> out;
    for (int i = 0; i < parts.size(); ++i) {
        bool ok = false;
        const int n = parts.at(i).trimmed().toInt(&ok);
        if (ok && n > 0 && !out.contains(n))
            out.append(n);
    }
    return out;
}

// Human text for an openGo error code ("-27", "Access denied", ...). Defined in core/protocol.cpp.
QString serverErrorText(const QString &code);

#endif // FORMS_H
