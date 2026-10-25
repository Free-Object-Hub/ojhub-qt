#include "projectdialog.h"
#include "lang.h"
#include "tagpicker.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTabWidget>

namespace {

QString channelTitle(int ch)
{
    // LANGXXX
    switch (ch) {
    case 1: return QString::fromUtf8("show");
    case 2: return QString::fromUtf8("pere");
    case 3: return QString::fromUtf8("tele");
    default: return QString::fromUtf8("camp");
    }
}


} // namespace

ProjectDialog::ProjectDialog(const ProjectForm &initial, QWidget *parent)
    : QDialog(parent)
    , m_channel(initial.channel)
    , m_id(initial.id)
{
    setWindowTitle((m_id > 0 ? QString::fromUtf8("Edit: ") : QString::fromUtf8("New: "))
                   + channelTitle(m_channel));   // LANGXXX
    resize(540, 600);

    QVBoxLayout *root = new QVBoxLayout(this);

    QFormLayout *top = new QFormLayout();
    m_title = new QLineEdit(initial.title, this);
    top->addRow(L("findName"), m_title); 
    root->addLayout(top);

    // One tab per language: description and short line. The server needs at least one description.
    m_langs = new QTabWidget(this);
    const QStringList langs = QStringList() << QStringLiteral("RU") << QStringLiteral("EN") << QStringLiteral("UA");
    for (int i = 0; i < langs.size(); ++i) {
        const QString lang = langs.at(i);
        QWidget *page = new QWidget(m_langs);
        QFormLayout *fl = new QFormLayout(page);
        QLineEdit *shortEdit = new QLineEdit(initial.shortText.value(lang), page);
        shortEdit->setMaxLength(200);
        QPlainTextEdit *desc = new QPlainTextEdit(page);
        desc->setPlainText(initial.description.value(lang));
        fl->addRow(L("campInput02a"), shortEdit); 
        fl->addRow(QString::fromUtf8("Description"), desc);                 // LANGXXX
        m_langs->addTab(page, lang);
        m_short.insert(lang, shortEdit);
        m_desc.insert(lang, desc);
    }
    root->addWidget(m_langs, 1);

    QFormLayout *bottom = new QFormLayout();

    m_links = new QPlainTextEdit(this);
    m_links->setPlaceholderText(QString::fromUtf8("Title|link, one per line"));   // LANGXXX
    QStringList lk;
    for (int i = 0; i < initial.links.size(); ++i) {
        const QPair<QString, QString> &l = initial.links.at(i);
        lk << (l.first.isEmpty() ? l.second : l.first + QLatin1Char('|') + l.second);
    }
    m_links->setPlainText(lk.join(QStringLiteral("\n")));
    bottom->addRow(QString::fromUtf8("Links"), m_links);   // LANGXXX

    // Image: url or a picked file (file wins).
    m_img = new QLineEdit(initial.img, this);
    QPushButton *imgBtn = new QPushButton(QString::fromUtf8("File..."), this);   // LANGXXX
    connect(imgBtn, SIGNAL(clicked()), this, SLOT(pickImg()));
    m_imgPicked = new QLabel(this);
    QHBoxLayout *imgRow = new QHBoxLayout();
    imgRow->addWidget(m_img, 1);
    imgRow->addWidget(imgBtn);
    imgRow->addWidget(m_imgPicked);
    bottom->addRow(QString::fromUtf8("Picture"), imgRow);   // LANGXXX

    m_ban = new QLineEdit(initial.ban, this);
    QPushButton *banBtn = new QPushButton(QString::fromUtf8("File..."), this);   // LANGXXX
    connect(banBtn, SIGNAL(clicked()), this, SLOT(pickBan()));
    m_banPicked = new QLabel(this);
    QHBoxLayout *banRow = new QHBoxLayout();
    banRow->addWidget(m_ban, 1);
    banRow->addWidget(banBtn);
    banRow->addWidget(m_banPicked);
    bottom->addRow(QString::fromUtf8("Banner"), banRow);   // LANGXXX

    root->addLayout(bottom);

    m_picker = new TagPicker(m_channel, false, initial.tags, initial.os, this);
    root->addWidget(m_picker);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, SIGNAL(accepted()), this, SLOT(tryAccept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));
    root->addWidget(box);
}

ProjectForm ProjectDialog::form() const
{
    ProjectForm f;
    f.channel = m_channel;
    f.id = m_id;
    f.title = m_title->text().trimmed();

    for (QMap<QString, QPlainTextEdit *>::const_iterator it = m_desc.constBegin(); it != m_desc.constEnd(); ++it)
        f.description.insert(it.key(), it.value()->toPlainText().trimmed());
    for (QMap<QString, QLineEdit *>::const_iterator it = m_short.constBegin(); it != m_short.constEnd(); ++it)
        f.shortText.insert(it.key(), it.value()->text().trimmed());

    const QStringList lines = m_links->toPlainText().split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty())
            continue;
        const int bar = line.indexOf(QLatin1Char('|'));
        if (bar < 0)
            f.links.append(qMakePair(QString(), line));
        else
            f.links.append(qMakePair(line.left(bar).trimmed(), line.mid(bar + 1).trimmed()));
    }

    f.img = m_img->text().trimmed();
    f.ban = m_ban->text().trimmed();
    f.tags = m_picker->tags();
    f.os = m_picker->os();
    return f;
}

void ProjectDialog::pickImg()
{
    const QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("Picture"), QString(),   // LANGXXX
                                                      QString::fromUtf8("Pictures (*.png *.jpg *.jpeg)"));   // LANGXXX
    if (path.isEmpty())
        return;
    m_imgFile = path;
    m_imgPicked->setText(QFileInfo(path).fileName());
}

void ProjectDialog::pickBan()
{
    const QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("Banner"), QString(),   // LANGXXX
                                                      QString::fromUtf8("Pictures (*.png *.jpg *.jpeg)"));   // LANGXXX
    if (path.isEmpty())
        return;
    m_banFile = path;
    m_banPicked->setText(QFileInfo(path).fileName());
}

void ProjectDialog::tryAccept()
{
    if (m_title->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("Project"), QString::fromUtf8("Enter a title."));   // LANGXXX
        return;
    }
    bool hasDesc = false;
    for (QMap<QString, QPlainTextEdit *>::const_iterator it = m_desc.constBegin(); it != m_desc.constEnd(); ++it)
        if (!it.value()->toPlainText().trimmed().isEmpty())
            hasDesc = true;
    if (!hasDesc) {
        QMessageBox::warning(this, QString::fromUtf8("Project"), QString::fromUtf8("A description is required in at least one language."));   // LANGXXX
        return;
    }
    accept();
}
