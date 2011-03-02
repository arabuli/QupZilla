#include "preferences.h"
#include "ui_preferences.h"
#include "qupzilla.h"
#include "bookmarkstoolbar.h"
#include "historymodel.h"
#include "tabwidget.h"
#include "cookiejar.h"
#include "locationbar.h"
#include "autofillmanager.h"
#include "mainapplication.h"
#include "cookiemanager.h"
#include "pluginslist.h"
#include "qtwin.h"
#include "pluginproxy.h"

Preferences::Preferences(QupZilla* mainClass, QWidget *parent) :
    QDialog(parent)
    ,ui(new Ui::Preferences)
    ,p_QupZilla(mainClass)
    ,m_pluginsList(0)
{
    ui->setupUi(this);
    m_bgLabelSize = this->sizeHint();

    QSettings settings(MainApplication::getInstance()->getActiveProfil()+"settings.ini", QSettings::IniFormat);
    //GENERAL URLs
    settings.beginGroup("Web-URL-Settings");
    m_homepage = settings.value("homepage","http://qupzilla.ic.cz/search/").toString();
    m_newTabUrl = settings.value("newTabUrl","").toString();
    ui->homepage->setText(m_homepage);
    ui->newTabUrl->setText(m_newTabUrl);
    int afterLaunch = settings.value("afterLaunch",1).toInt();
    ui->afterLaunch->setCurrentIndex(afterLaunch);

    ui->newTabFrame->setVisible(false);
    if (m_newTabUrl.isEmpty())
        ui->newTab->setCurrentIndex(0);
    else if (m_newTabUrl == m_homepage)
        ui->newTab->setCurrentIndex(1);
    else{
        ui->newTab->setCurrentIndex(2);
        ui->newTabFrame->setVisible(true);
    }
    connect(ui->newTab, SIGNAL(currentIndexChanged(int)), this, SLOT(newTabChanged()));
    connect(ui->useActualBut, SIGNAL(clicked()), this, SLOT(useActualHomepage()));
    connect(ui->newTabUseActual, SIGNAL(clicked()), this, SLOT(useActualNewTab()));

    //PROFILES
    ui->startPRofile->setEnabled(false);
    ui->createProfile->setEnabled(false);
    ui->deleteProfile->setEnabled(false);
    settings.endGroup();

    //WINDOW
    settings.beginGroup("Browser-View-Settings");
    ui->showStatusbar->setChecked( settings.value("showStatusBar",true).toBool() );
    ui->showBookmarksToolbar->setChecked( p_QupZilla->bookmarksToolbar()->isVisible() );
    ui->showNavigationToolbar->setChecked( p_QupZilla->navigationToolbar()->isVisible() );
    ui->showHome->setChecked( settings.value("showHomeButton",true).toBool() );
    ui->showBackForward->setChecked( settings.value("showBackForwardButtons",true).toBool() );
    if (settings.value("useTransparentBackground",false).toBool())
        ui->useTransparentBg->setChecked(true);
    else
        ui->useBgImage->setChecked(true);

    m_menuTextColor = settings.value("menuTextColor", QColor(Qt::black)).value<QColor>();
    ui->textColor->setStyleSheet("color: "+m_menuTextColor.name()+";");
    useBgImageChanged(ui->useBgImage->isChecked());
    settings.endGroup();
#ifdef Q_WS_WIN
    ui->useTransparentBg->setEnabled(QtWin::isCompositionEnabled());
#endif
    connect(ui->useBgImage, SIGNAL(toggled(bool)), this, SLOT(useBgImageChanged(bool)));
    connect(ui->backgroundButton, SIGNAL(clicked()), this, SLOT(chooseBackgroundPath()));
    connect(ui->resetDefaultBgButton, SIGNAL(clicked()), this, SLOT(resetBackground()));
    connect(ui->textColorChooser, SIGNAL(clicked()), this, SLOT(chooseColor()));
    updateBgLabel();

    //TABS
    settings.beginGroup("Browser-Tabs-Settings");
    ui->makeMovable->setChecked( settings.value("makeTabsMovable",true).toBool() );
    ui->hideCloseOnTab->setChecked( settings.value("hideCloseButtonWithOneTab",false).toBool() );
    ui->hideTabsOnTab->setChecked( settings.value("hideTabsWithOneTab",false).toBool() );
    ui->activateLastTab->setChecked( settings.value("ActivateLastTabWhenClosingActual", false).toBool() );
    settings.endGroup();
    //AddressBar
    settings.beginGroup("AddressBar");
    ui->selectAllOnFocus->setChecked( settings.value("SelectAllTextOnDoubleClick",true).toBool() );
    ui->addComWithCtrl->setChecked( settings.value("AddComDomainWithCtrlKey",false).toBool() );
    ui->addCountryWithAlt->setChecked( settings.value("AddCountryDomainWithAltKey",true).toBool() );
    settings.endGroup();

    //BROWSING
    settings.beginGroup("Web-Browser-Settings");
    ui->allowPlugins->setChecked( settings.value("allowFlash",true).toBool() );
    ui->allowJavaScript->setChecked( settings.value("allowJavaScript",true).toBool() );
    ui->blockPopup->setChecked( !settings.value("allowJavaScriptOpenWindow", false).toBool() );
    ui->allowJava->setChecked( settings.value("allowJava",true).toBool() );
    ui->loadImages->setChecked( settings.value("autoLoadImages",true).toBool() );
    ui->allowDNSPrefetch->setChecked( settings.value("DNS-Prefetch", false).toBool() );
    ui->jscanAccessClipboard->setChecked( settings.value("JavaScriptCanAccessClipboard", true).toBool() );
    ui->linksInFocusChain->setChecked( settings.value("IncludeLinkInFocusChain", false).toBool() );
    ui->zoomTextOnly->setChecked( settings.value("zoomTextOnly", false).toBool() );
    ui->printEBackground->setChecked( settings.value("PrintElementBackground", true).toBool() );
    ui->wheelScroll->setValue( settings.value("wheelScrollLines", qApp->wheelScrollLines()).toInt() );

    if (!ui->allowJavaScript->isChecked())
        ui->blockPopup->setEnabled(false);
    connect(ui->allowJavaScript, SIGNAL(toggled(bool)), this, SLOT(allowJavaScriptChanged(bool)));
    //Cache
    ui->pagesInCache->setValue( settings.value("maximumCachedPages",3).toInt() );
    connect(ui->pagesInCache, SIGNAL(valueChanged(int)), this, SLOT(pageCacheValueChanged(int)));
    ui->pageCacheLabel->setText(QString::number(ui->pagesInCache->value()));

    ui->allowCache->setChecked( settings.value("AllowLocalCache",true).toBool() );
    ui->cacheMB->setValue( settings.value("LocalCacheSize", 50).toInt() );
    ui->MBlabel->setText( settings.value("LocalCacheSize", 50).toString() + " MB");
    connect(ui->allowCache, SIGNAL(clicked(bool)), this, SLOT(allowCacheChanged(bool)));
    connect(ui->cacheMB, SIGNAL(valueChanged(int)), this, SLOT(cacheValueChanged(int)) );
    allowCacheChanged(ui->allowCache->isChecked());

    //PASSWORD MANAGER
    ui->allowPassManager->setChecked(settings.value("AutoFillForms",true).toBool());
    connect(ui->allowPassManager, SIGNAL(toggled(bool)), this, SLOT(showPassManager(bool)));

    m_autoFillManager = new AutoFillManager(this);
    ui->autoFillFrame->addWidget(m_autoFillManager);

    //PRIVACY
    //Web storage
    ui->storeIcons->setChecked( settings.value("allowPersistentStorage",true).toBool() );
    ui->saveHistory->setChecked( p_QupZilla->getMainApp()->history()->isSaving() );
    ui->deleteHistoryOnClose->setChecked( settings.value("deleteHistoryOnClose",false).toBool() );
    if (!ui->saveHistory->isChecked())
        ui->deleteHistoryOnClose->setEnabled(false);
    connect(ui->saveHistory, SIGNAL(toggled(bool)), this, SLOT(saveHistoryChanged(bool)));
    //Cookies
    ui->saveCookies->setChecked( settings.value("allowCookies",true).toBool() );
    if (!ui->saveCookies->isChecked())
        ui->deleteCookiesOnClose->setEnabled(false);
    connect(ui->saveCookies, SIGNAL(toggled(bool)), this, SLOT(saveCookiesChanged(bool)));
    ui->deleteCookiesOnClose->setChecked( settings.value("deleteCookiesOnClose", false).toBool() );
    ui->matchExactly->setChecked( settings.value("allowCookiesFromVisitedDomainOnly",false).toBool() );
    ui->filterTracking->setChecked( settings.value("filterTrackingCookie",false).toBool() );
    settings.endGroup();

    //DOWNLOADS
    settings.beginGroup("DownloadManager");
    ui->downLoc->setText( settings.value("defaultDownloadPath","").toString() );
    if (ui->downLoc->text().isEmpty())
        ui->askEverytime->setChecked(true);
    else
        ui->useDefined->setChecked(true);
    connect(ui->useDefined, SIGNAL(toggled(bool)), this, SLOT(downLocChanged(bool)));
    ui->closeDownDialogOnFinish->setChecked( settings.value("autoCloseOnFinish",false).toBool() );
    connect(ui->downButt, SIGNAL(clicked()), this, SLOT(chooseDownPath()));
    downLocChanged(ui->useDefined->isChecked());
    settings.endGroup();

    //PLUGINS
    m_pluginsList = new PluginsList(this);
    ui->pluginsFrame->addWidget(m_pluginsList);

    //OTHER
    //Languages
    QString activeLanguage="";
    if (!p_QupZilla->activeLanguage().isEmpty()) {
        activeLanguage = p_QupZilla->activeLanguage();
        QString loc = activeLanguage;
        loc.remove(".qm");
        QLocale locale(loc);
        QString country = QLocale::countryToString(locale.country());
        QString language = QLocale::languageToString(locale.language());
        ui->languages->addItem(language+", "+country+" ("+loc+")", activeLanguage);
    }
    ui->languages->addItem("English (en_US)");

    QDir lanDir(MainApplication::getInstance()->DATADIR+"locale");
    QStringList list = lanDir.entryList(QStringList("*.qm"));
    foreach(QString name, list) {
        if (name.startsWith("qt_") || name == activeLanguage)
            continue;

        QString loc = name;
        loc.remove(".qm");
        QLocale locale(loc);
        QString country = QLocale::countryToString(locale.country());
        QString language = QLocale::languageToString(locale.language());
        ui->languages->addItem(language+", "+country+" ("+loc+")", name);
    }

    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
    connect(ui->cookieManagerBut, SIGNAL(clicked()), this, SLOT(showCookieManager()));

    connect(ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(showStackedPage(QListWidgetItem*)));
    ui->listWidget->setItemSelected(ui->listWidget->itemAt(5,5), true);

    ui->version->setText(" QupZilla v"+QupZilla::VERSION);
}

