#include "tagpicker.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>

#include "lang.h"
#include "tagnames.h"
#include "api/searchresult.h"

namespace {

// One group of checkboxes, two per row. The label of each box is the translated tag name.
QGroupBox *makeGroup(const QString &title, const QString &prefix, int from, int to,
                     const QList<int> &selected, QList<QPair<QCheckBox *, int> > &out, QWidget *parent)
{
    QGroupBox *box = new QGroupBox(title, parent);
    QGridLayout *grid = new QGridLayout(box);
    int row = 0, col = 0;
    for (int id = from; id <= to; ++id) {
        const QString key = prefix + QString::number(id);
        QCheckBox *cb = new QCheckBox(L(key.toUtf8().constData()), box);
        cb->setChecked(selected.contains(id));
        grid->addWidget(cb, row, col);
        out.append(qMakePair(cb, id));
        if (++col == 2) {
            col = 0;
            ++row;
        }
    }
    return box;
}

} // namespace

TagPicker::TagPicker(int channel, bool vacancy, const QList<int> &tags, const QList<int> &os, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    const TagChannelInfo &c = tagChannelInfo(vacancy ? kChannelVacancies : channel);
    root->addWidget(makeGroup(L(c.tagsTitle), QString::fromLatin1(c.prefix), c.tagFrom, c.tagTo,
                              tags, m_tagBoxes, this));
    if (c.osFrom > 0)
        root->addWidget(makeGroup(L(c.osTitle), QString::fromLatin1(c.prefix), c.osFrom, c.osTo,
                                  os, m_osBoxes, this));
}

static QList<int> checkedIds(const QList<QPair<QCheckBox *, int> > &boxes)
{
    QList<int> out;
    for (int i = 0; i < boxes.size(); ++i) {
        if (boxes.at(i).first->isChecked())
            out.append(boxes.at(i).second);
    }
    return out;
}

QList<int> TagPicker::tags() const
{
    return checkedIds(m_tagBoxes);
}

QList<int> TagPicker::os() const
{
    return checkedIds(m_osBoxes);
}
