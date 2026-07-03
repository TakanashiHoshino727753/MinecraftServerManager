#include "MainWindow.h"
#include "ServerListPanel.h"
#include "CreateWizard.h"
#include "ServerDetailPanel.h"
#include "SettingsDialog.h"
#include "JavaPanel.h"
#include "Theme.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QApplication>
#include <QFrame>
#include <QStyle>
#include <QEvent>
#include <QMouseEvent>
#include <QSpacerItem>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#endif

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // 无边框窗口 (必须在 setupUi 之前设置)
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint |
                   Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);

    m_store      = new ConfigStore(this);
    m_process    = new ServerProcess(this);
    m_downloader = new DownloadManager(this);

    // Load app settings
    m_appSettings.load();

    // Initialize language from saved settings
    {
        auto* lm = LanguageManager::instance();
        lm->setLanguageCode(m_appSettings.data.language);
        lm->setWebUILangCode(m_appSettings.data.webUILang);
    }

    if (m_appSettings.data.webApiEnabled) {
        m_webApi = new WebApiServer(m_store, m_process, m_downloader, &m_appSettings, this);
        m_webApi->start(m_appSettings.data.webApiPort, m_appSettings.data.webApiToken);
    }

    initUi();
    resize(1000, 750);
    setMinimumSize(780, 600);

    // ── Title bar icon: reuse app icon from qApp ──
    ui.titleIcon->setPixmap(qApp->windowIcon().pixmap(24, 24));

    // ── Apply dynamic theme based on saved settings ──
    qApp->setStyleSheet(dynamicAppStyleSheet(
        m_appSettings.data.themeColor,
        m_appSettings.data.bgStyle,
        m_appSettings.data.customBgPath));

    // 无边框 DWM 阴影
    {
        HWND hwnd = reinterpret_cast<HWND>(winId());
        MARGINS margins = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea(hwnd, &margins);
    }

    connect(m_process, &ServerProcess::stateChanged, this, &MainWindow::onProcessStateChanged);
}

// 窗口状态变化时更新最大化按钮文字
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMaximized())
            ui.btnMaximize->setIcon(QIcon(":/icons/restore.svg"));
        else
            ui.btnMaximize->setIcon(QIcon(":/icons/square.svg"));
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::initUi() {
    // Load UI from .ui file
    ui.setupUi(this);

    // 标题栏拖拽/双击最大化
    ui.titleBar->installEventFilter(this);

    // 侧栏不拉伸，右侧内容区填满剩余空间
    ui.bodyLayout->setStretch(0, 0);
    ui.bodyLayout->setStretch(1, 1);

    // Hide tab bar initially
    ui.tabBar->hide();

    // Connect title bar buttons
    connect(ui.btnMinimize, &QPushButton::clicked, this, [this]() { showMinimized(); });
    connect(ui.btnMaximize, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) { showNormal(); ui.btnMaximize->setIcon(QIcon(":/icons/square.svg")); }
        else              { showMaximized(); ui.btnMaximize->setIcon(QIcon(":/icons/restore.svg")); }
    });
    connect(ui.btnClose, &QPushButton::clicked, this, [this]() { close(); });

    // Sidebar navigation
    connect(ui.btnServers, &QPushButton::clicked, this, &MainWindow::switchToServers);
    connect(ui.btnCreate,  &QPushButton::clicked, this, &MainWindow::switchToCreate);
    ui.btnServers->setObjectName("sidebarNavBtnActive");
    ui.btnCreate->setObjectName("sidebarNavBtn");

    // Settings button (insert before stretch in sidebar layout)
    m_btnSettings = new QPushButton("  ⚙ Settings");
    m_btnSettings->setObjectName("sidebarNavBtn");
    m_btnSettings->setCursor(Qt::PointingHandCursor);
    connect(m_btnSettings, &QPushButton::clicked, this, &MainWindow::openSettingsDialog);
    int insertPos = ui.sidebarLayout->indexOf(ui.btnCreate) + 1;
    ui.sidebarLayout->insertWidget(insertPos, m_btnSettings);

    // Tab bar
    connect(ui.tabBar, &QTabBar::tabBarClicked, this, &MainWindow::onTabClicked);
    connect(ui.tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    // Create content panels
    m_listPanel   = new ServerListPanel(m_store, this);
    m_createPanel = new CreateWizard(m_store, m_downloader, this);
    m_detailPanel = new ServerDetailPanel(m_store, m_process, this);

    ui.stack->addWidget(m_listPanel);   // index 0
    ui.stack->addWidget(m_createPanel); // index 1
    ui.stack->addWidget(m_detailPanel); // index 2

    // Signal connections
    connect(m_listPanel,   &ServerListPanel::serverSelected, this, &MainWindow::onServerSelected);
    connect(m_listPanel,   &ServerListPanel::serverDoubleClicked, this, &MainWindow::onServerSelected);
    connect(m_listPanel,   &ServerListPanel::createRequested, this, &MainWindow::switchToCreate);
    connect(m_listPanel,   &ServerListPanel::serverControlRequested, this, &MainWindow::onServerControlRequested);
    connect(m_listPanel,   &ServerListPanel::serverConfigRequested, this, &MainWindow::onServerConfigRequested);
    connect(m_createPanel, &CreateWizard::created, this, &MainWindow::onServerCreated);
    connect(m_detailPanel, &ServerDetailPanel::backRequested, this, &MainWindow::onBackToList);
    connect(m_store,       &ConfigStore::serversChanged, this, &MainWindow::refreshServerCounts);
    connect(m_store,       &ConfigStore::serversChanged, this, &MainWindow::cleanupTabs);

    // Apply stylesheet objects
    ui.titleIcon->setObjectName("titleIcon");
    ui.titleVersion->setObjectName("titleVersion");
    ui.btnMinimize->setObjectName("winBtn");
    ui.btnMaximize->setObjectName("winBtn");
    ui.btnClose->setObjectName("winBtnClose");
    ui.totalCountLabel->setObjectName("sidebarStatLabel");
    ui.runningCountLabel->setObjectName("sidebarStatLabel");
    ui.sidebarSection->setObjectName("sidebarSection");
    ui.sidebarFooterText->setObjectName("sidebarFooterText");
    ui.totalCount->setStyleSheet("color: #a29bfe; font-size: 18px; font-weight: bold;");
    ui.runningCount->setStyleSheet("color: #00d2a0; font-size: 18px; font-weight: bold;");
    ui.runningDot->setStyleSheet("background: #00d2a0; border-radius: 3px;");

    // Draw lucide-react-style icons for Settings & Close
    createButtonIcons();

    refreshServerCounts();
    switchToServers();

    // System tray icon — created at startup
    createTrayIcon();
    m_trayIcon->show();

    // Apply language to UI
    applyLanguage();
}