void Preferences::showStackedPage(QListWidgetItem *item)
{
    if (!item)
        return;
   ui->caption->setText("<b>"+item->text()+"</b>");
   ui->stackedWidget->setCurrentIndex(item->whatsThis().toInt());
}

void Preferences::chooseColor()
{
    m_menuTextColor = QColorDialog::getColor(Qt::black, this);
    ui->textColor->setStyleSheet("color: "+m_menuTextColor.name()+";");
}

void Preferences::allowCacheChanged(bool state)
{
    ui->cacheFrame->setEnabled(state);
    ui->cacheMB->setEnabled(state);
}

void Preferences::useActualHomepage()
{
    ui->homepage->setText(p_QupZilla->weView()->url().toString());
}

void Preferences::useActualNewTab()
{
    ui->newTabUrl->setText(p_QupZilla->weView()->url().toString());
}

void Preferences::resetBackground()
{
    QFile::remove(p_QupZilla->activeProfil()+"background.png");
    QFile(MainApplication::getInstance()->DATADIR+"data/default/profiles/default/background.png").copy(p_QupZilla->activeProfil()+"background.png");

    m_menuTextColor = QColor(Qt::black);
    ui->textColor->setStyleSheet("color: "+m_menuTextColor.name()+";");

    updateBgLabel();
}

