#include "lang.h"
#include "socialdialogs.h"

#include <QDateTime>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QLabel>
#include <QVBoxLayout>

namespace {

// Stored text keeps newlines as the two characters \n; the editors want real newlines.
QString storedToEdit(const QString &s)
{
    QString out = s;
    out.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    return out;
}

QString editToStored(const QString &s)
{
    QString out = s;
    out.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    out.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    return out;
}

} // namespace

// ---------- ProfileEditDialog ----------

ProfileEditDialog::ProfileEditDialog(const QString &nick, const QString &resume, const QString &socials, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("Edit profile"));   // LANGXXX
    resize(460, 420);

    QVBoxLayout *root = new QVBoxLayout(this);
    QFormLayout *fl = new QFormLayout();
    m_nick = new QLineEdit(nick, this);
    m_resume = new QPlainTextEdit(storedToEdit(resume), this);
    m_socials = new QPlainTextEdit(storedToEdit(socials), this);
    fl->addRow(L("profName"), m_nick);   
    fl->addRow(L("profResume"), m_resume);  
    fl->addRow(L("profSocials"), m_socials);
    root->addLayout(fl, 1);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, SIGNAL(accepted()), this, SLOT(tryAccept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));
    root->addWidget(box);
}

QString ProfileEditDialog::nick() const
{
    return m_nick->text().trimmed();
}

QString ProfileEditDialog::resume() const
{
    return editToStored(m_resume->toPlainText().trimmed());
}

QString ProfileEditDialog::socials() const
{
    return editToStored(m_socials->toPlainText().trimmed());
}

void ProfileEditDialog::tryAccept()
{
    if (m_nick->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), QString::fromUtf8("Nickname cannot be empty."));   // LANGXXX
        return;
    }
    accept();
}

// ---------- SubscriptionsDialog ----------

SubscriptionsDialog::SubscriptionsDialog(const QVariantList &items, int page, bool hasMore, QWidget *parent)
    : QDialog(parent)
    , m_chosen(0)
    , m_nextPage(page + 1)
{
    setWindowTitle(L("gdpsSubs")); 
    resize(420, 380);

    QVBoxLayout *root = new QVBoxLayout(this);
    m_list = new QListWidget(this);
    addRows(items);
    if (items.isEmpty())
        m_list->addItem(QString::fromUtf8("You are not subscribed to anything"));   // LANGXXX
    connect(m_list, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(onActivated(QListWidgetItem*)));
    root->addWidget(m_list, 1);

    QHBoxLayout *row = new QHBoxLayout();
    m_moreBtn = new QPushButton(QString::fromUtf8("Show more"), this);   // LANGXXX
    m_moreBtn->setVisible(hasMore);
    QPushButton *openBtn = new QPushButton(L("openGdps"), this); 
    QPushButton *closeBtn = new QPushButton(L("WINDOW-close"), this);
    connect(m_moreBtn, SIGNAL(clicked()), this, SLOT(onMoreClicked()));
    connect(openBtn, SIGNAL(clicked()), this, SLOT(onOpenClicked()));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(reject()));
    row->addWidget(m_moreBtn);
    row->addStretch(1);
    row->addWidget(openBtn);
    row->addWidget(closeBtn);
    root->addLayout(row);
}

void SubscriptionsDialog::addRows(const QVariantList &items)
{
    for (int i = 0; i < items.size(); ++i) {
        const QVariantMap m = items.at(i).toMap();
        QListWidgetItem *it = new QListWidgetItem(m.value(QStringLiteral("title")).toString(), m_list);
        it->setData(Qt::UserRole, m.value(QStringLiteral("ID")).toInt());
    }
}

void SubscriptionsDialog::appendItems(const QVariantList &items, int page, bool hasMore)
{
    addRows(items);
    m_nextPage = page + 1;
    m_moreBtn->setVisible(hasMore);
}

void SubscriptionsDialog::onMoreClicked()
{
    m_moreBtn->setEnabled(false);   // re-enabled when the next page arrives
    emit moreRequested(m_nextPage);
}

void SubscriptionsDialog::onActivated(QListWidgetItem *item)
{
    const int id = item ? item->data(Qt::UserRole).toInt() : 0;
    if (id <= 0)
        return;
    m_chosen = id;
    accept();
}

void SubscriptionsDialog::onOpenClicked()
{
    onActivated(m_list->currentItem());
}

// ---------- AlarmsDialog ----------