// ── Language ──

void MainWindow::applyLanguage() {
    auto* lm = LanguageManager::instance();

    // Sidebar
    ui.sidebarSection->setText(lm->t("sidebar.section"));
    ui.totalCountLabel->setText(lm->t("sidebar.total"));
    ui.runningCountLabel->setText(lm->t("sidebar.running"));
    ui.btnServers->setText(lm->t("sidebar.servers"));
    ui.btnCreate->setText(lm->t("sidebar.create"));
    m_btnSettings->setText("  " + lm->t("sidebar.settings"));
    ui.sidebarFooterText->setText(lm->t("sidebar.footer"));

    setWindowTitle(lm->t("app.title"));
    ui.titleLabel->setText(lm->t("app.title"));
    ui.titleVersion->setText(lm->t("app.version"));

    // Update tray menu texts
    if (m_trayMenu) {
        QList<QAction*> actions = m_trayMenu->actions();
        if (actions.size() >= 1) actions[0]->setText(lm->t("tray.show"));
        if (actions.size() >= 2) actions[1]->setText(lm->t("tray.hide"));
        // separator at index 2
        if (actions.size() >= 4) actions[3]->setText(lm->t("tray.quit"));
    }
    if (m_trayIcon)
        m_trayIcon->setToolTip(lm->t("app.title"));
}

void MainWindow::onLanguageChanged() {
    applyLanguage();
}

// ── Navigation ──

static void resetNavButton(QPushButton* btn) {
    if (!btn) return;
    btn->setObjectName("sidebarNavBtn");
    btn->style()->unpolish(btn);
    btn->style()->polish(btn);
}

static void activateNavButton(QPushButton* btn) {
    if (!btn) return;
    btn->setObjectName("sidebarNavBtnActive");
    btn->style()->unpolish(btn);
    btn->style()->polish(btn);
}

void MainWindow::resetAllNavButtons() {
    QPushButton* btns[] = { ui.btnServers, ui.btnCreate, m_btnSettings, nullptr };
    for (int i = 0; btns[i]; i++) resetNavButton(btns[i]);
}

void MainWindow::switchToServers() {
    resetAllNavButtons();
    activateNavButton(ui.btnServers);
    ui.stack->setCurrentIndex(m_listIndex);
    hideTabBar();
}

void MainWindow::switchToCreate() {
    resetAllNavButtons();
    activateNavButton(ui.btnCreate);
    m_createPanel->reset();
    ui.stack->setCurrentIndex(m_createIndex);
    hideTabBar();
}

