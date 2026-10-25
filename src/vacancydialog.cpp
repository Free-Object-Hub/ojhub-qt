
#include "lang.h"
#include "vacancydialog.h"
#include "tagpicker.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QLineEdit>
#include <QPlainTextEdit>

VacancyDialog::VacancyDialog(const VacancyForm &initial, QWidget *parent)
    : QDialog(parent)
    , m_base(initial)
{
    setWindowTitle(initial.id > 0 ? QString::fromUtf8("Edit vacancy")     // LANGXXX
                                  : QString::fromUtf8("New vacancy"));           // LANGXXX
    resize(480, 460);

    QVBoxLayout *root = new QVBoxLayout(this);
    QFormLayout *fl = new QFormLayout();

    m_title = new QLineEdit(initial.title, this);
    fl->addRow(L("findName"), m_title); 

    m_short = new QLineEdit(initial.shortText, this);
    m_short->setMaxLength(200);
    fl->addRow(L("campInput02a"), m_short); 

    m_text = new QPlainTextEdit(initial.text, this);
    fl->addRow(QString::fromUtf8("Description"), m_text);   // LANGXXX
    root->addLayout(fl, 1);

    m_tags = new TagPicker(0, true, initial.tags, QList<int>(), this);
    root->addWidget(m_tags);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, SIGNAL(accepted()), this, SLOT(tryAccept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));
    root->addWidget(box);
}

VacancyForm VacancyDialog::form() const
{
    VacancyForm f = m_base;
    f.title = m_title->text().trimmed();
    f.text = m_text->toPlainText().trimmed();
    f.shortText = m_short->text().trimmed();
    f.tags = m_tags->tags();
    return f;
}

void VacancyDialog::tryAccept()
{
    if (m_title->text().trimmed().isEmpty() || m_text->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("Vacancy"),   // LANGXXX
                             QString::fromUtf8("Fill in the title and description."));   // LANGXXX
        return;
    }
    accept();
}
