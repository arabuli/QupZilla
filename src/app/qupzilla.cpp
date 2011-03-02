#include "qupzilla.h"
#include "tabwidget.h"
#include "tabbar.h"
#include "webpage.h"
#include "webview.h"
#include "lineedit.h"
#include "historymodel.h"
#include "locationbar.h"
#include "searchtoolbar.h"
#include "websearchbar.h"
#include "downloadmanager.h"
#include "cookiejar.h"
#include "cookiemanager.h"
#include "historymanager.h"
#include "bookmarksmanager.h"
#include "bookmarkstoolbar.h"
#include "clearprivatedata.h"
#include "sourceviewer.h"
#include "siteinfo.h"
#include "preferences.h"
#include "networkmanager.h"
#include "autofillmodel.h"
#include "networkmanagerproxy.h"
#include "rssmanager.h"
#include "mainapplication.h"
#include "aboutdialog.h"
#include "pluginproxy.h"
#include "qtwin.h"

const QString QupZilla::VERSION="0.9.7";
const QString QupZilla::BUILDTIME="03/05/2011 14:48";
const QString QupZilla::AUTHOR="nowrep";
const QString QupZilla::COPYRIGHT="2010-2011";
const QString QupZilla::WWWADDRESS="http://qupzilla.ic.cz";
const QString QupZilla::WEBKITVERSION=qWebKitVersion();

QupZilla::QupZilla(bool tryRestore, QUrl startUrl) :
    QMainWindow()
    ,p_mainApp(MainApplication::getInstance())
    ,m_tryRestore(tryRestore)
    ,m_startingUrl(startUrl)
    ,m_actionPrivateBrowsing(0)
    ,m_webInspectorDock(0)
    ,m_webSearchToolbar(0)
{
    this->resize(640,480);
    this->setWindowState(Qt::WindowMaximized);
    this->setWindowTitle("QupZilla");
    setUpdatesEnabled(false);

    m_activeProfil = p_mainApp->getActiveProfil();
    m_activeLanguage = p_mainApp->getActiveLanguage();

    QDesktopServices::setUrlHandler("http", this, "loadAddress");

    setupUi();
    setupMenu();
    QTimer::singleShot(0, this, SLOT(postLaunch()));
    connect(p_mainApp, SIGNAL(message(MainApplication::MessageType,bool)), this, SLOT(receiveMessage(MainApplication::MessageType,bool)));
}

void QupZilla::loadSettings()
{
    QSettings settings(m_activeProfil+"settings.ini", QSettings::IniFormat);

    //Url settings
    settings.beginGroup("Web-URL-Settings");
    m_homepage = settings.value("homepage","http://qupzilla.ic.cz/search/").toUrl();
    m_newtab = settings.value("newTabUrl","").toUrl();
    settings.endGroup();

    QWebSettings* websettings=p_mainApp->webSettings();
    websettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
    //Web browsing settings
    settings.beginGroup("Web-Browser-Settings");
    bool allowFlash = settings.value("allowFlash",true).toBool();
    settings.endGroup();
    m_allowFlashIcon->setVisible(allowFlash);

    //Browser Window settings
    settings.beginGroup("Browser-View-Settings");
    m_menuTextColor = settings.value("menuTextColor", QColor(Qt::black)).value<QColor>();
    setBackground(m_menuTextColor);
    m_bookmarksToolbar->setColor(m_menuTextColor);
    m_ipLabel->setStyleSheet("QLabel {color: "+m_menuTextColor.name()+";}");
    bool showStatusBar = settings.value("showStatusBar",true).toBool();
    bool showHomeIcon = settings.value("showHomeButton",true).toBool();
    bool showBackForwardIcons = settings.value("showBackForwardButtons",true).toBool();
    bool showBookmarksToolbar = settings.value("showBookmarksToolbar",true).toBool();
    bool showNavigationToolbar = settings.value("showNavigationToolbar",true).toBool();
    bool showMenuBar = settings.value("showMenubar",true).toBool();
    bool makeTransparent = settings.value("useTransparentBackground",false).toBool();
    settings.endGroup();

    statusBar()->setVisible(showStatusBar);
    m_actionShowStatusbar->setChecked(showStatusBar);

    m_bookmarksToolbar->setVisible(showBookmarksToolbar);
    m_actionShowBookmarksToolbar->setChecked(showBookmarksToolbar);

    m_navigation->setVisible(showNavigationToolbar);
    m_actionShowToolbar->setChecked(showNavigationToolbar);

    m_actionShowMenubar->setChecked(showMenuBar);
    menuBar()->setVisible(showMenuBar);
    m_navigation->actions().at(m_navigation->actions().count()-2)->setVisible(!showMenuBar);

    m_buttonHome->setVisible(showHomeIcon);
    m_buttonBack->setVisible(showBackForwardIcons);
    m_buttonNext->setVisible(showBackForwardIcons);

    //Private browsing
    m_actionPrivateBrowsing->setChecked( p_mainApp->webSettings()->testAttribute(QWebSettings::PrivateBrowsingEnabled) );
    m_privateBrowsing->setVisible( p_mainApp->webSettings()->testAttribute(QWebSettings::PrivateBrowsingEnabled) );

    if (!makeTransparent)
        return;
    //Opacity
#ifdef Q_WS_X11
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, false);
    QPalette pal = palette();
    QColor bg = pal.window().color();
    bg.setAlpha(180);
    pal.setColor(QPalette::Window, bg);
    setPalette(pal);
    ensurePolished(); // workaround Oxygen filling the background
    setAttribute(Qt::WA_StyledBackground, false);
