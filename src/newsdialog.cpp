#include "newsdialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>

NewsDialog::NewsDialog(const NewsForm &initial, QWidget *parent)
    : QDialog(parent)
    , m_base(initial)
{
    setWindowTitle(initial.id > 0 ? QString::fromUtf8("Edit news")    // LANGXXX
                                  : QString::fromUtf8("New news"));          // LANGXXX
    resize(480, 420);

    QVBoxLayout *root = new QVBoxLayout(this);
    QFormLayout *fl = new QFormLayout();

    m_title = new QLineEdit(initial.title, this);
    fl->addRow(QString::fromUtf8("Title"), m_title);   // LANGXXX

    m_text = new QPlainTextEdit(initial.text, this);
    fl->addRow(QString::fromUtf8("Text"), m_text);   // LANGXXX

    // Picture is only taken on create: the server ignores files on edit.
    QPushButton *fileBtn = new QPushButton(QString::fromUtf8("Picture..."), this);   // LANGXXX
    connect(fileBtn, SIGNAL(clicked()), this, SLOT(pickFile()));
    m_picked = new QLabel(this);
    QHBoxLayout *fileRow = new QHBoxLayout();
    fileRow->addWidget(fileBtn);
    fileRow->addWidget(m_picked, 1);
    fl->addRow(QString::fromUtf8("Picture"), fileRow);   // LANGXXX
    if (initial.id > 0)
        fileBtn->setEnabled(false);

    root->addLayout(fl, 1);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, SIGNAL(accepted()), this, SLOT(tryAccept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));
    root->addWidget(box);
}

NewsForm NewsDialog::form() const
{
    NewsForm f = m_base;
    f.title = m_title->text().trimmed();
    f.text = m_text->toPlainText().trimmed();
    return f;
}

void NewsDialog::pickFile()
{
    const QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("Picture"), QString(),   // LANGXXX
                                                      QString::fromUtf8("Pictures (*.png *.jpg *.jpeg)"));   // LANGXXX
    if (path.isEmpty())
        return;
    m_file = path;
    m_picked->setText(QFileInfo(path).fileName());
}

void NewsDialog::tryAccept()
{
    if (m_title->text().trimmed().isEmpty() || m_text->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("News"),   // LANGXXX
                             QString::fromUtf8("Fill in the title and text."));   // LANGXXX
        return;
    }
    accept();
}