void Preferences::updateBgLabel()
{
    ui->bgLabel->setStyleSheet("#bgLabel {background: url("+p_QupZilla->activeProfil()+"background.png) top right;}");
}

void Preferences::chooseDownPath()
{
    QString userFileName = QFileDialog::getExistingDirectory(p_QupZilla, tr("Choose download location..."), QDir::homePath());
    if (userFileName.isEmpty())
        return;
    ui->downLoc->setText(userFileName);
}

void Preferences::chooseBackgroundPath()
{
    QString file = QFileDialog::getOpenFileName(p_QupZilla, tr("Choose background location..."), QDir::homePath(), "*.png");
    if (file.isEmpty())
        return;
    QFile::remove(p_QupZilla->activeProfil()+"background.png");
    QFile(file).copy(p_QupZilla->activeProfil()+"background.png");

    updateBgLabel();
}

void Preferences::newTabChanged()
{
    if (ui->newTab->currentIndex() == 2)
        ui->newTabFrame->setVisible(true);
    else
        ui->newTabFrame->setVisible(false);
}

void Preferences::useBgImageChanged(bool state)
{
    ui->bgLabel->setEnabled(state);
}

void Preferences::downLocChanged(bool state)
{
    ui->downButt->setEnabled(state);
    ui->downLoc->setEnabled(state);
}

