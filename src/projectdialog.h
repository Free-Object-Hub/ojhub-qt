
#ifndef PROJECTDIALOG_H
#define PROJECTDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>

#include "api/forms.h"

class QLineEdit;
class QPlainTextEdit;
class QTabWidget;
class QLabel;
class TagPicker;

// Create or edit a camp, show, pere or tele.
class ProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectDialog(const ProjectForm &initial, QWidget *parent = 0);

    ProjectForm form() const;
    QString imgFile() const { return m_imgFile; }
    QString banFile() const { return m_banFile; }

private slots:
    void pickImg();
    void pickBan();
    void tryAccept();

private:
    int m_channel;
    int m_id;

    QLineEdit   *m_title;
    QTabWidget  *m_langs;
    QMap<QString, QPlainTextEdit *> m_desc;
    QMap<QString, QLineEdit *>      m_short;
    QPlainTextEdit *m_links;
    QLineEdit   *m_img;
    QLineEdit   *m_ban;
    QLabel      *m_imgPicked;
    QLabel      *m_banPicked;
    TagPicker   *m_picker;

    QString m_imgFile;
    QString m_banFile;
};

#endif // PROJECTDIALOG_H
