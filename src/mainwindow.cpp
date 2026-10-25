#include "mainwindow.h"
#include "tagpicker.h"
#include "tagnames.h"
#include "profileview.h"
#include "logindialog.h"
#include "registerdialog.h"
#include "projectdialog.h"
#include "vacancydialog.h"
#include "newsdialog.h"
#include "socialdialogs.h"
#include "workdialog.h"
#include "lang.h"
#include "version.h"

#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_api(new ApiClient(this))
    , m_model(new ResultsModel(this))
    , m_delegate(new ResultDelegate(this))
    , m_mode(ModeProjects)
    , m_channel(kChannelAll)
    , m_method(0)
    , m_nextPage(0)
    , m_pendingChannel(0)
    , m_pendingProfile(0)
    , m_alarmsDlg(0)
    , m_subsDlg(0)
{
    m_api->ensureDeviceToken();
    m_api->restoreSession();   // after construction, so the base URL is already final
    setupUi();
    updateAuthUi();

    connect(m_api, SIGNAL(searchFinished(SearchResultList,bool,int)),
            this, SLOT(onSearchFinished(SearchResultList,bool,int)));
    connect(m_api, SIGNAL(searchError(QString)),
            this, SLOT(onSearchError(QString)));
    connect(m_api, SIGNAL(networkError(QString)),
            this, SLOT(onSearchError(QString)));
    connect(m_api, SIGNAL(detailFinished(DetailData)),
            this, SLOT(onDetailFinished(DetailData)));
    connect(m_api, SIGNAL(detailError(QString)),
            this, SLOT(onDetailError(QString)));
    connect(m_api, SIGNAL(loginFinished(bool,QString)),
            this, SLOT(onLoginFinished(bool,QString)));
    connect(m_api, SIGNAL(registerFinished(bool,QString)),
            this, SLOT(onRegisterFinished(bool,QString)));
    connect(m_api, SIGNAL(profileFinished(PublicProfile)),
            this, SLOT(onProfileFinished(PublicProfile)));
    connect(m_api, SIGNAL(profileError(QString)),
            this, SLOT(onProfileError(QString)));
    connect(m_api, SIGNAL(projectFormLoaded(ProjectForm)),
            this, SLOT(onProjectFormLoaded(ProjectForm)));
    connect(m_api, SIGNAL(vacancyFormLoaded(VacancyForm)),
            this, SLOT(onVacancyFormLoaded(VacancyForm)));
    connect(m_api, SIGNAL(actionFinished(int,bool,QString,int)),
            this, SLOT(onActionFinished(int,bool,QString,int)));
    connect(m_api, SIGNAL(votesChanged(int,int,int,int)),
            this, SLOT(onVotesChanged(int,int,int,int)));
    connect(m_api, SIGNAL(commentsUpdated(int,int,SearchResultList,bool)),
            this, SLOT(onCommentsUpdated(int,int,SearchResultList,bool)));
    connect(m_api, SIGNAL(commentEdited(int,QString)),
            this, SLOT(onCommentEdited(int,QString)));
    connect(m_api, SIGNAL(commentDeleted(int)),
            this, SLOT(onCommentDeleted(int)));
    connect(m_api, SIGNAL(profileFieldSaved(int,QString)),
            this, SLOT(onProfileFieldSaved(int,QString)));
    connect(m_api, SIGNAL(subscriptionChanged(int,bool)),
            this, SLOT(onSubscriptionChanged(int,bool)));
    connect(m_api, SIGNAL(subscriptionsLoaded(QVariantList,int,bool,int)),
            this, SLOT(onSubsLoaded(QVariantList,int,bool,int)));
    connect(m_api, SIGNAL(alarmsLoaded(QVariantList,int,bool)),
            this, SLOT(onAlarmsLoaded(QVariantList,int,bool)));
    connect(m_api, SIGNAL(vacancyOwnership(int,bool)),
            this, SLOT(onVacancyOwnership(int,bool)));
    connect(m_api, SIGNAL(newsOwnership(int,bool)),
            this, SLOT(onNewsOwnership(int,bool)));
    connect(m_api, SIGNAL(accountChanged()),
            this, SLOT(onAccountChanged()));
    connect(m_api, SIGNAL(alarmLoaded(QVariantMap)),
            this, SLOT(onAlarmLoaded(QVariantMap)));
    connect(m_api, SIGNAL(alarmDeleted(int)),
            this, SLOT(onAlarmDeleted(int)));

    setWindowTitle(L("ojhubname") + QStringLiteral(" " OH_VERSION));
    resize(780, 580);
    statusBar()->showMessage(L("ready"));

    // Connected last: addTab and the first tab selection must not reach half-built widgets.
    connect(m_tabs, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    m_tabs = new QTabWidget(central);
    m_tabs->addTab(new QWidget(), L("projects"));
    m_tabs->addTab(new QWidget(), L("vacancies"));
    m_tabs->addTab(new QWidget(), L("newsTab"));
    mainLayout->addWidget(m_tabs);

    // Search panel: hidden on the news tab, which has no search.
    m_searchPanel = new QWidget(central);
    QVBoxLayout *panelLayout = new QVBoxLayout(m_searchPanel);
    panelLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *searchRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(m_searchPanel);
    m_searchEdit->setPlaceholderText(L("findName"));
    connect(m_searchEdit, SIGNAL(returnPressed()), this, SLOT(onSearchClicked()));
    m_searchBtn = new QPushButton(L("finder-name"), m_searchPanel);
    connect(m_searchBtn, SIGNAL(clicked()), this, SLOT(onSearchClicked()));
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(m_searchBtn);
    panelLayout->addLayout(searchRow);

    // Channel row: projects only (vacancies have a single channel).
    m_channelRow = new QWidget(m_searchPanel);
    QHBoxLayout *chLayout = new QHBoxLayout(m_channelRow);
    chLayout->setContentsMargins(0, 0, 0, 0);
    chLayout->addWidget(new QLabel(L("finder-channel"), m_channelRow));
    m_channelCombo = new QComboBox(m_channelRow);
    m_channelCombo->addItem(L("channelAll"), kChannelAll);
    m_channelCombo->addItem(L("searchCamps"), 0);
    m_channelCombo->addItem(L("searchShows"), 1);
    m_channelCombo->addItem(L("searchPeres"), 2);
    m_channelCombo->addItem(L("searchTeles"), 3);
    chLayout->addWidget(m_channelCombo, 1);
    panelLayout->addWidget(m_channelRow);

    QHBoxLayout *methodRow = new QHBoxLayout();
    methodRow->addWidget(new QLabel(L("otherSort"), m_searchPanel));
    m_methodCombo = new QComboBox(m_searchPanel);
    m_methodCombo->addItem(L("search4"), 0);
    m_methodCombo->addItem(L("mostLike"), 1);
    m_methodCombo->addItem(L("mostDisl"), 2);
    m_methodCombo->addItem(L("search1"), 3);
    methodRow->addWidget(m_methodCombo, 1);
    panelLayout->addLayout(methodRow);

    // Tag filter: checkboxes of one section (TagPicker). "All projects" has no tags.
    m_tagHint = new QLabel(QString::fromUtf8("Теги доступны после выбора раздела"), m_searchPanel);   // LANGXXX
    m_tagHolder = new QWidget(m_searchPanel);
    new QVBoxLayout(m_tagHolder);
    m_tagHolder->layout()->setContentsMargins(0, 0, 0, 0);
    m_tagPicker = 0;
    panelLayout->addWidget(m_tagHint);
    panelLayout->addWidget(m_tagHolder);
    connect(m_channelCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(rebuildTagPicker()));
    rebuildTagPicker();

    mainLayout->addWidget(m_searchPanel);

    m_progress = new QProgressBar(central);
    m_progress->setRange(0, 0);
    m_progress->setVisible(false);
    mainLayout->addWidget(m_progress);

    // Page 0: list of results (projects, vacancies or news depending on the tab).
    m_listPage = new QWidget(central);
    QVBoxLayout *listLayout = new QVBoxLayout(m_listPage);
    listLayout->setContentsMargins(0, 0, 0, 0);
    m_resultsView = new QListView(m_listPage);
    m_resultsView->setModel(m_model);
    m_resultsView->setItemDelegate(m_delegate);
    m_resultsView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultsView->setUniformItemSizes(false);
    m_resultsView->setSpacing(2);
    connect(m_resultsView, SIGNAL(activated(QModelIndex)),
            this, SLOT(onResultActivated(QModelIndex)));
    listLayout->addWidget(m_resultsView, 1);

    m_moreBtn = new QPushButton(L("showMore"), m_listPage);
    m_moreBtn->setVisible(false);
    connect(m_moreBtn, SIGNAL(clicked()), this, SLOT(onShowMore()));
    listLayout->addWidget(m_moreBtn);

    // Page 1: details of an opened project, vacancy or news item.
    m_detailPage = new QWidget(central);
    QVBoxLayout *detailLayout = new QVBoxLayout(m_detailPage);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    m_backBtn = new QPushButton(L("back"), m_detailPage);
    connect(m_backBtn, SIGNAL(clicked()), this, SLOT(onDetailBack()));
    detailLayout->addWidget(m_backBtn, 0, Qt::AlignLeft);
    m_detailTitle = new QLabel(m_detailPage);
    m_detailTitle->setWordWrap(true);
    detailLayout->addWidget(m_detailTitle);

    m_actionsBox = new QWidget(m_detailPage);
    QHBoxLayout *actionsLayout = new QHBoxLayout(m_actionsBox);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    m_editBtn = new QPushButton(QString::fromUtf8("Edit"), m_actionsBox);   // LANGXXX
    m_addVacBtn = new QPushButton(QString::fromUtf8("New vacancy"), m_actionsBox);   // LANGXXX
    m_deleteBtn = new QPushButton(L("delete"), m_actionsBox); 
    m_applyBtn = new QPushButton(L("vacRespond"), m_actionsBox); 
    connect(m_editBtn, SIGNAL(clicked()), this, SLOT(onEditDetail()));
    connect(m_addVacBtn, SIGNAL(clicked()), this, SLOT(onAddVacancy()));
    connect(m_deleteBtn, SIGNAL(clicked()), this, SLOT(onDeleteVacancy()));
    connect(m_applyBtn, SIGNAL(clicked()), this, SLOT(onApplyToggle()));
    m_subBtn = new QPushButton(L("gdpsSub"), m_actionsBox);   
    m_unsubBtn = new QPushButton(L("gdpsUnsub"), m_actionsBox);  
    connect(m_subBtn, SIGNAL(clicked()), this, SLOT(onSubscribeClicked()));
    connect(m_unsubBtn, SIGNAL(clicked()), this, SLOT(onUnsubscribeClicked()));
    actionsLayout->addWidget(m_editBtn);
    actionsLayout->addWidget(m_addVacBtn);
    actionsLayout->addWidget(m_deleteBtn);
    actionsLayout->addWidget(m_applyBtn);
    actionsLayout->addWidget(m_subBtn);
    actionsLayout->addWidget(m_unsubBtn);
    actionsLayout->addStretch(1);
    m_actionsBox->setVisible(false);
    detailLayout->addWidget(m_actionsBox);
    m_detailBody = new QTextBrowser(m_detailPage);
    // Links are ours (oh://...), so handle them ourselves instead of letting Qt navigate.
    m_detailBody->setOpenLinks(false);
    m_detailBody->setOpenExternalLinks(false);
    connect(m_detailBody, SIGNAL(anchorClicked(QUrl)), this, SLOT(onDetailAnchor(QUrl)));
    detailLayout->addWidget(m_detailBody, 1);

    m_stack = new QStackedWidget(central);
    m_stack->addWidget(m_listPage);
    m_stack->addWidget(m_detailPage);
    mainLayout->addWidget(m_stack, 1);

    m_statusLabel = new QLabel(central);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    QMenu *accountMenu = menuBar()->addMenu(L("account"));
    m_loginAction = accountMenu->addAction(L("login"));
    m_registerAction = accountMenu->addAction(L("register"));
    m_logoutAction = accountMenu->addAction(L("logout2"));
    connect(m_loginAction, SIGNAL(triggered()), this, SLOT(onLogin()));
    connect(m_registerAction, SIGNAL(triggered()), this, SLOT(onRegister()));
    connect(m_logoutAction, SIGNAL(triggered()), this, SLOT(onLogout()));
    accountMenu->addSeparator();
    QAction *myProfileAction = accountMenu->addAction(QString::fromUtf8("My profile"));   // LANGXXX
    QAction *subsAction = accountMenu->addAction(L("gdpsSubs"));        
    QAction *alarmsAction = accountMenu->addAction(L("Alarms"));   
    connect(myProfileAction, SIGNAL(triggered()), this, SLOT(onOpenMyProfile()));
    connect(subsAction, SIGNAL(triggered()), this, SLOT(onOpenSubs()));
    connect(alarmsAction, SIGNAL(triggered()), this, SLOT(onOpenAlarms()));

    QMenu *toolsMenu = menuBar()->addMenu(L("menuTools"));
    QAction *profileAction = toolsMenu->addAction(L("menuProfile"));
    connect(profileAction, SIGNAL(triggered()), this, SLOT(onOpenProfile()));

    QMenu *createMenu = menuBar()->addMenu(QString::fromUtf8("Create"));   // LANGXXX
    const QStringList kinds = QStringList() << QString::fromUtf8("Camp")       // LANGXXX
                                            << QString::fromUtf8("Show")
                                            << QString::fromUtf8("Pere")
                                            << QString::fromUtf8("Tele");
    for (int i = 0; i < kinds.size(); ++i) {
        QAction *a = createMenu->addAction(kinds.at(i));
        a->setData(i);
        connect(a, SIGNAL(triggered()), this, SLOT(onCreateProject()));
    }

    m_authStatus = new QLabel(this);
    statusBar()->addPermanentWidget(m_authStatus);
}

// -------------------- Tabs and loading --------------------

void MainWindow::onTabChanged(int index)
{
    if (index == TabVacancies)
        m_mode = ModeVacancies;
    else if (index == TabNews)
        m_mode = ModeNews;
    else
        m_mode = ModeProjects;

    m_searchPanel->setVisible(m_mode != ModeNews);
    m_channelRow->setVisible(m_mode == ModeProjects);
    rebuildTagPicker();
    clearResults();
    m_statusLabel->clear();
    startTabLoad();
}

// Vacancies and news list the whole section without a query; projects wait for a search.
void MainWindow::startTabLoad()
{
    if (m_mode == ModeVacancies) {
        m_query.clear();
        m_channel = kChannelVacancies;
        m_method = m_methodCombo->currentData().toInt();
        m_tags.clear();
        m_os.clear();
        m_nextPage = 0;
        setBusy(true);
        m_api->fetchVacancies(0);
    } else if (m_mode == ModeNews) {
        m_channel = kChannelNews;
        m_nextPage = 0;
        setBusy(true);
        m_api->fetchNews(0);
    }
}

void MainWindow::clearResults()
{
    m_model->clear();
    m_moreBtn->setVisible(false);
    m_history.clear();
    m_stack->setCurrentIndex(0);
}

void MainWindow::setBusy(bool busy)
{
    m_searchBtn->setEnabled(!busy);
    m_searchEdit->setEnabled(!busy);
    m_tagHolder->setEnabled(!busy);
    m_channelCombo->setEnabled(!busy);
    m_methodCombo->setEnabled(!busy);
    m_tabs->setEnabled(!busy);
    m_moreBtn->setEnabled(!busy);
    m_progress->setVisible(busy);
    if (busy)
        statusBar()->showMessage(L("searching"));
}

// -------------------- Search and list paging --------------------

void MainWindow::onSearchClicked()
{
    m_query = m_searchEdit->text().trimmed();
    m_channel = (m_mode == ModeVacancies) ? kChannelVacancies
                                          : m_channelCombo->currentData().toInt();
    m_method = m_methodCombo->currentData().toInt();
    m_tags = m_tagPicker ? m_tagPicker->tags() : QList<int>();
    m_os = m_tagPicker ? m_tagPicker->os() : QList<int>();
    m_nextPage = 0;

    setBusy(true);
    clearResults();
    m_statusLabel->clear();
    if (m_mode == ModeVacancies && m_query.isEmpty())
        m_api->fetchVacancies(0);
    else
        m_api->search(m_query, m_channel, m_method, 0, m_tags, m_os);
}

void MainWindow::onShowMore()
{
    setBusy(true);
    if (m_mode == ModeNews)
        m_api->fetchNews(m_nextPage);
    else if (m_mode == ModeVacancies && m_query.isEmpty())
        m_api->fetchVacancies(m_nextPage);
    else
        m_api->search(m_query, m_channel, m_method, m_nextPage, m_tags, m_os);
}

void MainWindow::onSearchFinished(const SearchResultList &results, bool hasMore, int page)
{
    setBusy(false);
    if (page == 0)
        m_model->setResults(results, m_api->baseUrl());
    else
        m_model->appendResults(results, m_api->baseUrl());
    m_nextPage = page + 1;
    m_moreBtn->setVisible(hasMore);

    const int total = m_model->rowCount();
    if (total == 0) {
        m_statusLabel->setText(m_mode == ModeNews ? L("noNews") : L("noResults"));
        statusBar()->showMessage(L("ready") + QStringLiteral(" — 0"));
        return;
    }
    QString found = L("found");
    found.replace(QStringLiteral("%1"), QString::number(total));
    m_statusLabel->setText(found);
    statusBar()->showMessage(L("ready") + QStringLiteral(" — ") + QString::number(total));
}

void MainWindow::onSearchError(const QString &error)
{
    setBusy(false);
    m_statusLabel->setText(L("error") + QStringLiteral(": ") + error);
    statusBar()->showMessage(L("error"));
    QMessageBox::warning(this, L("error"), error);
}

// -------------------- Opening cards and detail history --------------------

void MainWindow::onResultActivated(const QModelIndex &index)
{
    const SearchResult r = index.data(ResultsModel::ResultRole).value<SearchResult>();
    int kind = DetailProject;
    if (r.channel == kChannelVacancies)
        kind = DetailVacancy;
    else if (r.channel == kChannelNews)
        kind = DetailNews;
    openDetail(kind, r.id, 0);
}

void MainWindow::openDetail(int kind, int id, int page, int extra)
{
    setBusy(true);
    m_api->fetchDetail(kind, id, page, extra);
}

void MainWindow::openComments(int contentType, int id, int page)
{
    setBusy(true);
    m_api->fetchComments(contentType, id, page);
}

// Same codes as the JS client: project 0, news 3, vacancy 5.
static int commentTypeFor(int kind)
{
    switch (kind) {
    case DetailProject: return 0;
    case DetailNews:    return 3;
    case DetailVacancy: return 5;
    default:            return -1;
    }
}

void MainWindow::onDetailFinished(const DetailData &d)
{
    setBusy(false);
    if (!d.found) {
        QMessageBox::information(this, L("error"), L("notFound"));
        return;
    }

    // Pages that extend the item on top of history instead of opening a new entry.
    if (d.kind == DetailProjectNews) {
        if (m_history.isEmpty())
            return;
        DetailData &top = m_history.last();
        if (top.kind != DetailProject || top.main.id != d.id)
            return;   // stale reply: the user has moved on
        top.news += d.news;
        top.hasMoreNews = d.hasMoreNews;
        top.page = d.page;
        showTopDetail();
        return;
    }
    if (d.kind == DetailCommentsPage) {
        if (m_history.isEmpty())
            return;
        DetailData &top = m_history.last();
        if (commentTypeFor(top.kind) != d.contentType || top.main.id != d.id)
            return;
        top.comments += d.comments;
        top.hasMoreComments = d.hasMoreComments;
        top.commentsPage = d.commentsPage;
        showTopDetail();
        return;
    }

    m_history.append(d);
    showTopDetail();
}

void MainWindow::onDetailError(const QString &error)
{
    setBusy(false);
    QMessageBox::warning(this, L("error"), error);
}

void MainWindow::showTopDetail()
{
    const DetailData &d = m_history.last();
    m_detailTitle->setText(d.main.title);
    m_detailBody->setHtml(renderDetail(d));
    updateActions(d);
    m_stack->setCurrentIndex(1);
    ensureProbes();
}

void MainWindow::onDetailBack()
{
    if (!m_history.isEmpty())
        m_history.removeLast();
    if (m_history.isEmpty())
        m_stack->setCurrentIndex(0);
    else
        showTopDetail();
}

// Links inside detail pages. All are oh://<what>/<numbers>:
//   news/ID  proj/ID  vac/ID  user/ID  profworks/ID  profadded/ID/CHANNEL
//   projnews/ID/PAGE  comm/TYPE/ID/PAGE
void MainWindow::onDetailAnchor(const QUrl &url)
{
    QString path = url.path();
    if (path.startsWith(QLatin1Char('/')))
        path.remove(0, 1);
    const QStringList p = path.split(QLatin1Char('/'));
    auto num = [&p](int i) -> int { return i < p.size() ? p.at(i).toInt() : 0; };

    const QString what = url.host();
    if (what == QStringLiteral("news"))
        openDetail(DetailNews, num(0), 0);
    else if (what == QStringLiteral("proj"))
        openDetail(DetailProject, num(0), 0);
    else if (what == QStringLiteral("vac"))
        openDetail(DetailVacancy, num(0), 0);
    else if (what == QStringLiteral("user"))
        openDetail(DetailProfile, num(0), 0);
    else if (what == QStringLiteral("profworks"))
        openDetail(DetailProfileWorks, num(0), 0);
    else if (what == QStringLiteral("profadded"))
        openDetail(DetailProfileAdded, num(0), 0, num(1));
    else if (what == QStringLiteral("projnews"))
        openDetail(DetailProjectNews, num(0), num(1));
    else if (what == QStringLiteral("vote"))
        onVote(num(0), num(1), p.value(2) == QLatin1String("dislike"));
    else if (what == QStringLiteral("commwrite"))
        onWriteComment(num(0), num(1));
    else if (what == QStringLiteral("cedit"))
        onEditComment(num(0));
    else if (what == QStringLiteral("cdel"))
        onDeleteComment(num(0));
    else if (what == QStringLiteral("newsnew"))
        onNewsNew(num(0));
    else if (what == QStringLiteral("newsedit"))
        onNewsEdit();
    else if (what == QStringLiteral("newsdel"))
        onNewsDelete();
    else if (what == QStringLiteral("profedit"))
        onProfileEdit();
    else if (what == QStringLiteral("subs"))
        onOpenSubs();
    else if (what == QStringLiteral("alarms"))
        onOpenAlarms();
    else if (what == QStringLiteral("workadd"))
        onAddWork();
    else if (what == QStringLiteral("workedit"))
        onEditWork(num(0));
    else if (what == QStringLiteral("workdel"))
        onDeleteWork(num(0));
    else if (what == QStringLiteral("comm"))
        openComments(num(0), num(1), num(2));
}

// ---- Rendering. Server text is escaped: QTextBrowser renders HTML. ----

static QString escapedText(const QString &s)
{
    return s.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>"));
}

// Name as a profile link. uid <= 0 means no id known, so plain text.
static QString userLink(const QString &name, int uid)
{
    if (uid <= 0)
        return name.toHtmlEscaped();
    return QStringLiteral("<a href=\"oh://user/%1\">%2</a>").arg(uid).arg(name.toHtmlEscaped());
}

static QString votesHtml(const SearchResult &m)
{
    QString votes = L("likesLine");
    votes.replace(QStringLiteral("%1"), QString::number(m.likes));
    votes.replace(QStringLiteral("%2"), QString::number(m.dislikes));
    votes.replace(QStringLiteral("%3"), QString::number(m.comments));
    return votes.toHtmlEscaped();
}

// Project, vacancy, news: description, news list (projects), comments.
// Like codes on the server (LikeTypes in legacy.go): item and its comments.
static int itemCodeFor(int kind)
{
    switch (kind) {
    case DetailProject: return 0;
    case DetailNews:    return 2;
    case DetailVacancy: return 11;
    default:            return -1;
    }
}

static int commentCodeFor(int kind)
{
    switch (kind) {
    case DetailProject: return 3;
    case DetailNews:    return 5;
    case DetailVacancy: return 12;
    default:            return -1;
    }
}

// Like / dislike links. Anchors go through onDetailAnchor -> onVote.
static QString voteLinks(const ApiClient *api, int typeCode, int id)
{
    // Active votes are shown in bold. The server removes a vote on a second click.
    const int st = api ? api->likeState(typeCode, id) : 0;
    const QString like = QStringLiteral("<a href=\"oh://vote/%1/%2/like\">%3</a>")
        .arg(typeCode).arg(id)
        .arg(st == 1 ? QStringLiteral("<b>Liked</b>") : QStringLiteral("Like"));
    const QString dislike = QStringLiteral("<a href=\"oh://vote/%1/%2/dislike\">%3</a>")
        .arg(typeCode).arg(id)
        .arg(st == -1 ? QStringLiteral("<b>Disliked</b>") : QStringLiteral("Dislike"));
    return like + QStringLiteral(" &nbsp; ") + dislike;
}

// Under a comment: votes for everyone logged in, edit and delete for its author.
static QString commentActions(const ApiClient *api, const SearchResult &c, bool logged, int userId, int commentCode)
{
    if (!logged)
        return QString();
    QString h = QStringLiteral("<br>");
    if (commentCode >= 0)
        h += voteLinks(api, commentCode, c.id) + QStringLiteral(" &nbsp; ");
    const int uid = c.raw.value(QStringLiteral("userId")).toInt();
    if (userId > 0 && uid == userId) {
        h += QStringLiteral("<a href=\"oh://cedit/%1\">Edit</a> <a href=\"oh://cdel/%1\">Delete</a>").arg(c.id);
    }
    return h;
}

static QString renderItem(const DetailData &d, int userId, bool logged, const ApiClient *api)
{
    const SearchResult &m = d.main;
    QString h;

    // Author ids: news keep it as authorId, projects and vacancies as author.
    const int authorId = m.raw.contains(QStringLiteral("authorId"))
        ? m.raw.value(QStringLiteral("authorId")).toInt()
        : m.raw.value(QStringLiteral("author")).toInt();

    QString head;
    if (!m.author.isEmpty())
        head += L("addedBy") + QStringLiteral(": <b>") + userLink(m.author, authorId) + QStringLiteral("</b><br>");
    head += votesHtml(m) + QStringLiteral("<br>");
    if (d.kind == DetailNews) {
        const qint64 ts = m.raw.value(QStringLiteral("date")).toLongLong();
        if (ts > 0)
            head += QDateTime::fromMSecsSinceEpoch(ts * 1000).toString(QStringLiteral("dd.MM.yyyy HH:mm")) + QStringLiteral("<br>");
    }
    if (logged && itemCodeFor(d.kind) >= 0)
        head += voteLinks(api, itemCodeFor(d.kind), m.id) + QStringLiteral("<br>");
    if (!m.tags.isEmpty()) {
        QString tags = L("tagsLine");
        tags.replace(QStringLiteral("%1"), tagsText(m.channel, m.tags));
        head += tags.toHtmlEscaped();
    }
    h += QStringLiteral("<p>") + head + QStringLiteral("</p>");

    QString body = m.raw.value(QStringLiteral("description")).toString();
    if (body.isEmpty())
        body = m.raw.value(QStringLiteral("text")).toString();
    if (body.isEmpty())
        body = m.shortDesc;
    if (body.isEmpty())
        body = L("descEmpty");
    h += QStringLiteral("<p>") + escapedText(body) + QStringLiteral("</p>");

    if (d.kind == DetailNews) {
        const int gdpsNum = m.raw.value(QStringLiteral("gdpsNum")).toInt();
        if (gdpsNum > 0) {
            h += QStringLiteral("<p><a href=\"oh://proj/%1\">%2</a></p>")
                    .arg(gdpsNum)
                    .arg(m.raw.value(QStringLiteral("gdpsTitle")).toString().toHtmlEscaped());
        }
    }

    if (logged && d.kind == DetailProject && userId > 0
        && m.raw.value(QStringLiteral("author")).toInt() == userId) {
        h += QStringLiteral("<p><a href=\"oh://newsnew/%1\">%2</a></p>")
                .arg(m.id)
                .arg(QString::fromUtf8("New news"));   // LANGXXX
    }
    if (d.kind == DetailProject) {
        if (!d.news.isEmpty()) {
            h += QStringLiteral("<h3>") + L("newsHead").toHtmlEscaped() + QStringLiteral("</h3><ul>");
            for (int i = 0; i < d.news.size(); ++i) {
                const SearchResult &n = d.news.at(i);
                h += QStringLiteral("<li><a href=\"oh://news/%1\">%2</a></li>")
                        .arg(n.id)
                        .arg(n.title.toHtmlEscaped());
            }
            h += QStringLiteral("</ul>");
        }
        if (d.hasMoreNews) {
            h += QStringLiteral("<p><a href=\"oh://projnews/%1/%2\">%3</a></p>")
                    .arg(d.id)
                    .arg(d.page + 1)
                    .arg(L("showMore").toHtmlEscaped());
        }
    }

    if (logged && commentCodeFor(d.kind) >= 0) {
        h += QStringLiteral("<p><a href=\"oh://commwrite/%1/%2\">%3</a></p>")
                .arg(commentTypeFor(d.kind))
                .arg(m.id)
                .arg(QString::fromUtf8("Write a comment"));   // LANGXXX
    }

    // Comments: the item's own count from the server, not the number loaded so far.
    const int total = qMax(m.comments, d.comments.size());
    QString commentsHead = L("commentsHead");
    commentsHead.replace(QStringLiteral("%1"), QString::number(total));
    h += QStringLiteral("<h3>") + commentsHead.toHtmlEscaped() + QStringLiteral("</h3>");
    if (d.comments.isEmpty()) {
        h += QStringLiteral("<p>") + L("noComments").toHtmlEscaped() + QStringLiteral("</p>");
    } else {
        for (int i = 0; i < d.comments.size(); ++i) {
            const SearchResult &c = d.comments.at(i);
            const int uid = c.raw.value(QStringLiteral("userId")).toInt();
            h += QStringLiteral("<p><b>") + userLink(c.author, uid) + QStringLiteral("</b> +")
               + QString::number(c.likes) + QStringLiteral(" / \u2212") + QString::number(c.dislikes)
               + QStringLiteral("<br>") + escapedText(c.shortDesc)
               + commentActions(api, c, logged, userId, commentCodeFor(d.kind)) + QStringLiteral("</p>");
        }
    }
    if (d.hasMoreComments) {
        h += QStringLiteral("<p><a href=\"oh://comm/%1/%2/%3\">%4</a></p>")
                .arg(commentTypeFor(d.kind))
                .arg(d.id)
                .arg(d.commentsPage + 1)
                .arg(L("showMore").toHtmlEscaped());
    }
    return h;
}

// Stored text keeps newlines as the two characters \n (see SocialDomain::setResume).
static QString storedToView(const QString &s)
{
    QString out = s;
    out.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    return out;
}

static QString renderProfile(const DetailData &d, bool mine)
{
    const SearchResult &m = d.main;
    const QVariantMap &raw = m.raw;
    QString h;
    h += QStringLiteral("<p>") + L("profId") + QStringLiteral(": ") + QString::number(d.id)
       + QStringLiteral("<br>") + L("profRole") + QStringLiteral(": ")
       + raw.value(QStringLiteral("role")).toString().toHtmlEscaped() + QStringLiteral("</p>");
    h += QStringLiteral("<p>") + (raw.value(QStringLiteral("isActive")).toBool()
                                   ? L("isActive") : L("isNotact")).toHtmlEscaped() + QStringLiteral("</p>");
    h += QStringLiteral("<h3>") + L("profSocials").toHtmlEscaped() + QStringLiteral("</h3>")
       + QStringLiteral("<p>") + escapedText(storedToView(raw.value(QStringLiteral("socials")).toString())) + QStringLiteral("</p>");
    h += QStringLiteral("<h3>") + L("profResume").toHtmlEscaped() + QStringLiteral("</h3>")
       + QStringLiteral("<p>") + escapedText(storedToView(raw.value(QStringLiteral("resume")).toString())) + QStringLiteral("</p>");
    if (mine) {
        h += QStringLiteral("<p><a href=\"oh://profedit/0\">%1</a> <a href=\"oh://subs/0\">%2</a> <a href=\"oh://alarms/0\">%3</a></p>")
                .arg(QString::fromUtf8("Edit profile"))   // LANGXXX
                .arg(L("gdpsSubs"))              
                .arg(L("Alarms"));          
    }
    h += QStringLiteral("<ul>");
    h += QStringLiteral("<li><a href=\"oh://profworks/%1\">%2</a></li>").arg(d.id).arg(L("worksHead").toHtmlEscaped());
    h += QStringLiteral("<li><a href=\"oh://profadded/%1/0\">%2</a></li>").arg(d.id).arg(L("addedCamps").toHtmlEscaped());
    h += QStringLiteral("<li><a href=\"oh://profadded/%1/1\">%2</a></li>").arg(d.id).arg(L("addedShows").toHtmlEscaped());
    h += QStringLiteral("<li><a href=\"oh://profadded/%1/2\">%2</a></li>").arg(d.id).arg(L("addedPeres").toHtmlEscaped());
    h += QStringLiteral("</ul>");
    return h;
}

static QString renderWorks(const DetailData &d, bool mine)
{
    QString h;
    if (mine)
        h += QStringLiteral("<p><a href=\"oh://workadd/0\">Add portfolio item</a></p>");
    if (d.items.isEmpty())
        return h + QStringLiteral("<p>") + L("noWorks").toHtmlEscaped() + QStringLiteral("</p>");
    for (int i = 0; i < d.items.size(); ++i) {
        const SearchResult &w = d.items.at(i);
        // gdpsTitle is 0 for an external link (the link itself is the URL); otherwise it is a project.
        const QString gt = w.raw.value(QStringLiteral("gdpsTitle")).toString();
        const bool external = gt.isEmpty() || gt == QStringLiteral("0");
        const QString link = w.raw.value(QStringLiteral("link")).toString();
        h += QStringLiteral("<h3>") + escapedText(w.title) + QStringLiteral("</h3>");
        if (external)
            h += QStringLiteral("<p>") + link.toHtmlEscaped() + QStringLiteral("</p>");
        else
            h += QStringLiteral("<p><a href=\"oh://proj/%1\">%2</a></p>").arg(link).arg(gt.toHtmlEscaped());
        h += QStringLiteral("<p>") + escapedText(w.shortDesc) + QStringLiteral("</p>");
        if (mine) {
            h += QStringLiteral("<p><a href=\"oh://workedit/%1\">Edit</a> <a href=\"oh://workdel/%1\">Delete</a></p>")
                    .arg(w.id);
        }
    }
    return h;
}

static QString renderAdded(const DetailData &d)
{
    if (d.items.isEmpty())
        return QStringLiteral("<p>") + L("noItems").toHtmlEscaped() + QStringLiteral("</p>");
    QString h = QStringLiteral("<ul>");
    for (int i = 0; i < d.items.size(); ++i) {
        const SearchResult &p = d.items.at(i);
        h += QStringLiteral("<li><a href=\"oh://proj/%1\">%2</a><br>%3</li>")
                .arg(p.id)
                .arg(p.title.toHtmlEscaped())
                .arg(escapedText(p.shortDesc));
    }
    return h + QStringLiteral("</ul>");
}

QString MainWindow::renderDetail(const DetailData &d) const
{
    switch (d.kind) {
    case DetailProfile:       return renderProfile(d, m_api->isLoggedIn() && d.id == m_api->userId());
    case DetailProfileWorks:  return renderWorks(d, m_api->isLoggedIn() && d.id == m_api->userId());
    case DetailProfileAdded:  return renderAdded(d);
    default:                  return renderItem(d, m_api->userId(), m_api->isLoggedIn(), m_api);
    }
}

// -------------------- Auth and profile (unchanged in behaviour) --------------------

void MainWindow::updateAuthUi()
{
    // Owner and subscription answers belong to the account that asked
    m_vacOwner.clear();
    m_newsOwner.clear();
    m_subState.clear();
    const bool logged = m_api->isLoggedIn();
    m_loginAction->setEnabled(!logged);
    m_registerAction->setEnabled(!logged);
    m_logoutAction->setEnabled(logged);
    m_authStatus->setText(logged ? L("loggedAs") : L("guest"));
}

void MainWindow::onLogin()
{
    LoginDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    if (dlg.username().isEmpty() || dlg.password().isEmpty()) {
        QMessageBox::warning(this, L("login"), L("errFillLogin"));
        return;
    }
    setBusy(true);
    m_api->login(dlg.username(), dlg.password());
}

void MainWindow::onRegister()
{
    RegisterDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    if (dlg.username().isEmpty() || dlg.password().isEmpty() || dlg.email().isEmpty()) {
        QMessageBox::warning(this, L("register"), L("errFillAll"));
        return;
    }
    setBusy(true);
    m_api->registerUser(dlg.username(), dlg.password(), dlg.email());
}

void MainWindow::onLogout()
{
    m_api->logout();
    updateAuthUi();
}

void MainWindow::onLoginFinished(bool ok, const QString &message)
{
    setBusy(false);
    updateAuthUi();
    if (ok)
        QMessageBox::information(this, L("login"), message);
    else
        QMessageBox::warning(this, L("login"), message);
}

void MainWindow::onRegisterFinished(bool ok, const QString &message)
{
    setBusy(false);
    updateAuthUi();
    if (ok)
        QMessageBox::information(this, L("register"), message);
    else
        QMessageBox::warning(this, L("register"), message);
}

void MainWindow::onOpenProfile()
{
    bool ok = false;
    int id = QInputDialog::getInt(this, L("menuProfile"), L("userIdPrompt"),
                                  1, 1, 999999999, 1, &ok);
    if (!ok)
        return;
    openDetail(DetailProfile, id, 0);
}

void MainWindow::onProfileFinished(const PublicProfile &profile)
{
    setBusy(false);
    // Dialog is parented to the window and deletes itself on close (WA_DeleteOnClose).
    ProfileView *view = new ProfileView(profile, this);
    view->show();
}

void MainWindow::onProfileError(const QString &error)
{
    setBusy(false);
    QMessageBox::warning(this, L("menuProfile"), error);
}


// -------------------- Create / edit / vacancy actions --------------------

bool MainWindow::requireLogin()
{
    if (m_api->isLoggedIn())
        return true;
    QMessageBox::information(this, L("login"),
                             QString::fromUtf8("Please log in to continue."));   // LANGXXX
    return false;
}

// Shows the buttons that make sense for the open item: owner tools on own projects, vacancy tools on vacancies.
void MainWindow::updateActions(const DetailData &d)
{
    const bool logged = m_api->isLoggedIn();
    const bool isProject = (d.kind == DetailProject);
    const bool isVacancy = (d.kind == DetailVacancy);
    const bool isNews = (d.kind == DetailNews);
    const bool mine = isProject && logged && m_api->userId() > 0
                      && d.main.raw.value(QStringLiteral("author")).toInt() == m_api->userId();
    const int applied = d.main.raw.value(QStringLiteral("isApplied")).toInt();

    // Project the item belongs to. Owner = author or co-owner.
    int pid = 0;
    if (isProject)
        pid = d.main.id;
    else if (isVacancy)
        pid = d.main.raw.value(QStringLiteral("gId")).toInt();
    else if (isNews)
        pid = d.main.raw.value(QStringLiteral("gdpsNum")).toInt();

    bool owned = false;
    if (m_api->accountLoaded()) {
        owned = m_api->ownsProject(pid);
    } else if (isVacancy) {
        owned = m_vacOwner.value(d.main.id, -1) == 1;     // fallback while account data is missing
    } else if (isNews) {
        owned = m_newsOwner.value(d.main.id, -1) == 1;
    }

    const bool showEdit = mine || owned;
    const bool showAddVac = isProject && (mine || owned);
    const bool showDelete = owned && (isVacancy || isNews);
    const bool showApply = isVacancy && logged;

    bool subKnown = m_api->accountLoaded() || m_subState.value(d.main.id, -1) != -1;
    const bool subscribed = m_api->accountLoaded() ? m_api->isSubscribed(d.main.id)
                                                   : (m_subState.value(d.main.id, -1) == 1);
    const bool showSub = isProject && logged && !mine && subKnown && !subscribed;
    const bool showUnsub = isProject && logged && !mine && subKnown && subscribed;

    m_editBtn->setVisible(showEdit);
    m_addVacBtn->setVisible(showAddVac);
    m_deleteBtn->setVisible(showDelete);
    m_applyBtn->setVisible(showApply);
    m_applyBtn->setText(applied > 0 ? QString::fromUtf8("Withdraw application")
                                    : QString::fromUtf8("Apply"));
    m_subBtn->setVisible(showSub);
    m_unsubBtn->setVisible(showUnsub);
    m_actionsBox->setVisible(showEdit || showAddVac || showDelete || showApply || showSub || showUnsub);
}

void MainWindow::reopenTopVacancy()
{
    if (m_history.isEmpty() || m_history.last().kind != DetailVacancy)
        return;
    const int vid = m_history.last().main.id;
    m_history.removeLast();
    openDetail(DetailVacancy, vid, 0);
}

void MainWindow::onCreateProject()
{
    if (!requireLogin())
        return;
    QAction *a = qobject_cast<QAction *>(sender());
    ProjectForm f;
    f.channel = a ? a->data().toInt() : 0;
    ProjectDialog dlg(f, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_pendingChannel = f.channel;
    setBusy(true);
    m_api->saveProject(dlg.form(), dlg.imgFile(), dlg.banFile());
}

void MainWindow::onEditDetail()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    const DetailData &d = m_history.last();
    if (d.kind == DetailNews) {
        onNewsEdit();
        return;
    }
    setBusy(true);
    if (d.kind == DetailVacancy)
        m_api->fetchVacancyForm(d.main.id);
    else
        m_api->fetchProjectForm(d.main.channel, d.main.id);
}

void MainWindow::onProjectFormLoaded(const ProjectForm &form)
{
    setBusy(false);
    ProjectDialog dlg(form, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_pendingChannel = form.channel;
    setBusy(true);
    m_api->saveProject(dlg.form(), dlg.imgFile(), dlg.banFile());
}

void MainWindow::onVacancyFormLoaded(const VacancyForm &form)
{
    setBusy(false);
    VacancyDialog dlg(form, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    setBusy(true);
    m_api->saveVacancy(dlg.form());
}

void MainWindow::onAddVacancy()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    VacancyForm f;
    f.gdpsId = m_history.last().main.id;
    VacancyDialog dlg(f, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    setBusy(true);
    m_api->saveVacancy(dlg.form());
}

void MainWindow::onDeleteVacancy()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    if (m_history.last().kind == DetailNews) {
        onNewsDelete();
        return;
    }
    if (QMessageBox::question(this, QString::fromUtf8("Vacancy"),   // LANGXXX
                              QString::fromUtf8("Delete this vacancy?"),   // LANGXXX
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    setBusy(true);
    m_api->removeVacancy(m_history.last().main.id);
}

void MainWindow::onApplyToggle()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    const DetailData &d = m_history.last();
    const int applied = d.main.raw.value(QStringLiteral("isApplied")).toInt();
    setBusy(true);
    if (applied > 0)
        m_api->unapplyVacancy(applied);
    else
        m_api->applyVacancy(d.main.id);
}

void MainWindow::onActionFinished(int action, bool ok, const QString &message, int id)
{
    setBusy(false);
    if (!ok) {
        m_pendingProfile = 0;
        QMessageBox::warning(this, L("error"), message);
        return;
    }
    statusBar()->showMessage(message, 5000);

    switch (action) {
    case ActSaveProject:
        if (id > 0) {
            // Edit: replace the stale entry with a fresh one
            if (!m_history.isEmpty())
                m_history.removeLast();
            openDetail(DetailProject, id, 0);
        } else {
            // Add: the server does not return the id, so show the user's list for this channel
            openDetail(DetailProfileAdded, m_api->userId(), 0, m_pendingChannel);
        }
        break;
    case ActSaveVacancy:
        if (id > 0)
            reopenTopVacancy();   // edit from a vacancy page; add from a project page is a no-op here
        break;
    case ActRemoveVacancy:
        onDetailBack();
        break;
    case ActApply:
    case ActUnapply:
        reopenTopVacancy();
        break;
    case ActSaveNews:
        // Edit replaces the open news; add opens the new one on top of the project page
        if (!m_history.isEmpty() && m_history.last().kind == DetailNews)
            m_history.removeLast();
        openDetail(DetailNews, id, 0);
        break;
    case ActDeleteNews:
        onDetailBack();
        break;
    case ActSaveWork:
    case ActDeleteWork:
        // Reload the portfolio page so it shows the change
        if (!m_history.isEmpty() && m_history.last().kind == DetailProfileWorks)
            m_history.removeLast();
        openDetail(DetailProfileWorks, m_api->userId(), 0);
        break;
    default:
        break;
    }
}


// -------------------- News, likes, comments --------------------

void MainWindow::onVote(int typeCode, int id, bool dislike)
{
    if (!requireLogin())
        return;
    m_api->vote(typeCode, id, dislike);
}

void MainWindow::onVotesChanged(int typeCode, int id, int likes, int dislikes)
{
    if (m_history.isEmpty())
        return;
    DetailData &top = m_history.last();
    if (typeCode == itemCodeFor(top.kind) && top.main.id == id) {
        top.main.likes = likes;
        top.main.dislikes = dislikes;
    } else if (typeCode == commentCodeFor(top.kind)) {
        for (int i = 0; i < top.comments.size(); ++i) {
            if (top.comments[i].id == id) {
                top.comments[i].likes = likes;
                top.comments[i].dislikes = dislikes;
            }
        }
    } else {
        return;
    }
    showTopDetail();
}

void MainWindow::onCommentsUpdated(int contentType, int id, const SearchResultList &comments, bool hasMore)
{
    if (m_history.isEmpty())
        return;
    DetailData &top = m_history.last();
    // Accept either the stored contentType or the kind-derived one (stale replies after navigation).
    if (top.main.id != id)
        return;
    if (top.contentType != contentType && commentTypeFor(top.kind) != contentType)
        return;
    top.comments = comments;
    top.hasMoreComments = hasMore;
    top.commentsPage = 0;
    // postComment always reloads the first page after a successful post.
    // The total comment count on the item increases by one.
    top.main.comments += 1;
    showTopDetail();
}

void MainWindow::onCommentEdited(int commentId, const QString &text)
{
    if (m_history.isEmpty())
        return;
    DetailData &top = m_history.last();
    for (int i = 0; i < top.comments.size(); ++i) {
        if (top.comments[i].id == commentId)
            top.comments[i].shortDesc = text;
    }
    showTopDetail();
}

void MainWindow::onCommentDeleted(int commentId)
{
    if (m_history.isEmpty())
        return;
    DetailData &top = m_history.last();
    for (int i = 0; i < top.comments.size(); ++i) {
        if (top.comments[i].id == commentId) {
            top.comments.removeAt(i);
            top.main.comments = qMax(0, top.main.comments - 1);
            break;
        }
    }
    showTopDetail();
}

void MainWindow::onWriteComment(int contentType, int id)
{
    if (!requireLogin())
        return;
    bool ok = false;
    const QString text = QInputDialog::getText(this, L("comms"), QString::fromUtf8("Comment text:"),   // LANGXXX
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;
    if (text.trimmed().length() < 10) {
        QMessageBox::warning(this, L("comms"), L("min10chars"));
        return;
    }
    m_api->postComment(contentType, id, text.trimmed());
}

void MainWindow::onEditComment(int commentId)
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    const DetailData &top = m_history.last();
    QString current;
    for (int i = 0; i < top.comments.size(); ++i) {
        if (top.comments[i].id == commentId)
            current = top.comments[i].shortDesc;
    }
    bool ok = false;
    const QString text = QInputDialog::getText(this, L("comms"), QString::fromUtf8("Comment text:"),   // LANGXXX
                                               QLineEdit::Normal, current, &ok);
    if (!ok)
        return;
    if (text.trimmed().length() < 10) {
        QMessageBox::warning(this, L("comms"), L("min10chars"));
        return;
    }
    m_api->editComment(commentId, text.trimmed());
}

void MainWindow::onDeleteComment(int commentId)
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    if (QMessageBox::question(this, L("comms"), QString::fromUtf8("Delete this comment?"),   // LANGXXX
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    const DetailData &top = m_history.last();
    int ctype = top.contentType;
    if (ctype < 0)
        ctype = commentTypeFor(top.kind);
    m_api->deleteComment(commentId, ctype);
}

void MainWindow::onNewsNew(int projectId)
{
    if (!requireLogin())
        return;
    NewsForm f;
    f.gdpsId = projectId;
    NewsDialog dlg(f, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    setBusy(true);
    m_api->saveNews(dlg.form(), dlg.file());
}

void MainWindow::onNewsEdit()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    const DetailData &top = m_history.last();
    NewsForm f;
    f.id = top.main.id;
    f.gdpsId = top.main.raw.value(QStringLiteral("gdpsNum")).toInt();
    f.title = top.main.title;
    f.text = top.main.shortDesc;
    NewsDialog dlg(f, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    setBusy(true);
    m_api->saveNews(dlg.form(), QString());
}

void MainWindow::onNewsDelete()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    if (QMessageBox::question(this, L("news"), QString::fromUtf8("Delete this news?"),   // LANGXXX
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    setBusy(true);
    m_api->deleteNews(m_history.last().main.id);
}


// -------------------- Profile, subscriptions, notifications --------------------

void MainWindow::onOpenMyProfile()
{
    if (!requireLogin())
        return;
    openDetail(DetailProfile, m_api->userId(), 0);
}

void MainWindow::onProfileEdit()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    const DetailData &top = m_history.last();
    ProfileEditDialog dlg(top.main.raw.value(QStringLiteral("username")).toString(),
                          top.main.raw.value(QStringLiteral("resume")).toString(),
                          top.main.raw.value(QStringLiteral("socials")).toString(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_pendingProfile = 3;
    setBusy(true);
    m_api->setNickname(dlg.nick());
    m_api->setResume(dlg.resume());
    m_api->setSocials(dlg.socials());
}

void MainWindow::onProfileFieldSaved(int field, const QString &value)
{
    Q_UNUSED(field);
    Q_UNUSED(value);
    if (m_pendingProfile > 0)
        --m_pendingProfile;
    if (m_pendingProfile > 0)
        return;
    // All three saved: reload the profile page so it shows the new values
    setBusy(false);
    if (!m_history.isEmpty() && m_history.last().kind == DetailProfile)
        m_history.removeLast();
    openDetail(DetailProfile, m_api->userId(), 0);
    statusBar()->showMessage(QString::fromUtf8("Profile saved"), 5000);   // LANGXXX
}

void MainWindow::onOpenSubs()
{
    if (!requireLogin())
        return;
    setBusy(true);
    m_api->fetchSubscriptions(0);
}

void MainWindow::onSubsLoaded(const QVariantList &items, int page, bool hasMore, int probeId)
{
    if (probeId > 0) {
        // Only checking whether the open project is in the subscriptions. Walk the pages until found.
        bool found = false;
        for (int i = 0; i < items.size(); ++i) {
            if (items.at(i).toMap().value(QStringLiteral("ID")).toInt() == probeId) {
                found = true;
                break;
            }
        }
        if (!found && hasMore) {
            m_api->fetchSubscriptions(page + 1, probeId);
            return;
        }
        m_subState.insert(probeId, found ? 1 : 0);
        if (!m_history.isEmpty() && m_history.last().main.id == probeId)
            updateActions(m_history.last());
        return;
    }
    if (m_subsDlg) {
        m_subsDlg->appendItems(items, page, hasMore);
        return;
    }
    setBusy(false);
    SubscriptionsDialog dlg(items, page, hasMore, this);
    m_subsDlg = &dlg;
    connect(&dlg, SIGNAL(moreRequested(int)), this, SLOT(onSubsMore(int)));
    const bool picked = dlg.exec() == QDialog::Accepted && dlg.chosenId() > 0;
    m_subsDlg = 0;
    if (picked)
        openDetail(DetailProject, dlg.chosenId(), 0);
}

void MainWindow::onSubscribeClicked()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    m_api->subscribe(m_history.last().main.id, false);
}

void MainWindow::onUnsubscribeClicked()
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    m_api->subscribe(m_history.last().main.id, true);
}

void MainWindow::onSubscriptionChanged(int id, bool subscribed)
{
    m_subState.insert(id, subscribed ? 1 : 0);
    if (!m_history.isEmpty() && m_history.last().main.id == id)
        updateActions(m_history.last());
    statusBar()->showMessage(subscribed ? QString::fromUtf8("You subscribed")    // LANGXXX
                                        : QString::fromUtf8("You unsubscribed"),   // LANGXXX
                             5000);
}

void MainWindow::onOpenAlarms()
{
    if (!requireLogin())
        return;
    setBusy(true);
    m_api->fetchAlarms(0);
}

void MainWindow::onAlarmsLoaded(const QVariantList &items, int page, bool hasMore)
{
    if (m_alarmsDlg) {
        m_alarmsDlg->appendItems(items, page, hasMore);
        return;
    }
    setBusy(false);
    AlarmsDialog dlg(items, page, hasMore, this);
    m_alarmsDlg = &dlg;
    connect(&dlg, SIGNAL(selectRequested(int)), this, SLOT(onAlarmSelect(int)));
    connect(&dlg, SIGNAL(deleteRequested(int)), this, SLOT(onAlarmDelete(int)));
    connect(&dlg, SIGNAL(moreRequested(int)), this, SLOT(onAlarmsMore(int)));
    dlg.exec();
    m_alarmsDlg = 0;
}

void MainWindow::onAlarmSelect(int id)
{
    m_api->fetchAlarm(id);
}

void MainWindow::onAlarmLoaded(const QVariantMap &alarm)
{
    if (m_alarmsDlg)
        m_alarmsDlg->showAlarm(alarm);
}

void MainWindow::onAlarmDelete(int id)
{
    m_api->deleteAlarm(id);
}

void MainWindow::onAlarmDeleted(int id)
{
    if (m_alarmsDlg)
        m_alarmsDlg->removeAlarm(id);
}


// -------------------- Paging and owner probes --------------------

void MainWindow::onSubsMore(int page)
{
    m_api->fetchSubscriptions(page, 0);
}

void MainWindow::onAlarmsMore(int page)
{
    m_api->fetchAlarms(page);
}

void MainWindow::onVacancyOwnership(int id, bool owned)
{
    m_vacOwner.insert(id, owned ? 1 : 0);
    if (!m_history.isEmpty() && m_history.last().kind == DetailVacancy && m_history.last().main.id == id)
        updateActions(m_history.last());
}

void MainWindow::onNewsOwnership(int id, bool owned)
{
    m_newsOwner.insert(id, owned ? 1 : 0);
    if (!m_history.isEmpty() && m_history.last().kind == DetailNews && m_history.last().main.id == id)
        updateActions(m_history.last());
}

// Asks the server once per object for things the page cannot know from its own data.
// Results come back through the signals above, and each object is asked only once per account.
void MainWindow::ensureProbes()
{
    // With account data loaded (likesT and loginT), owner and subscription state are known.
    if (!m_api->isLoggedIn() || m_api->accountLoaded() || m_history.isEmpty())
        return;
    const DetailData &top = m_history.last();
    const int id = top.main.id;
    if (top.kind == DetailVacancy && !m_vacOwner.contains(id)) {
        m_vacOwner.insert(id, -1);
        m_api->probeVacancy(id);
    } else if (top.kind == DetailNews && !m_newsOwner.contains(id)) {
        m_newsOwner.insert(id, -1);
        m_api->probeNews(id, top.main.raw.value(QStringLiteral("gdpsNum")).toInt());
    } else if (top.kind == DetailProject && m_api->userId() > 0
               && top.main.raw.value(QStringLiteral("author")).toInt() != m_api->userId()
               && !m_subState.contains(id)) {
        m_subState.insert(id, -1);
        m_api->fetchSubscriptions(0, id);
    }
}


// -------------------- Portfolio --------------------

void MainWindow::onAddWork()
{
    if (!requireLogin())
        return;
    WorkForm f;
    WorkDialog dlg(f, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    setBusy(true);
    m_api->saveWork(dlg.form());
}

void MainWindow::onEditWork(int workId)
{
    if (!requireLogin() || m_history.isEmpty())
        return;
    const DetailData &top = m_history.last();
    for (int i = 0; i < top.items.size(); ++i) {
        const SearchResult &w = top.items.at(i);
        if (w.id != workId)
            continue;
        const QString gt = w.raw.value(QStringLiteral("gdpsTitle")).toString();
        WorkForm f;
        f.id = workId;
        f.title = w.title;
        f.text = w.shortDesc;
        f.linkType = (gt.isEmpty() || gt == QStringLiteral("0")) ? 1 : 0;
        f.link = w.raw.value(QStringLiteral("link")).toString();
        WorkDialog dlg(f, this);
        if (dlg.exec() != QDialog::Accepted)
            return;
        setBusy(true);
        m_api->saveWork(dlg.form());
        return;
    }
}

void MainWindow::onDeleteWork(int workId)
{
    if (!requireLogin())
        return;
    if (QMessageBox::question(this, QString::fromUtf8("Portfolio"), QString::fromUtf8("Delete this portfolio item?"),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    setBusy(true);
    m_api->deleteWork(workId);
}


void MainWindow::onAccountChanged()
{
    // Likes, subscriptions and ownership arrived: redraw the open item
    if (!m_history.isEmpty())
        showTopDetail();
}

void MainWindow::rebuildTagPicker()
{
    if (m_tagPicker) {
        delete m_tagPicker;                  // leaves its layout with it
        m_tagPicker = 0;
    }
    const bool vacancies = (m_mode == ModeVacancies);
    const bool projects = (m_mode == ModeProjects);
    const int channel = projects ? m_channelCombo->currentData().toInt() : kChannelAll;
    // A tag filter needs one concrete section. In "all projects" there is no picker at all.
    const bool concrete = projects && channel >= 0 && channel <= 3;
    if (vacancies)
        m_tagPicker = new TagPicker(0, true, QList<int>(), QList<int>(), m_tagHolder);
    else if (concrete)
        m_tagPicker = new TagPicker(channel, false, QList<int>(), QList<int>(), m_tagHolder);
    if (m_tagPicker) {
        m_tagHolder->layout()->addWidget(m_tagPicker);
        m_tagPicker->show();
    }
    m_tagHolder->setVisible(vacancies || concrete);
    m_tagHint->setVisible(projects && !concrete);
}