void Preferences::allowJavaScriptChanged(bool stat)
{
    ui->blockPopup->setEnabled(stat);
}

void Preferences::saveHistoryChanged(bool stat)
{
    ui->deleteHistoryOnClose->setEnabled(stat);
}

void Preferences::saveCookiesChanged(bool stat)
{
    ui->deleteCookiesOnClose->setEnabled(stat);
}

void Preferences::showCookieManager()
{
    CookieManager* m = new CookieManager();
    m->refreshTable();
    m->setAttribute(Qt::WA_DeleteOnClose);
    m->setWindowModality(Qt::WindowModal);
    m->show();
}

void Preferences::cacheValueChanged(int value)
{
    ui->MBlabel->setText(QString::number(value) + " MB");
    if (value == 0) {
        ui->allowCache->setChecked(false);
        allowCacheChanged(false);
    }
    else if (!ui->allowCache->isChecked()) {
        ui->allowCache->setChecked(true);
        allowCacheChanged(true);
    }
}

void Preferences::pageCacheValueChanged(int value)
{
    ui->pageCacheLabel->setText(QString::number(value));
}

void Preferences::showPassManager(bool state)
{
    m_autoFillManager->setVisible(state);
}

void Preferences::buttonClicked(QAbstractButton *button)
{
    switch (ui->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
        saveSettings();
        break;

    case QDialogButtonBox::RejectRole:
        close();
        break;

    case QDialogButtonBox::AcceptRole:
        saveSettings();
        close();
        break;

    default:
        break;
    }
}