#endif
    if (QtWin::isCompositionEnabled()) {
        QtWin::extendFrameIntoClientArea(this);
        setContentsMargins(0, 0, 0, 0);
    }
    setWindowIcon(QIcon(":/icons/qupzilla.png"));
}

void QupZilla::receiveMessage(MainApplication::MessageType mes, bool state)
{
    switch (mes) {
    case MainApplication::ShowFlashIcon:
        m_allowFlashIcon->setVisible(state);
        break;

    case MainApplication::CheckPrivateBrowsing:
        m_privateBrowsing->setVisible(state);
        m_actionPrivateBrowsing->setChecked(state);
        break;

    default:
        qWarning() << "Unresolved message sent!";
        break;
    }
}

void QupZilla::refreshHistory(int index)
{
    QWebHistory* history;
    if (index == -1)
        history = weView()->page()->history();
    else
        history = weView()->page()->history();

    if (history->canGoBack()) {
        m_buttonBack->setEnabled(true);
    }else{
        m_buttonBack->setEnabled(false);
    }

    if (history->canGoForward()) {
        m_buttonNext->setEnabled(true);
    }else{
        m_buttonNext->setEnabled(false);
    }
}

void QupZilla::goAtHistoryIndex()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        weView()->page()->history()->goToItem(weView()->page()->history()->itemAt(action->data().toInt()));
    }
    refreshHistory();
}

void QupZilla::aboutToShowHistoryBackMenu()
{
    if (!m_menuBack || !weView())
        return;
    m_menuBack->clear();
    QWebHistory* history = weView()->history();
    int curindex = history->currentItemIndex();
    for (int i = curindex-1;i>=0;i--) {
        QWebHistoryItem item = history->itemAt(i);
        if (item.isValid()) {
            QString title = item.title();
            if (title.length()>40) {
                title.truncate(40);
                title+="..";
            }
            QAction* action = m_menuBack->addAction(m_locationBar->icon(item.url()),title, this, SLOT(goAtHistoryIndex()));
            action->setData(i);
        }
    }
}

void QupZilla::aboutToShowHistoryNextMenu()
{
    if (!m_menuForward || !weView())
        return;
    m_menuForward->clear();
    QWebHistory* history = weView()->history();
    int curindex = history->currentItemIndex();
    for (int i = curindex+1;i<history->count();i++) {
        QWebHistoryItem item = history->itemAt(i);
        if (item.isValid()) {
            QString title = item.title();
            if (title.length()>40) {
                title.truncate(40);
                title+="..";
            }
            QAction* action = m_menuForward->addAction(m_locationBar->icon(item.url()),title, this, SLOT(goAtHistoryIndex()));
            action->setData(i);
        }
    }
}

