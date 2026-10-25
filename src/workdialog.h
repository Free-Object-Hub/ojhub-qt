
#ifndef WORKDIALOG_H
#define WORKDIALOG_H

#include <QDialog>

#include "api/forms.h"

class QLineEdit;
class QPlainTextEdit;
class QRadioButton;

// Add or edit a portfolio item.
class WorkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WorkDialog(const WorkForm &initial, QWidget *parent = 0);
    WorkForm form() const;

private slots:
    void tryAccept();
    void typeChanged();

private:
    WorkForm        m_base;
    QLineEdit      *m_title;
    QPlainTextEdit *m_text;
    QRadioButton   *m_project;
    QRadioButton   *m_url;
    QLineEdit      *m_target;
};

#endif // WORKDIALOG_H