void Preferences::saveSettings()
{
    QSettings settings(MainApplication::getInstance()->getActiveProfil()+"settings.ini", QSettings::IniFormat);
    //GENERAL URLs
    settings.beginGroup("Web-URL-Settings");
    settings.setValue("homepage",ui->homepage->text());

    QString homepage = ui->homepage->text();
    settings.setValue("afterLaunch",ui->afterLaunch->currentIndex() );


    if (ui->newTab->currentIndex() == 0)
        settings.setValue("newTabUrl","");
    else if (ui->newTab->currentIndex() == 1)
        settings.setValue("newTabUrl",homepage);
    else
        settings.setValue("newTabUrl",ui->newTabUrl->text());

    settings.endGroup();
    //PROFILES
    /*
     *
     *
     *
     */

    //WINDOW
    settings.beginGroup("Browser-View-Settings");

    settings.setValue("showStatusbar",ui->showStatusbar->isChecked());
    settings.setValue("showBookmarksToolbar", ui->showBookmarksToolbar->isChecked());
    settings.setValue("showNavigationToolbar", ui->showNavigationToolbar->isChecked());
    settings.setValue("showHomeButton", ui->showHome->isChecked());
    settings.setValue("showBackForwardButtons",ui->showBackForward->isChecked());
    settings.setValue("useTransparentBackground", ui->useTransparentBg->isChecked());
    settings.setValue("menuTextColor", m_menuTextColor);
    settings.endGroup();

    //TABS
    settings.beginGroup("Browser-Tabs-Settings");
    settings.setValue("makeTabsMovable",ui->makeMovable->isChecked() );
    settings.setValue("hideCloseButtonWithOneTab",ui->hideCloseOnTab->isChecked());
    settings.setValue("hideTabsWithOneTab",ui->hideTabsOnTab->isChecked() );
    settings.setValue("ActivateLastTabWhenClosingActual", ui->activateLastTab->isChecked());
    settings.endGroup();
    //Downloads
    settings.beginGroup("DownloadManager");
    if (ui->askEverytime->isChecked())
        settings.setValue("defaultDownloadPath","");
    else{
        QString text = ui->downLoc->text();
        if (!text.endsWith("/"))
            text+="/";
        settings.setValue("defaultDownloadPath",text);
    }
    settings.setValue("autoCloseOnFinish",ui->closeDownDialogOnFinish->isChecked());
    settings.endGroup();

    //BROWSING
    settings.beginGroup("Web-Browser-Settings");
    settings.setValue("allowFlash",ui->allowPlugins->isChecked());
    settings.setValue("allowJavaScript",ui->allowJavaScript->isChecked());
    settings.setValue("allowJavaScriptOpenWindow", !ui->blockPopup->isChecked());
    settings.setValue("allowJava",ui->allowJava->isChecked());
    settings.setValue("autoLoadImages",ui->loadImages->isChecked());
    settings.setValue("maximumCachedPages",ui->pagesInCache->value());
    settings.setValue("DNS-Prefetch", ui->allowDNSPrefetch->isChecked());
    settings.setValue("JavaScriptCanAccessClipboard", ui->jscanAccessClipboard->isChecked());
    settings.setValue("IncludeLinkInFocusChain", ui->linksInFocusChain->isChecked());
    settings.setValue("zoomTextOnly", ui->zoomTextOnly->isChecked());
    settings.setValue("PrintElementBackground", ui->printEBackground->isChecked());
    settings.setValue("wheelScrollLines", ui->wheelScroll->value());
    //Cache
    settings.setValue("AllowLocalCache", ui->allowCache->isChecked());
    settings.setValue("LocalCacheSize", ui->cacheMB->value());

    //PRIVACY
    //Web storage
    settings.setValue("allowPersistentStorage", ui->storeIcons->isChecked());
    //ui->saveHistory->setChecked( p_QupZilla->history->isSaving() );
    settings.setValue("deleteHistoryOnClose",ui->deleteHistoryOnClose->isChecked());

    //Cookies
    settings.setValue("allowCookies",ui->saveCookies->isChecked());
    settings.setValue("deleteCookiesOnClose", ui->deleteCookiesOnClose->isChecked());
    settings.setValue("allowCookiesFromVisitedDomainOnly",ui->matchExactly->isChecked() );
    settings.setValue("filterTrackingCookie",ui->filterTracking->isChecked() );
    settings.endGroup();

    //OTHER
    //AddressBar
    settings.beginGroup("AddressBar");
    settings.setValue("SelectAllTextOnDoubleClick",ui->selectAllOnFocus->isChecked() );
    settings.setValue("AddComDomainWithCtrlKey",ui->addComWithCtrl->isChecked() );
    settings.setValue("AddCountryDomainWithAltKey", ui->addCountryWithAlt->isChecked() );
    settings.endGroup();
    //Languages
    settings.beginGroup("Browser-Window-Settings");
    settings.setValue("language",ui->languages->itemData(ui->languages->currentIndex()).toString());
    settings.endGroup();

    m_pluginsList->save();
    p_QupZilla->loadSettings();
    p_QupZilla->tabWidget()->loadSettings();
    p_QupZilla->getMainApp()->cookieJar()->loadSettings();
    p_QupZilla->getMainApp()->history()->loadSettings();
    p_QupZilla->locationBar()->loadSettings();
    MainApplication::getInstance()->loadSettings();
    MainApplication::getInstance()->plugins()->c2f_saveSettings();
}

Preferences::~Preferences()
{
    qDebug() << __FUNCTION__ << "called";
    delete ui;
    delete m_autoFillManager;
    delete m_pluginsList;
}