void QupZilla::aboutToShowBookmarksMenu()
{
    m_menuBookmarks->clear();
    m_menuBookmarks->addAction(tr("Bookmark This Page"), this, SLOT(bookmarkPage()))->setShortcut(QKeySequence("Ctrl+D"));
    m_menuBookmarks->addAction(tr("Bookmark All Tabs"), this, SLOT(bookmarkAllTabs()));
    m_menuBookmarks->addAction(QIcon::fromTheme("user-bookmarks"), tr("Organize Bookmarks"), this, SLOT(showBookmarksManager()))->setShortcut(QKeySequence("Ctrl+Shift+O"));
    m_menuBookmarks->addSeparator();
    if (m_tabWidget->count() == 1)
        m_menuBookmarks->actions().at(1)->setEnabled(false);
    QSqlQuery query;
    query.exec("SELECT title, url FROM bookmarks WHERE folder='bookmarksMenu'");
    while(query.next()) {
        QUrl url = query.value(1).toUrl();
        QString title = query.value(0).toString();
        if (title.length()>40) {
            title.truncate(40);
            title+="..";
        }
        m_menuBookmarks->addAction(LocationBar::icon(url), title, this, SLOT(loadActionUrl()))->setData(url);
    }

    QMenu* folderBookmarks = new QMenu(tr("Bookmarks In ToolBar"), m_menuBookmarks);
    folderBookmarks->setIcon(QIcon(style()->standardIcon(QStyle::SP_DirOpenIcon)));

    query.exec("SELECT title, url FROM bookmarks WHERE folder='bookmarksToolbar'");
    while(query.next()) {
        QUrl url = query.value(1).toUrl();
        QString title = query.value(0).toString();
        if (title.length()>40) {
            title.truncate(40);
            title+="..";
        }
        folderBookmarks->addAction(LocationBar::icon(url), title, this, SLOT(loadActionUrl()))->setData(url);
    }
    if (folderBookmarks->isEmpty())
        folderBookmarks->addAction(tr("Empty"));
    m_menuBookmarks->addMenu(folderBookmarks);

    query.exec("SELECT name FROM folders");
    while(query.next()) {
        QMenu* tempFolder = new QMenu(query.value(0).toString(), m_menuBookmarks);
        tempFolder->setIcon(QIcon(style()->standardIcon(QStyle::SP_DirOpenIcon)));

        QSqlQuery query2;
        query2.exec("SELECT title, url FROM bookmarks WHERE folder='"+query.value(0).toString()+"'");
        while(query2.next()) {
            QUrl url = query2.value(1).toUrl();
            QString title = query2.value(0).toString();
            if (title.length()>40) {
                title.truncate(40);
                title+="..";
            }
            tempFolder->addAction(LocationBar::icon(url), title, this, SLOT(loadActionUrl()))->setData(url);
        }
        if (tempFolder->isEmpty())
            tempFolder->addAction(tr("Empty"));
        m_menuBookmarks->addMenu(tempFolder);
    }

}

void QupZilla::aboutToShowHistoryMenu()
{
    if (!weView())
        return;
    m_menuHistory->clear();
    m_menuHistory->addAction(
#ifdef Q_WS_X11
            style()->standardIcon(QStyle::SP_ArrowBack)
#else
            QIcon(":/icons/faenza/back.png")
#endif
            , tr("Back"), this, SLOT(goBack()))->setShortcut(QKeySequence("Ctrl+Left"));
    m_menuHistory->addAction(
#ifdef Q_WS_X11
            style()->standardIcon(QStyle::SP_ArrowForward)
#else
            QIcon(":/icons/faenza/forward.png")
#endif
            , tr("Forward"), this, SLOT(goNext()))->setShortcut(QKeySequence("Ctrl+Right"));
    m_menuHistory->addAction(
#ifdef Q_WS_X11
            QIcon::fromTheme("go-home")
#else
            QIcon(":/icons/faenza/home.png")
#endif
            , tr("Home"), this, SLOT(goHome()))->setShortcut(QKeySequence("Alt+Home"));

    if (!weView()->history()->canGoBack())
        m_menuHistory->actions().at(0)->setEnabled(false);
    if (!weView()->history()->canGoForward())
        m_menuHistory->actions().at(1)->setEnabled(false);

    m_menuHistory->addAction(QIcon(":/icons/menu/history.png"), tr("Show All History"), this, SLOT(showHistoryManager()))->setShortcut(QKeySequence("Ctrl+H"));
    m_menuHistory->addSeparator();

    QSqlQuery query;
    query.exec("SELECT title, url FROM history ORDER BY date DESC LIMIT 10");
    while(query.next()) {
        QUrl url = query.value(1).toUrl();
        QString title = query.value(0).toString();
        if (title.length()>40) {
            title.truncate(40);
            title+="..";
        }
        m_menuHistory->addAction(LocationBar::icon(url), title, this, SLOT(loadActionUrl()))->setData(url);
    }
}

