
#ifndef SOCIALDIALOGS_H
#define SOCIALDIALOGS_H

#include <QDialog>
#include <QVariantList>
#include <QVariantMap>

class QLineEdit;
class QPlainTextEdit;
class QListWidget;
class QListWidgetItem;
class QTextBrowser;
class QPushButton;

// Edit nickname, resume and socials of the logged-in user.
class ProfileEditDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileEditDialog(const QString &nick, const QString &resume, const QString &socials, QWidget *parent = 0);

    QString nick() const;
    QString resume() const;     // newlines encoded as the two characters \n, as the server stores them
    QString socials() const;

private slots:
    void tryAccept();

private:
    QLineEdit      *m_nick;
    QPlainTextEdit *m_resume;
    QPlainTextEdit *m_socials;
};

// Projects the user is subscribed to, loaded page by page. Picking one sets chosenId().
class SubscriptionsDialog : public QDialog
{
    Q_OBJECT

public:
    SubscriptionsDialog(const QVariantList &items, int page, bool hasMore, QWidget *parent = 0);

    // Adds the rows of the next page
    void appendItems(const QVariantList &items, int page, bool hasMore);
    int chosenId() const { return m_chosen; }

signals:
    void moreRequested(int page);

private slots:
    void onActivated(QListWidgetItem *item);
    void onOpenClicked();
    void onMoreClicked();

private:
    void addRows(const QVariantList &items);

    QListWidget *m_list;
    QPushButton *m_moreBtn;
    int m_chosen;
    int m_nextPage;
};

// Notifications: list on the left, text on the right. Selecting an item asks for its text.
class AlarmsDialog : public QDialog
{
    Q_OBJECT

public:
    AlarmsDialog(const QVariantList &items, int page, bool hasMore, QWidget *parent = 0);

    void appendItems(const QVariantList &items, int page, bool hasMore);
    void showAlarm(const QVariantMap &alarm);
    void removeAlarm(int id);

signals:
    void selectRequested(int id);
    void deleteRequested(int id);
    void moreRequested(int page);

private slots:
    void onItemClicked(QListWidgetItem *item);
    void onDeleteClicked();
    void onMoreClicked();

private:
    void addRows(const QVariantList &items);
    void updateTitle();
    QListWidgetItem *itemById(int id) const;

    QListWidget  *m_list;
    QTextBrowser *m_body;
    QPushButton  *m_moreBtn;
    int m_nextPage;
};

#endif // SOCIALDIALOGS_H
