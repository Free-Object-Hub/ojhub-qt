#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QTabWidget>
#include <QListView>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QAction>
#include <QModelIndex>
#include <QUrl>
#include <QList>
#include <QHash>

#include "api/apiclient.h"
#include "api/searchresult.h"
#include "resultsmodel.h"

class AlarmsDialog;
class SubscriptionsDialog;

class TagPicker;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Tab order matches the QTabWidget.
    enum Tab { TabProjects = 0, TabVacancies = 1, TabNews = 2 };

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onSearchClicked();
    void onShowMore();
    void onSearchFinished(const SearchResultList &results, bool hasMore, int page);
    void onSearchError(const QString &error);

    void onResultActivated(const QModelIndex &index);
    void onDetailFinished(const DetailData &d);
    void onDetailError(const QString &error);
    void onDetailBack();
    void onDetailAnchor(const QUrl &url);

    void onTabChanged(int index);

    void onLogin();
    void onRegister();
    void onLogout();
    void onLoginFinished(bool ok, const QString &message);
    void onRegisterFinished(bool ok, const QString &message);

    void onOpenProfile();
    void onProfileFinished(const PublicProfile &profile);
    void onProfileError(const QString &error);

    void onCreateProject();
    void onEditDetail();
    void onAddVacancy();
    void onDeleteVacancy();
    void onApplyToggle();
    void onProjectFormLoaded(const ProjectForm &form);
    void onVacancyFormLoaded(const VacancyForm &form);
    void onActionFinished(int action, bool ok, const QString &message, int id);

    void onVotesChanged(int typeCode, int id, int likes, int dislikes);
    void onCommentsUpdated(int contentType, int id, const SearchResultList &comments, bool hasMore);
    void onCommentEdited(int commentId, const QString &text);
    void onCommentDeleted(int commentId);

    void onOpenMyProfile();
    void onProfileEdit();
    void onOpenSubs();
    void onOpenAlarms();
    void onSubscribeClicked();
    void onUnsubscribeClicked();
    void onSubscriptionChanged(int id, bool subscribed);
    void onSubsLoaded(const QVariantList &items, int page, bool hasMore, int probeId);
    void onSubsMore(int page);
    void onAlarmsMore(int page);
    void onVacancyOwnership(int id, bool owned);
    void onNewsOwnership(int id, bool owned);
    void onAlarmsLoaded(const QVariantList &items, int page, bool hasMore);
    void onAlarmLoaded(const QVariantMap &alarm);
    void onAlarmSelect(int id);
    void onAlarmDelete(int id);
    void onAlarmDeleted(int id);
    void onProfileFieldSaved(int field, const QString &value);

private:
    // What the list shows. Vacancies and news load on tab open; projects wait for a search.
    enum Mode { ModeProjects, ModeVacancies, ModeNews };

    void setupUi();
    void setBusy(bool busy);
    void updateAuthUi();
    void clearResults();
    void startTabLoad();
    void openDetail(int kind, int id, int page, int extra = 0);
    void openComments(int contentType, int id, int page);
    void updateActions(const DetailData &d);
    void reopenTopVacancy();
    void onVote(int typeCode, int id, bool dislike);
    void onWriteComment(int contentType, int id);
    void onEditComment(int commentId);
    void onDeleteComment(int commentId);
    void onNewsNew(int projectId);
    void onNewsEdit();
    void onNewsDelete();
    void onAddWork();
    void onAccountChanged();
    void onEditWork(int workId);
    void onDeleteWork(int workId);
    void rebuildTagPicker();
    bool requireLogin();
    void showTopDetail();
    QString renderDetail(const DetailData &d) const;

    ApiClient      *m_api;
    ResultsModel   *m_model;
    ResultDelegate *m_delegate;

    Mode m_mode;

    // Search state, kept so "show more" requests the next page of the same query
    QString    m_query;
    int        m_channel;
    int        m_method;
    QList<int> m_tags;
    QList<int> m_os;
    int        m_nextPage;

    QTabWidget   *m_tabs;
    QWidget      *m_searchPanel;
    QLineEdit    *m_searchEdit;
    QWidget      *m_tagHolder;
    TagPicker    *m_tagPicker;
    QLabel       *m_tagHint;
    QComboBox    *m_channelCombo;
    QComboBox    *m_methodCombo;
    QWidget      *m_channelRow;
    QPushButton  *m_searchBtn;
    QProgressBar *m_progress;
    QLabel       *m_statusLabel;

    // Frame: page 0 = list, page 1 = detail. Back is history-based, see m_history.
    QStackedWidget *m_stack;
    QWidget        *m_listPage;
    QListView      *m_resultsView;
    QPushButton    *m_moreBtn;
    QWidget        *m_detailPage;
    QPushButton    *m_backBtn;
    QLabel         *m_detailTitle;
    QTextBrowser   *m_detailBody;
    QList<DetailData> m_history;

    QAction *m_loginAction;
    QAction *m_registerAction;
    QAction *m_logoutAction;
    QLabel  *m_authStatus;

    // Actions row above the detail body
    QWidget     *m_actionsBox;
    QPushButton *m_editBtn;
    QPushButton *m_addVacBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_applyBtn;

    // Channel of the project being created, used to open the user's list after save
    int m_pendingChannel;

    // Profile save in progress: nickname, resume, socials. Reloads the profile when all are saved
    int m_pendingProfile;

    // Open notifications dialog, while it is shown (exec)
    AlarmsDialog *m_alarmsDlg;
    SubscriptionsDialog *m_subsDlg;

    // Owner / subscription state from probes. Key: object id. Value: 1 yes, 0 no, -1 asked
    QHash<int, int> m_vacOwner;
    QHash<int, int> m_newsOwner;
    QHash<int, int> m_subState;

    void ensureProbes();

    QPushButton *m_subBtn;
    QPushButton *m_unsubBtn;
};

#endif // MAINWINDOW_H