void QupZilla::aboutToShowHelpMenu()
{
    m_menuHelp->clear();
    m_menuHelp->addAction(tr("Report Bug"), this, SLOT(reportBug()));
    m_menuHelp->addSeparator();
    p_mainApp->plugins()->populateHelpMenu(m_menuHelp);
    m_menuHelp->addAction(QIcon(":/icons/menu/qt.png"), tr("About Qt"), qApp, SLOT(aboutQt()));
    m_menuHelp->addAction(QIcon(":/icons/qupzilla.png"), tr("About QupZilla"), this, SLOT(aboutQupZilla()));
}

void QupZilla::aboutToShowToolsMenu()
{
    m_menuTools->clear();
    m_menuTools->addAction(tr("Web Search"), this, SLOT(webSearch()))->setShortcut(QKeySequence("Ctrl+K"));
    m_menuTools->addAction(QIcon::fromTheme("dialog-information"), tr("Page Info"), this, SLOT(showPageInfo()))->setShortcut(QKeySequence("Ctrl+I"));
    m_menuTools->addSeparator();
    m_menuTools->addAction(tr("Download Manager"), this, SLOT(showDownloadManager()))->setShortcut(QKeySequence("Ctrl+Y"));
    m_menuTools->addAction(tr("Cookies Manager"), this, SLOT(showCookieManager()));
    m_menuTools->addAction(QIcon(":/icons/menu/rss.png"), tr("RSS Reader"), this,  SLOT(showRSSManager()));
    m_menuTools->addAction(QIcon::fromTheme("edit-clear"), tr("Clear Recent History"), this, SLOT(showClearPrivateData()));
    m_actionPrivateBrowsing = new QAction(tr("Private Browsing"), this);
    m_actionPrivateBrowsing->setCheckable(true);
    m_actionPrivateBrowsing->setChecked(p_mainApp->webSettings()->testAttribute(QWebSettings::PrivateBrowsingEnabled));
    connect(m_actionPrivateBrowsing, SIGNAL(triggered(bool)), this, SLOT(startPrivate(bool)));
    m_menuTools->addAction(m_actionPrivateBrowsing);
    m_menuTools->addSeparator();
    p_mainApp->plugins()->populateToolsMenu(m_menuTools);
    m_menuTools->addAction(QIcon(":/icons/faenza/settings.png"), tr("Preferences"), this, SLOT(showPreferences()))->setShortcut(QKeySequence("Ctrl+P"));
}

void QupZilla::aboutToShowViewMenu()
{
    if (!weView())
        return;

    if (weView()->isLoading())
        m_actionStop->setEnabled(true);
    else
        m_actionStop->setEnabled(false);
}

void QupZilla::bookmarkPage()
{
    p_mainApp->bookmarksManager()->addBookmark(weView());
}

void QupZilla::addBookmark(const QUrl &url, const QString &title)
{
    p_mainApp->bookmarksManager()->insertBookmark(url, title);
}

void QupZilla::bookmarkAllTabs()
{
    p_mainApp->bookmarksManager()->insertAllTabs();
}

void QupZilla::loadActionUrl()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        loadAddress(action->data().toUrl());
    }
}

void QupZilla::urlEnter()
{
    if (m_locationBar->text().isEmpty())
        return;
    loadAddress(QUrl(WebView::guessUrlFromString(m_locationBar->text())));
    weView()->setFocus();
}

void QupZilla::showCookieManager()
{
    CookieManager* m = p_mainApp->cookieManager();
    m->refreshTable();
    m->show();
}

void QupZilla::showHistoryManager()
{
    HistoryManager* m = p_mainApp->historyManager();
    m->refreshTable();
    m->setMainWindow(this);
    m->show();
}

void QupZilla::showRSSManager()
{
    RSSManager* m = p_mainApp->rssManager();
    m->refreshTable();
    m->setMainWindow(this);
    m->show();
}

void QupZilla::showBookmarksManager()
{
    BookmarksManager* m = p_mainApp->bookmarksManager();
    m->refreshTable();
    m->setMainWindow(this);
    m->show();
}

void QupZilla::showClearPrivateData()
{
    ClearPrivateData clear(this, this);
    clear.exec();
}

