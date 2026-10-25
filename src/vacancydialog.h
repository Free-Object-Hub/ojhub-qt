
#ifndef VACANCYDIALOG_H
#define VACANCYDIALOG_H

#include <QDialog>

#include "api/forms.h"

class QLineEdit;
class QPlainTextEdit;
class TagPicker;

// Create or edit a vacancy of a project.
class VacancyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VacancyDialog(const VacancyForm &initial, QWidget *parent = 0);
    VacancyForm form() const;

private slots:
    void tryAccept();

private:
    VacancyForm     m_base;      // id and gdpsId, not editable here
    QLineEdit      *m_title;
    QPlainTextEdit *m_text;
    QLineEdit      *m_short;
    TagPicker      *m_tags;
};

#endif // VACANCYDIALOG_H
