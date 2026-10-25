#ifndef NEWSDIALOG_H
#define NEWSDIALOG_H

#include <QDialog>

#include "api/forms.h"

class QLineEdit;
class QPlainTextEdit;
class QLabel;

// Create or edit a news post of a project.
class NewsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewsDialog(const NewsForm &initial, QWidget *parent = 0);
    NewsForm form() const;
    QString file() const { return m_file; }

private slots:
    void pickFile();
    void tryAccept();

private:
    NewsForm        m_base;
    QLineEdit      *m_title;
    QPlainTextEdit *m_text;
    QLabel         *m_picked;
    QString         m_file;
};

#endif // NEWSDIALOG_H