void QupZilla::showDownloadManager()
{
    MainApplication::getInstance()->downManager()->show();
}

void QupZilla::showPreferences()
{
    bool flashIconVisibility = m_allowFlashIcon->isVisible();
    Preferences prefs(this, this);
    prefs.exec();

    if (flashIconVisibility != m_allowFlashIcon->isVisible())
        emit message(MainApplication::ShowFlashIcon, m_allowFlashIcon->isVisible());
}

void QupZilla::showSource()
{
    SourceViewer* source = new SourceViewer(this);
    source->setAttribute(Qt::WA_DeleteOnClose);
    source->show();
}

void QupZilla::showPageInfo()
{
    SiteInfo* info = new SiteInfo(this, this);
    info->setAttribute(Qt::WA_DeleteOnClose);
    info->show();
}

void QupZilla::showBookmarksToolbar()
{
    if (m_bookmarksToolbar->isVisible()) {
        m_bookmarksToolbar->setVisible(false);
        m_actionShowBookmarksToolbar->setChecked(false);
    }else{
        m_bookmarksToolbar->setVisible(true);
        m_actionShowBookmarksToolbar->setChecked(true);
    }
}

void QupZilla::showNavigationToolbar()
{
    if (!menuBar()->isVisible() && !m_actionShowToolbar->isChecked())
        showMenubar();

    if (m_navigation->isVisible()) {
        m_navigation->setVisible(false);
        m_actionShowToolbar->setChecked(false);
    }else{
        m_navigation->setVisible(true);
        m_actionShowToolbar->setChecked(true);
    }
}

void QupZilla::showMenubar()
{
    if (!m_navigation->isVisible() && !m_actionShowMenubar->isChecked())
        showNavigationToolbar();

    menuBar()->setVisible(!menuBar()->isVisible());
    m_navigation->actions().at(m_navigation->actions().count()-2)->setVisible(!menuBar()->isVisible());
    m_actionShowMenubar->setChecked(menuBar()->isVisible());
}

void QupZilla::showStatusbar()
{
    if (statusBar()->isVisible()) {
        statusBar()->setVisible(false);
        m_actionShowStatusbar->setChecked(false);
    }else{
        statusBar()->setVisible(true);
        m_actionShowStatusbar->setChecked(true);
    }
}

void QupZilla::showInspector()
{
    if (!m_webInspectorDock) {
        m_webInspectorDock = new QDockWidget(this);
        if (m_webInspector)
            delete m_webInspector;
        m_webInspector = new QWebInspector(this);
        m_webInspector->setPage(weView()->page());
        addDockWidget(Qt::BottomDockWidgetArea, m_webInspectorDock);
        m_webInspectorDock->setWindowTitle(tr("Web Inspector"));
        m_webInspectorDock->setObjectName("WebInspector");
        m_webInspectorDock->setWidget(m_webInspector);
        m_webInspectorDock->setFeatures(QDockWidget::DockWidgetClosable);
        m_webInspectorDock->setContextMenuPolicy(Qt::CustomContextMenu);
    } else if (m_webInspectorDock->isVisible()) { //Next tab
        m_webInspectorDock->show();
        m_webInspector->setPage(weView()->page());
        m_webInspectorDock->setWidget(m_webInspector);
    } else { //Showing hidden dock
        m_webInspectorDock->show();
        if (m_webInspector->page() != weView()->page()) {
            m_webInspector->setPage(weView()->page());
            m_webInspectorDock->setWidget(m_webInspector);
        }
    }
}

void QupZilla::aboutQupZilla()
{
    AboutDialog about(this);
    about.exec();
}

void QupZilla::searchOnPage()
{
    if (!m_webSearchToolbar) {
        m_webSearchToolbar = new SearchToolBar(this);
        addToolBar(Qt::BottomToolBarArea, m_webSearchToolbar);
        m_webSearchToolbar->showBar();
        return;
    }
    if (m_webSearchToolbar->isVisible()) {
        m_webSearchToolbar->hideBar();
        weView()->setFocus();
    }else{
        m_webSearchToolbar->showBar();
    }
}

void QupZilla::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open file..."), QDir::homePath(), "(*.html *.htm *.jpg *.png)");
    if (!filePath.isEmpty())
        loadAddress(QUrl(filePath));
}