void MainWindow::hideTabBar() {
    if (ui.tabBar->count() > 0) {
        ui.tabBar->blockSignals(true);
        ui.tabBar->setCurrentIndex(-1);
        ui.tabBar->blockSignals(false);
    }
}

void MainWindow::onServerSelected(const QString& id) { openServerTab(id); }
void MainWindow::onServerCreated() { m_listPanel->refresh(); switchToServers(); }
void MainWindow::onBackToList() { switchToServers(); }

void MainWindow::onServerControlRequested(const QString& id) {
    bool isRunning = (m_process->state() == ServerProcess::Running);
    bool isMe = (m_process->serverId() == id);
    if (isRunning && isMe) { m_process->stop(); }
    else { ServerConfig* cfg = m_store->findServer(id); if (cfg) m_process->start(*cfg); }
}

void MainWindow::onServerConfigRequested(const QString& id) {
    m_detailPanel->loadServer(id);
    m_detailPanel->onEditConfig();
    m_listPanel->refresh();
}

// ── Tab management ──

void MainWindow::openServerTab(const QString& id) {
    for (int i = 0; i < m_openServerIds.size(); i++) {
        if (m_openServerIds[i] == id) {
            ui.tabBar->setCurrentIndex(i);
            m_detailPanel->loadServer(id);
            ui.stack->setCurrentIndex(m_detailIndex);
            ui.tabBar->setVisible(true);
            return;
        }
    }
    ServerConfig* cfg = m_store->findServer(id);
    QString tabName = cfg ? cfg->name : "Server";
    int idx = ui.tabBar->addTab(tabName);
    m_openServerIds.insert(idx, id);
    ui.tabBar->setCurrentIndex(idx);
    m_detailPanel->loadServer(id);
    ui.stack->setCurrentIndex(m_detailIndex);
    ui.tabBar->setVisible(true);
}

void MainWindow::onTabClicked(int index) {
    if (index < 0 || index >= m_openServerIds.size()) return;
    QString serverId = m_openServerIds[index];
    ui.tabBar->blockSignals(true);
    ui.tabBar->setCurrentIndex(index);
    ui.tabBar->blockSignals(false);
    m_detailPanel->loadServer(serverId);
    ui.stack->setCurrentIndex(m_detailIndex);
    ServerConfig* cfg = m_store->findServer(serverId);
    if (cfg) ui.tabBar->setTabText(index, cfg->name);
    ui.btnServers->setObjectName("sidebarNavBtnActive");
    ui.btnServers->style()->unpolish(ui.btnServers);
    ui.btnServers->style()->polish(ui.btnServers);
    ui.btnCreate->setObjectName("sidebarNavBtn");
    ui.btnCreate->style()->unpolish(ui.btnCreate);
    ui.btnCreate->style()->polish(ui.btnCreate);
}

void MainWindow::onTabCloseRequested(int index) { closeServerTabByIndex(index); }

void MainWindow::closeServerTabByIndex(int index) {
    if (index < 0 || index >= m_openServerIds.size()) return;
    int currentIndex = ui.tabBar->currentIndex();
    m_openServerIds.removeAt(index);
    ui.tabBar->removeTab(index);
    if (ui.tabBar->count() == 0) { ui.tabBar->hide(); switchToServers(); return; }
    if (index == currentIndex) {
        int newIdx = index >= ui.tabBar->count() ? ui.tabBar->count() - 1 : index;
        if (newIdx >= 0 && newIdx < m_openServerIds.size()) {
            ui.tabBar->setCurrentIndex(newIdx);
            m_detailPanel->loadServer(m_openServerIds[newIdx]);
            ui.stack->setCurrentIndex(m_detailIndex);
        }
    }
}

void MainWindow::closeServerTab(const QString& id) {
    for (int i = 0; i < m_openServerIds.size(); i++)
        if (m_openServerIds[i] == id) { closeServerTabByIndex(i); return; }
}

void MainWindow::cleanupTabs() {
    auto servers = m_store->servers();
    QSet<QString> validIds;
    for (const auto& s : servers) validIds.insert(s.id);
    QVector<int> toRemove;
    for (int i = 0; i < m_openServerIds.size(); i++)
        if (!validIds.contains(m_openServerIds[i])) toRemove.append(i);
    for (int i = toRemove.size() - 1; i >= 0; i--)
        closeServerTabByIndex(toRemove[i]);
}

void MainWindow::onProcessStateChanged(ServerProcess::State state) {
    Q_UNUSED(state);
    refreshServerCounts();
}