AlarmsDialog::AlarmsDialog(const QVariantList &items, int page, bool hasMore, QWidget *parent)
    : QDialog(parent)
    , m_nextPage(page + 1)
{
    resize(620, 440);

    QVBoxLayout *root = new QVBoxLayout(this);
    QHBoxLayout *main = new QHBoxLayout();

    m_list = new QListWidget(this);
    m_list->setMaximumWidth(230);
    addRows(items);
    if (items.isEmpty())
        m_list->addItem(QString::fromUtf8("No notifications"));   // LANGXXX
    connect(m_list, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onItemClicked(QListWidgetItem*)));
    main->addWidget(m_list);

    m_body = new QTextBrowser(this);
    main->addWidget(m_body, 1);
    root->addLayout(main, 1);

    QHBoxLayout *row = new QHBoxLayout();
    m_moreBtn = new QPushButton(QString::fromUtf8("Show more"), this);   // LANGXXX
    m_moreBtn->setVisible(hasMore);
    QPushButton *delBtn = new QPushButton(L("delete"), this); 
    QPushButton *closeBtn = new QPushButton(L("WINDOW-close"), this);
    connect(m_moreBtn, SIGNAL(clicked()), this, SLOT(onMoreClicked()));
    connect(delBtn, SIGNAL(clicked()), this, SLOT(onDeleteClicked()));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(accept()));
    row->addWidget(m_moreBtn);
    row->addStretch(1);
    row->addWidget(delBtn);
    row->addWidget(closeBtn);
    root->addLayout(row);

    updateTitle();
}

void AlarmsDialog::addRows(const QVariantList &items)
{
    for (int i = 0; i < items.size(); ++i) {
        const QVariantMap m = items.at(i).toMap();
        QListWidgetItem *it = new QListWidgetItem(m.value(QStringLiteral("title")).toString(), m_list);
        it->setData(Qt::UserRole, m.value(QStringLiteral("ID")).toInt());
        QFont f = it->font();
        f.setBold(m.value(QStringLiteral("public")).toInt() == 1);   // 1 = unread
        it->setFont(f);
    }
}

void AlarmsDialog::appendItems(const QVariantList &items, int page, bool hasMore)
{
    addRows(items);
    m_nextPage = page + 1;
    m_moreBtn->setVisible(hasMore);
    m_moreBtn->setEnabled(true);
    updateTitle();
}

// Counts unread rows among the pages loaded so far.
void AlarmsDialog::updateTitle()
{
    int unread = 0;
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->font().bold())
            ++unread;
    }
    setWindowTitle(QString::fromUtf8("Notifications (unread: %1)").arg(unread));   // LANGXXX
}

void AlarmsDialog::onMoreClicked()
{
    m_moreBtn->setEnabled(false);
    emit moreRequested(m_nextPage);
}

void AlarmsDialog::onItemClicked(QListWidgetItem *item)
{
    const int id = item ? item->data(Qt::UserRole).toInt() : 0;
    if (id > 0)
        emit selectRequested(id);
}

void AlarmsDialog::onDeleteClicked()
{
    QListWidgetItem *it = m_list->currentItem();
    const int id = it ? it->data(Qt::UserRole).toInt() : 0;
    if (id <= 0)
        return;
    if (QMessageBox::question(this, windowTitle(), QString::fromUtf8("Delete this notification?"),   // LANGXXX
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    emit deleteRequested(id);
}

void AlarmsDialog::showAlarm(const QVariantMap &a)
{
    const QString title = a.value(QStringLiteral("title")).toString().toHtmlEscaped();
    QString text = a.value(QStringLiteral("text")).toString();
    text.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    const QString admin = a.value(QStringLiteral("adminName")).toString().toHtmlEscaped();
    const qint64 date = a.value(QStringLiteral("date")).toLongLong();
    const QString when = date > 0 ? QDateTime::fromMSecsSinceEpoch(date * 1000).toString(QStringLiteral("dd.MM.yyyy HH:mm"))
                                  : QString();

    QString html = QStringLiteral("<h3>%1</h3><p>%2</p>")
        .arg(title)
        .arg(text.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>")));
    html += QStringLiteral("<p><small>%1 %2</small></p>").arg(admin).arg(when.toHtmlEscaped());
    m_body->setHtml(html);

    // Opening marks it read on the server; mirror that in the list
    const int id = a.value(QStringLiteral("ID")).toInt();
    if (QListWidgetItem *it = itemById(id)) {
        QFont f = it->font();
        f.setBold(false);
        it->setFont(f);
        updateTitle();
    }
}

void AlarmsDialog::removeAlarm(int id)
{
    if (QListWidgetItem *it = itemById(id))
        delete it;
    m_body->clear();
    updateTitle();
}

QListWidgetItem *AlarmsDialog::itemById(int id) const
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *it = m_list->item(i);
        if (it->data(Qt::UserRole).toInt() == id)
            return it;
    }
    return 0;
}