void QupZilla::fullScreen(bool make)
{
    if (make) {
        m_menuBarVisible = menuBar()->isVisible();
        m_statusBarVisible = statusBar()->isVisible();
        setWindowState(windowState() | Qt::WindowFullScreen);
        menuBar()->hide();
        statusBar()->hide();
    }else{
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        if (m_menuBarVisible)
            showMenubar();
        if (m_statusBarVisible)
            showStatusbar();
    }
    m_actionShowFullScreen->setChecked(make);
    m_actionExitFullscreen->setVisible(make);
}

void QupZilla::savePage()
{
    QNetworkRequest request(weView()->url());

    DownloadManager* dManager = MainApplication::getInstance()->downManager();
    dManager->download(request);
}

void QupZilla::printPage()
{
    QPrintPreviewDialog* dialog = new QPrintPreviewDialog(this);
    connect(dialog, SIGNAL(paintRequested(QPrinter*)), weView(), SLOT(print(QPrinter*)));
    dialog->exec();
    delete dialog;
}

void QupZilla::startPrivate(bool state)
{
    if (state) {
        QString title = tr("Are you sure you want to turn on private browsing?");
        QString text1 = tr("When private browsing is turned on, some actions concerning your privacy will be disabled:");

        QStringList actions;
        actions.append(tr("Webpages are not added to the history."));
        actions.append(tr("New cookies are not stored, but current cookies can be accessed."));
        actions.append(tr("Your session won't be stored."));

        QString text2 = tr("Until you close the window, you can still click the Back and Forward "
                                   "buttons to return to the webpages you have opened.");

        QString message = QString(QLatin1String("<b>%1</b><p>%2</p><ul><li>%3</li></ul><p>%4</p>")).arg(title, text1, actions.join(QLatin1String("</li><li>")), text2);

        QMessageBox::StandardButton button = QMessageBox::question(this, tr("Start Private Browsing"),
                                                                   message, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (button != QMessageBox::Yes)
            return;
    }
    p_mainApp->webSettings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, state);
    p_mainApp->history()->setSaving(!state);
    p_mainApp->cookieJar()->setAllowCookies(!state);
    emit message(MainApplication::CheckPrivateBrowsing, state);
}

void QupZilla::closeEvent(QCloseEvent* event)
{
    if (p_mainApp->isClosing())
        return;

    QSettings settings(m_activeProfil+"settings.ini", QSettings::IniFormat);
    settings.beginGroup("Web-URL-Settings");

    if (settings.value("afterLaunch",0).toInt()!=2 && m_tabWidget->count()>1) {
        QMessageBox::StandardButton button = QMessageBox::warning(this, tr("There are still open tabs"),
                             tr("There are still %1 open tabs and your session won't be stored. Are you sure to quit?").arg(m_tabWidget->count()), QMessageBox::Yes | QMessageBox::No);
        if (button != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }
    settings.endGroup();

    p_mainApp->cookieJar()->saveCookies();
    p_mainApp->saveStateSlot();
    p_mainApp->aboutToCloseWindow(this);

    this->~QupZilla();
    event->accept();
}

void QupZilla::quitApp()
{
    QSettings settings(m_activeProfil+"settings.ini", QSettings::IniFormat);
    settings.beginGroup("Web-URL-Settings");

    if (settings.value("afterLaunch",0).toInt()!=2 && m_tabWidget->count()>1) {
        QMessageBox::StandardButton button = QMessageBox::warning(this, tr("There are still open tabs"),
                             tr("There are still %1 open tabs and your session won't be stored. Are you sure to quit?").arg(m_tabWidget->count()), QMessageBox::Yes | QMessageBox::No);
        if (button != QMessageBox::Yes)
            return;
    }
    settings.endGroup();

    p_mainApp->quitApplication();
}

QupZilla::~QupZilla()
{
    delete m_tabWidget;
    delete m_privateBrowsing;
    delete m_allowFlashIcon;
    delete m_menuBack;
    delete m_menuForward;
    delete m_locationBar;
    delete m_searchLine;
    delete m_bookmarksToolbar;
    delete m_webSearchToolbar;
    delete m_buttonBack;
    delete m_buttonNext;
    delete m_buttonHome;
    delete m_buttonStop;
    delete m_buttonReload;
    delete m_actionExitFullscreen;
    delete m_navigationSplitter;
    delete m_navigation;
    delete m_progressBar;

    if (m_webInspectorDock) {
        delete m_webInspector;
        delete m_webInspectorDock;
    }
}