void MainWindow::refreshServerCounts() {
    auto servers = m_store->servers();
    ui.totalCount->setText(QString::number(servers.size()));
    bool running = (m_process->state() == ServerProcess::Running);
    ui.runningCount->setText(running ? "1" : "0");
    ui.runningDot->setVisible(running);
    m_listPanel->setRunningServerId(running ? m_process->serverId() : QString());
    m_listPanel->refresh();
}

// ═══════════════════════════════════════════
//  Frameless window: drag / double-click / resize
// ═══════════════════════════════════════════

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (qobject_cast<QPushButton*>(obj))
            return QMainWindow::eventFilter(obj, event);

        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
#ifdef Q_OS_WIN
            ReleaseCapture();
            SendMessage(reinterpret_cast<HWND>(winId()), WM_NCLBUTTONDOWN, HTCAPTION, 0);
#endif
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
        if (!qobject_cast<QPushButton*>(obj)) {
            isMaximized() ? showNormal() : showMaximized();
            return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

// ═══════════════════════════════════════════
//  Button icons — loaded from .qrc
// ═══════════════════════════════════════════

void MainWindow::createButtonIcons() {
    // ── Title bar buttons ──
    ui.btnMinimize->setText("");
    ui.btnMinimize->setIcon(QIcon(":/icons/minus.svg"));
    ui.btnMinimize->setIconSize(QSize(16, 16));

    ui.btnMaximize->setText("");
    ui.btnMaximize->setIcon(QIcon(":/icons/square.svg"));
    ui.btnMaximize->setIconSize(QSize(16, 16));

    ui.btnClose->setText("");
    ui.btnClose->setIcon(QIcon(":/icons/x.svg"));
    ui.btnClose->setIconSize(QSize(16, 16));

    // ── Settings button (gear icon) ──
    m_btnSettings->setIcon(QIcon(":/icons/gear.svg"));
    m_btnSettings->setIconSize(QSize(20, 20));
    m_btnSettings->setText("  " + TR("sidebar.settings"));
}

// ═══════════════════════════════════════════
//  System tray
// ═══════════════════════════════════════════

void MainWindow::createTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);

    // Tray icon: reuse app icon
    m_trayIcon->setIcon(qApp->windowIcon());
    m_trayIcon->setToolTip(TR("app.title"));

    // Context menu
    m_trayMenu = new QMenu(this);

    QAction* showAction = m_trayMenu->addAction(TR("tray.show"));
    connect(showAction, &QAction::triggered, this, [this]() {
        showNormal();
        activateWindow();
        raise();
    });

    QAction* hideAction = m_trayMenu->addAction(TR("tray.hide"));
    connect(hideAction, &QAction::triggered, this, [this]() { hide(); });

    m_trayMenu->addSeparator();

    QAction* quitAction = m_trayMenu->addAction(TR("tray.quit"));
    connect(quitAction, &QAction::triggered, this, [this]() {
        m_trayIcon->hide();
        QApplication::quit();
    });

    m_trayIcon->setContextMenu(m_trayMenu);

    // Double‑click tray icon → restore window
    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::DoubleClick ||
                    reason == QSystemTrayIcon::Trigger) {
                    showNormal();
                    activateWindow();
                    raise();
                }
            });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Minimize to tray instead of quitting
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

// ── Settings ──

void MainWindow::openSettingsDialog() {
    SettingsDialog dlg(&m_appSettings, this);

    // Connect language change signal
    connect(&dlg, &SettingsDialog::languageChanged, this, &MainWindow::onLanguageChanged);

    if (dlg.exec() == QDialog::Accepted) {
        // Re-apply dynamic theme (may have changed accent/bg style)
        qApp->setStyleSheet(dynamicAppStyleSheet(
            m_appSettings.data.themeColor,
            m_appSettings.data.bgStyle,
            m_appSettings.data.customBgPath));

        // Apply Web API changes
        if (m_appSettings.data.webApiEnabled) {
            if (!m_webApi) {
                m_webApi = new WebApiServer(m_store, m_process, m_downloader, &m_appSettings, this);
            }
            if (!m_webApi->isRunning() || m_webApi->port() != m_appSettings.data.webApiPort) {
                m_webApi->stop();
                m_webApi->start(m_appSettings.data.webApiPort, m_appSettings.data.webApiToken);
            }
        } else {
            if (m_webApi) {
                m_webApi->stop();
                delete m_webApi;
                m_webApi = nullptr;
            }
        }
    }
}

#ifdef Q_OS_WIN

static const int kResizeBorder = 6;

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(result);
    return QMainWindow::nativeEvent(eventType, message, result);
}

#endif
