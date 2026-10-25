
#ifndef TAGPICKER_H
#define TAGPICKER_H

#include <QWidget>
#include <QList>
#include <QPair>

class QCheckBox;

// Checkboxes for the tags and OS of a project channel (or the tags of a vacancy).
// Names come from RU.json (Camptag1, Showtag1, ... Vacstag1).
class TagPicker : public QWidget
{
    Q_OBJECT

public:
    // channel: 0 camp, 1 show, 2 pere, 3 tele. vacancy: only tags, channel is ignored.
    TagPicker(int channel, bool vacancy, const QList<int> &tags, const QList<int> &os, QWidget *parent = 0);

    QList<int> tags() const;
    QList<int> os() const;

private:
    QList<QPair<QCheckBox *, int> > m_tagBoxes;
    QList<QPair<QCheckBox *, int> > m_osBoxes;
};

#endif // TAGPICKER_H
