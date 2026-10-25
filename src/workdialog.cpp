
#include "workdialog.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QVBoxLayout>

WorkDialog::WorkDialog(const WorkForm &initial, QWidget *parent)
    : QDialog(parent)
    , m_base(initial)
{
    setWindowTitle(initial.id > 0 ? QString::fromUtf8("Edit portfolio item")
                                  : QString::fromUtf8("New portfolio item"));
    resize(480, 380);

    QVBoxLayout *root = new QVBoxLayout(this);
    QFormLayout *fl = new QFormLayout();

    m_title = new QLineEdit(initial.title, this);
    m_title->setMaxLength(100);
    fl->addRow(QString::fromUtf8("Title"), m_title);

    m_text = new QPlainTextEdit(initial.text, this);
    fl->addRow(QString::fromUtf8("Text"), m_text);
    root->addLayout(fl, 1);

    // Link: a project from the catalog (its id) or an external url
    m_project = new QRadioButton(QString::fromUtf8("Project from the catalog"), this);
    m_url = new QRadioButton(QString::fromUtf8("External link"), this);
    QButtonGroup *group = new QButtonGroup(this);
    group->addButton(m_project);
    group->addButton(m_url);
    const bool isUrl = initial.linkType == 1;
    m_project->setChecked(!isUrl);
    m_url->setChecked(isUrl);
    connect(m_project, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(m_url, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));

    QHBoxLayout *typeRow = new QHBoxLayout();
    typeRow->addWidget(m_project);
    typeRow->addWidget(m_url);
    root->addLayout(typeRow);

    m_target = new QLineEdit(initial.link, this);
    m_target->setMaxLength(255);
    fl = new QFormLayout();
    fl->addRow(QString::fromUtf8("Project ID or URL"), m_target);
    root->addLayout(fl);
    typeChanged();

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, SIGNAL(accepted()), this, SLOT(tryAccept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));
    root->addWidget(box);
}

void WorkDialog::typeChanged()
{
    m_target->setPlaceholderText(m_url->isChecked() ? QStringLiteral("https://...")
                                                    : QStringLiteral("12"));
}

WorkForm WorkDialog::form() const
{
    WorkForm f = m_base;
    f.title = m_title->text().trimmed();
    f.text = m_text->toPlainText().trimmed();
    f.linkType = m_url->isChecked() ? 1 : 0;
    f.link = m_target->text().trimmed();
    return f;
}

void WorkDialog::tryAccept()
{
    const WorkForm f = form();
    if (f.title.isEmpty() || f.title.length() > 100) {
        QMessageBox::warning(this, windowTitle(), QString::fromUtf8("The title is required, at most 100 characters."));
        return;
    }
    if (f.text.isEmpty() || f.text.length() > 2000) {
        QMessageBox::warning(this, windowTitle(), QString::fromUtf8("The text is required, at most 2000 characters."));
        return;
    }
    if (f.link.isEmpty() || f.link.length() > 255) {
        QMessageBox::warning(this, windowTitle(), QString::fromUtf8("The link is required, at most 255 characters."));
        return;
    }
    if (f.linkType == 0) {
        bool ok = false;
        f.link.toInt(&ok);
        if (!ok || f.link.toInt() <= 0) {
            QMessageBox::warning(this, windowTitle(), QString::fromUtf8("The project ID must be a positive number."));
            return;
        }
    } else if (!f.link.startsWith(QLatin1String("http://")) && !f.link.startsWith(QLatin1String("https://"))) {
        QMessageBox::warning(this, windowTitle(), QString::fromUtf8("The link must start with http:// or https://."));
        return;
    }
    accept();
}
