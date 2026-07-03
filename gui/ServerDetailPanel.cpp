#include "ServerDetailPanel.h"
#include "ModManagerPanel.h"
#include "ModpackPanel.h"
#include "PlayerWorldPanel.h"
#include "core/ServerProperties.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollBar>
#include <QScrollArea>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QStyle>
#include <QDialog>
#include <QComboBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QAbstractItemView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>

ServerDetailPanel::ServerDetailPanel(ConfigStore* store, ServerProcess* proc, QWidget* parent)
    : QWidget(parent), m_store(store), m_process(proc)
{
    m_uptimeTimer = new QTimer(this);

    ui.setupUi(this);

    // Style header
    ui.header->setStyleSheet("background: transparent; padding: 20px 24px 12px 24px; border-bottom: 1px solid rgba(42,42,74,0.3);");
    ui.nameLabel->setStyleSheet("font-size:20px; font-weight:bold; color:#e0e0f0;");
    ui.statusLabel->setStyleSheet("font-size:13px; color:#8888aa;");
    ui.versionPathLabel->setStyleSheet("color:#8888aa; font-size:12px;");

    // Style back button
    ui.backBtn->setStyleSheet("QPushButton { background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3); border-radius: 8px; color: #8888aa; font-size: 18px; } QPushButton:hover { background: rgba(255,255,255,0.05); color: #e0e0f0; }");
    connect(ui.backBtn, &QPushButton::clicked, this, &ServerDetailPanel::backRequested);

    // Style start/stop button
    ui.startStopBtn->setStyleSheet("QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(0,210,160,0.8), stop:1 rgba(0,229,176,0.8)); border: none; color: white; font-weight: bold; border-radius: 12px; padding: 0px 20px; font-size: 13px; } QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(0,210,160,1), stop:1 rgba(0,229,176,1)); } QPushButton:disabled { background: #555577; color: #8888aa; }");
    connect(ui.startStopBtn, &QPushButton::clicked, this, &ServerDetailPanel::onStartStop);

    // Apply i18n to .ui strings
    ui.killBtn->setText(TR("detail.forceStop"));
    ui.deleteBtn->setText(TR("detail.deleteBtn"));
    ui.editBtn->setText(TR("detail.editBtn"));
    ui.cmdSendBtn->setText(TR("detail.send"));
    ui.consoleTitle->setText(TR("detail.console"));
    ui.statsStatusLabelTxt->setText(TR("detail.statusLabel"));
    ui.statsUptimeLabelTxt->setText(TR("detail.uptimeLabel"));
    ui.statsMemLabelTxt->setText(TR("detail.memLabel"));
    ui.statsJavaLabelTxt->setText(TR("detail.javaLabel"));

    // Style other buttons
    ui.killBtn->setVisible(false);
    ui.killBtn->setCursor(Qt::PointingHandCursor);
    ui.killBtn->setMinimumHeight(40);
    ui.killBtn->setStyleSheet(
        "QPushButton { background: rgba(255,71,87,0.12); border: 1px solid rgba(255,71,87,0.2);"
        "  border-radius: 8px; color: #ff4757; font-size: 13px; padding: 0 14px; }"
        "QPushButton:hover { background: rgba(255,71,87,0.25); }");
    connect(ui.killBtn, &QPushButton::clicked, this, &ServerDetailPanel::onKill);

    ui.deleteBtn->setStyleSheet("QPushButton { background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3); border-radius: 8px; color: #8888aa; font-size: 13px; padding: 0 14px; } QPushButton:hover { background: rgba(255,71,87,0.1); color: #ff4757; }");
    connect(ui.deleteBtn, &QPushButton::clicked, this, &ServerDetailPanel::onDelete);

    ui.editBtn->setStyleSheet("QPushButton { background: rgba(108,92,231,0.1); border: 1px solid rgba(108,92,231,0.2); border-radius: 8px; color: #a29bfe; font-size: 13px; padding: 0 14px; } QPushButton:hover { background: rgba(108,92,231,0.2); }");
    connect(ui.editBtn, &QPushButton::clicked, this, &ServerDetailPanel::onEditConfig);

    // Add Properties button dynamically (before deleteBtn in topRow)
    auto* propsBtn = new QPushButton(TR("detail.properties"));
    propsBtn->setMinimumHeight(40);
    propsBtn->setStyleSheet("QPushButton { background: rgba(34,211,238,0.1); border: 1px solid rgba(34,211,238,0.2); border-radius: 8px; color: #22d3ee; font-size: 13px; padding: 0 14px; } QPushButton:hover { background: rgba(34,211,238,0.2); }");
    connect(propsBtn, &QPushButton::clicked, this, &ServerDetailPanel::onEditProperties);
    // Insert before deleteBtn
    int deleteIdx = ui.topRow->indexOf(ui.deleteBtn);
    if (deleteIdx >= 0) ui.topRow->insertWidget(deleteIdx, propsBtn);

    // Style stats bar — reduce spacing for Chinese labels
    ui.statsWidget->setStyleSheet("background: transparent; padding: 12px 24px;");
    ui.statsLayout->setSpacing(12);
    ui.statsStatusLabelTxt->setStyleSheet("color:#8888aa; font-size:12px;");
    ui.statsUptimeLabelTxt->setStyleSheet("color:#8888aa; font-size:12px;");
    ui.statsMemLabelTxt->setStyleSheet("color:#8888aa; font-size:12px;");
    ui.statsJavaLabelTxt->setStyleSheet("color:#8888aa; font-size:12px;");
    ui.statsStatusVal->setStyleSheet("color:#00d2a0; font-size:12px; font-weight:500;");
    ui.statsUptimeVal->setStyleSheet("color:#e0e0f0; font-size:12px; font-weight:500;");
    ui.statsMemVal->setStyleSheet("color:#e0e0f0; font-size:12px; font-weight:500;");
    ui.statsJavaVal->setStyleSheet("color:#e0e0f0; font-size:12px; font-weight:500;");

    // Style quick bar
    ui.quickBar->setStyleSheet("background: transparent;");

    // Style console
    ui.consoleBox->setStyleSheet("background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3); border-radius: 12px;");
    ui.consoleHeader->setStyleSheet("background: rgba(15,15,26,0.5); border-radius: 12px 12px 0 0; padding: 10px 16px;");
    ui.consoleIcon->setStyleSheet("color:#a29bfe; font-size:14px; font-family:monospace;");
    ui.consoleTitle->setStyleSheet("color:#8888aa; font-size:12px; font-weight:500;");
    ui.logCountLabel->setStyleSheet("color:rgba(136,136,170,0.5); font-size:10px;");

    ui.consoleOutput->setStyleSheet("QTextEdit#consoleOutput { background: #050510; border: none; border-radius: 0; padding: 12px; font-family: 'JetBrains Mono','Fira Code','Consolas',monospace; font-size: 12px; color: rgba(224,224,240,0.7); }");
    ui.consoleOutput->setReadOnly(true);

    connect(ui.consoleOutput, &QTextEdit::textChanged, this, [this]() {
        if (m_autoScroll) { QScrollBar* sb = ui.consoleOutput->verticalScrollBar(); sb->setValue(sb->maximum()); }
    });
    connect(ui.consoleOutput->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int val) {
        m_autoScroll = (val >= ui.consoleOutput->verticalScrollBar()->maximum() - 60);
    });

    // Style command input
    ui.cmdInputArea->setStyleSheet("background: rgba(15,15,26,0.5); border-radius: 0 0 12px 12px; padding: 10px 16px;");
    ui.cmdPrompt->setStyleSheet("color:#a29bfe; font-size:14px; font-family:monospace;");
    ui.cmdInput->setStyleSheet("QLineEdit { background: transparent; border: none; color: #e0e0f0; font-family: 'JetBrains Mono','Fira Code',monospace; font-size: 13px; padding: 0; } QLineEdit:disabled { color: rgba(136,136,170,0.5); }");
    connect(ui.cmdInput, &QLineEdit::returnPressed, this, &ServerDetailPanel::onSendCommand);

    ui.cmdSendBtn->setStyleSheet("QPushButton { background: rgba(108,92,231,0.15); border: none; border-radius: 6px; color: #a29bfe; font-size: 12px; } QPushButton:hover { background: rgba(108,92,231,0.25); } QPushButton:disabled { background: transparent; color: #444466; }");
    connect(ui.cmdSendBtn, &QPushButton::clicked, this, &ServerDetailPanel::onSendCommand);

    // Process signals
    connect(m_process, &ServerProcess::consoleOutput, this, &ServerDetailPanel::onConsoleOutput);
    connect(m_process, &ServerProcess::stateChanged, this, &ServerDetailPanel::onStateChanged);
    connect(m_uptimeTimer, &QTimer::timeout, this, &ServerDetailPanel::tickUptime);

    // ── Quick actions bar ──
    buildQuickActions();

    // ── Command auto-complete ──
    buildCmdCompleter();

    // ── Cheat sheet button ──
    ui.cheatSheetBtn->setText(TR("detail.cheatSheet"));
    ui.cheatSheetBtn->setStyleSheet(
        "QPushButton { background: rgba(108,92,231,0.1); border: 1px solid rgba(108,92,231,0.2);"
        "  border-radius: 5px; color: #a29bfe; font-size: 12px; font-weight: 500; padding: 0 8px; }"
        "QPushButton:hover { background: rgba(108,92,231,0.25); }");
    connect(ui.cheatSheetBtn, &QPushButton::clicked, this, &ServerDetailPanel::showCheatSheet);

    // ── Wrap console + per-server panels into a QTabWidget ──
    m_tabWidget = new QTabWidget;
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: none; background: transparent; }"
        "QTabBar::tab { background: rgba(26,26,46,0.3); color: #8888aa; padding: 8px 18px;"
        "  border: 1px solid rgba(42,42,74,0.2); border-bottom: none; margin-right: 2px;"
        "  border-radius: 8px 8px 0 0; font-size: 12px; }"
        "QTabBar::tab:selected { background: rgba(26,26,46,0.6); color: #e0e0f0;"
        "  border-color: rgba(108,92,231,0.3); }"
        "QTabBar::tab:hover:!selected { background: rgba(26,26,46,0.5); color: #c0c0d0; }");

    // Remove consoleWrapper from rootLayout and add as "Console" tab
    ui.rootLayout->removeWidget(ui.consoleWrapper);
    m_tabWidget->addTab(ui.consoleWrapper, TR("detail.tabConsole"));

    // Create per-server sub-panels as additional tabs
    m_modPanel         = new ModManagerPanel(m_store);
    m_modpackPanel     = new ModpackPanel(m_store);
    m_playerWorldPanel = new PlayerWorldPanel(m_store, m_process);

    m_tabWidget->addTab(m_modPanel,         TR("detail.tabMods"));
    m_tabWidget->addTab(m_modpackPanel,     TR("detail.tabModpack"));
    m_tabWidget->addTab(m_playerWorldPanel, TR("detail.tabPlayerWorld"));

    // Add tab widget back into rootLayout (replace where consoleWrapper was)
    ui.rootLayout->addWidget(m_tabWidget);
}

void ServerDetailPanel::loadServer(const QString& id) {
    bool sameServer = (id == m_prevServerId);
    m_prevServerId = id; m_serverId = id;
    ServerConfig* cfg = m_store->findServer(id);
    if (!cfg) return;
    m_config = *cfg;

    ui.nameLabel->setText(m_config.name);
    QString capType = m_config.type; capType[0] = capType[0].toUpper();
    ui.versionPathLabel->setText(QString("Minecraft %1  |  %2  |  %3").arg(m_config.version, capType, m_config.path));
    ui.statsMemVal->setText(QString("%1M - %2M").arg(m_config.minRamMB).arg(m_config.maxRamMB));
    ui.statsJavaVal->setText(m_config.javaPath.isEmpty() ? TR("detail.default") : TR("detail.javaDefault"));

    if (!sameServer) ui.consoleOutput->clear();
    updateUI();

    // Forward to per-server sub-panels
    if (m_modPanel)         m_modPanel->loadServer(id);
    if (m_playerWorldPanel) m_playerWorldPanel->loadServer(id);
}

void ServerDetailPanel::updateUI() {
    bool running = m_process->state() == ServerProcess::Running;
    bool isOurs = m_process->serverId() == m_serverId;

    if (isOurs && running) {
        ui.statusLabel->setText(QString::fromUtf8("\u25CF ") + TR("list.running"));
        ui.statusLabel->setStyleSheet("font-size:13px; color:#00d2a0; font-weight:bold;");
        ui.statsStatusVal->setText(TR("list.running"));
        ui.statsStatusVal->setStyleSheet("color:#00d2a0; font-size:13px; font-weight:500;");
        ui.startStopBtn->setText(TR("detail.stopServer"));
        ui.startStopBtn->setStyleSheet("QPushButton { background: rgba(255,71,87,0.15); border: 1px solid rgba(255,71,87,0.2); color: #ff4757; font-weight: bold; border-radius: 12px; padding: 0px 20px; font-size: 13px; } QPushButton:hover { background: rgba(255,71,87,0.25); }");
        ui.killBtn->setVisible(true);
        ui.cmdInput->setEnabled(true);
        ui.cmdSendBtn->setEnabled(true);
        ui.cmdInput->setPlaceholderText(TR("detail.cmdPlaceholder"));
        ui.cheatSheetBtn->setEnabled(true);
        // Enable quick bar children buttons
        for (int i = 1; i < ui.quickBarLayout->count(); i++) {
            QWidget* w = ui.quickBarLayout->itemAt(i)->widget();
            if (w) w->setEnabled(true);
        }
        m_uptimeTimer->start(1000);
    } else {
        ui.statusLabel->setText(QString::fromUtf8("\u25CF ") + TR("list.stopped"));
        ui.statusLabel->setStyleSheet("font-size:13px; color:#8888aa;");
        ui.statsStatusVal->setText(TR("list.stopped"));
        ui.statsStatusVal->setStyleSheet("color:#8888aa; font-size:13px; font-weight:500;");
        ui.startStopBtn->setText(TR("detail.startServer"));
        ui.startStopBtn->setStyleSheet("QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(0,210,160,0.8), stop:1 rgba(0,229,176,0.8)); border: none; color: white; font-weight: bold; border-radius: 12px; padding: 8px 20px; font-size: 13px; } QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(0,210,160,1), stop:1 rgba(0,229,176,1)); }");
        ui.killBtn->setVisible(false);
        ui.cmdInput->setEnabled(false);
        ui.cmdSendBtn->setEnabled(false);
        ui.cmdInput->setPlaceholderText(TR("detail.serverNotRunning"));
        ui.cheatSheetBtn->setEnabled(false);
        // Disable quick bar children buttons
        for (int i = 1; i < ui.quickBarLayout->count(); i++) {
            QWidget* w = ui.quickBarLayout->itemAt(i)->widget();
            if (w) w->setEnabled(false);
        }
        m_uptimeTimer->stop();
        ui.statsUptimeVal->setText("--");
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Quick Actions Bar
// ═══════════════════════════════════════════════════════════════════════

void ServerDetailPanel::buildQuickActions() {
    ui.quickBarLabel->setText(TR("detail.quickActions"));
    ui.quickBarLabel->setStyleSheet("color:#8888aa; font-size:11px; font-weight:500; margin-right:4px;");

    struct QuickAction {
        QString cmd;
        QString labelKey;
    };
    QVector<QuickAction> actions = {
        {"time set day",      "cmd.qTimeDay"},
        {"weather clear",     "cmd.qWeatherClear"},
        {"save-all",          "cmd.qSaveAll"},
        {"list",              "cmd.qList"},
        {"gamemode creative", "cmd.qGameMode"},
        {"say ",              "cmd.qSay"},
        {"difficulty peaceful","cmd.qDifficulty"},
        {"gamerule keepInventory true","cmd.qKeepInventory"},
    };

    for (const auto& qa : actions) {
        auto* btn = new QPushButton(TR(qa.labelKey));
        btn->setMinimumHeight(28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: rgba(108,92,231,0.08); border: 1px solid rgba(108,92,231,0.15);"
            "  border-radius: 14px; color: #a29bfe; font-size: 10px; padding: 2px 10px; }"
            "QPushButton:hover { background: rgba(108,92,231,0.2); border-color: rgba(108,92,231,0.35); }"
            "QPushButton:disabled { background: transparent; border-color: rgba(42,42,74,0.2); color: rgba(136,136,170,0.3); }");
        connect(btn, &QPushButton::clicked, this, [this, cmd = qa.cmd]() { fillCommand(cmd); });
        ui.quickBarLayout->addWidget(btn);
    }
    ui.quickBarLayout->addStretch();
}

// ═══════════════════════════════════════════════════════════════════════
//  Command Auto-Complete
// ═══════════════════════════════════════════════════════════════════════

void ServerDetailPanel::buildCmdCompleter() {
    // ── Command list with descriptions ──
    struct { QString cmd; QString descKey; } cmdData[] = {
        {"advancement",     "cmdDesc.advancement"},
        {"ban",             "cmdDesc.ban"},
        {"ban-ip",          "cmdDesc.banip"},
        {"banlist",         "cmdDesc.banlist"},
        {"defaultgamemode", "cmdDesc.defaultgamemode"},
        {"deop",            "cmdDesc.deop"},
        {"difficulty",      "cmdDesc.difficulty"},
        {"effect",          "cmdDesc.effect"},
        {"enchant",         "cmdDesc.enchant"},
        {"fill",            "cmdDesc.fill"},
        {"gamemode",        "cmdDesc.gamemode"},
        {"gamerule",        "cmdDesc.gamerule"},
        {"give",            "cmdDesc.give"},
        {"help",            "cmdDesc.help"},
        {"kick",            "cmdDesc.kick"},
        {"kill",            "cmdDesc.kill"},
        {"list",            "cmdDesc.list"},
        {"me",              "cmdDesc.msg"},
        {"msg",             "cmdDesc.msg"},
        {"op",              "cmdDesc.op"},
        {"pardon",          "cmdDesc.pardon"},
        {"pardon-ip",       "cmdDesc.pardonip"},
        {"reload",          "cmdDesc.reload"},
        {"save-all",        "cmdDesc.saveall"},
        {"save-off",        "cmdDesc.saveoff"},
        {"save-on",         "cmdDesc.saveon"},
        {"say",             "cmdDesc.say"},
        {"scoreboard",      "cmdDesc.scoreboard"},
        {"seed",            "cmdDesc.seed"},
        {"setblock",        "cmdDesc.setblock"},
        {"spawnpoint",      "cmdDesc.spawnpoint"},
        {"stop",            "cmdDesc.stop"},
        {"summon",          "cmdDesc.summon"},
        {"teleport",        "cmdDesc.tp"},
        {"tell",            "cmdDesc.msg"},
        {"time",            "cmdDesc.time"},
        {"tp",              "cmdDesc.tp"},
        {"weather",         "cmdDesc.weather"},
        {"whitelist",       "cmdDesc.whitelist"},
        {"worldborder",     "cmdDesc.worldborder"},
        {"xp",              "cmdDesc.xp"},
    };

    auto* model = new QStandardItemModel(this);
    for (const auto& d : cmdData) {
        QString desc = TR(d.descKey);
        // Plain command (no slash) — used for matching
        auto* item = new QStandardItem();
        item->setText(d.cmd);                       // matching text
        item->setData(d.cmd, Qt::UserRole);          // command to fill
        item->setData(desc, Qt::UserRole + 1);       // description
        model->appendRow(item);

        // Slash variant
        auto* itemSlash = new QStandardItem();
        itemSlash->setText("/" + d.cmd);
        itemSlash->setData("/" + d.cmd, Qt::UserRole);
        itemSlash->setData(desc, Qt::UserRole + 1);
        model->appendRow(itemSlash);
    }
    model->sort(0);

    // ── Custom delegate for two-line display ──
    class CmdDelegate : public QStyledItemDelegate {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
            painter->save();
            // Selection background
            if (option.state & QStyle::State_Selected) {
                painter->fillRect(option.rect, QColor(108, 92, 231, 60));
            } else if (option.state & QStyle::State_MouseOver) {
                painter->fillRect(option.rect, QColor(108, 92, 231, 25));
            }

            QString cmd = index.data(Qt::UserRole).toString();
            QString desc = index.data(Qt::UserRole + 1).toString();

            QRect r = option.rect.adjusted(10, 3, -6, -3);

            // Command line — purple monospace
            QFont cmdFont("Consolas", 11, QFont::Normal);
            QFontMetrics cmdFM(cmdFont);
            painter->setFont(cmdFont);
            painter->setPen(QColor("#a29bfe"));
            QRect cmdRect = r;
            cmdRect.setHeight(cmdFM.height());
            painter->drawText(cmdRect, Qt::AlignLeft | Qt::AlignVCenter, cmd);

            // Description line — gray
            QFont descFont("Microsoft YaHei", 9);
            painter->setFont(descFont);
            painter->setPen(QColor("#666688"));
            QRect descRect = r;
            descRect.setTop(r.top() + cmdFM.height() + 2);
            descRect.setHeight(r.height() - cmdFM.height() - 2);
            QString elidedDesc = QFontMetrics(descFont).elidedText(desc, Qt::ElideRight, descRect.width());
            painter->drawText(descRect, Qt::AlignLeft | Qt::AlignTop, elidedDesc);

            painter->restore();
        }
        QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
            return QSize(280, 40);
        }
    };

    m_cmdCompleter = new QCompleter(model, this);
    m_cmdCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_cmdCompleter->setFilterMode(Qt::MatchContains);
    m_cmdCompleter->setCompletionMode(QCompleter::PopupCompletion);
    m_cmdCompleter->setMaxVisibleItems(10);
    m_cmdCompleter->setCompletionRole(Qt::UserRole); // fill with command text from UserRole
    m_cmdCompleter->popup()->setItemDelegate(new CmdDelegate(m_cmdCompleter->popup()));
    m_cmdCompleter->popup()->setMinimumWidth(300);

    // Style the completer popup
    m_cmdCompleter->popup()->setStyleSheet(
        "QAbstractItemView {"
        "  background: #12121e; border: 1px solid #3a3a5a; border-radius: 6px;"
        "  outline: none; padding: 4px;"
        "}"
        "QAbstractItemView::item { padding: 0px; border-radius: 4px; }");

    ui.cmdInput->setCompleter(m_cmdCompleter);
}

void ServerDetailPanel::fillCommand(const QString& cmd) {
    ui.cmdInput->setText(cmd);
    ui.cmdInput->setFocus();
    // Place cursor at end
    ui.cmdInput->end(false);
}

bool ServerDetailPanel::eventFilter(QObject* obj, QEvent* event) {
    // Handle click on cheat sheet rows
    if (event->type() == QEvent::MouseButtonPress) {
        QVariant cmdVar = obj->property("cmdFill");
        if (cmdVar.isValid()) {
            fillCommand(cmdVar.toString());
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ═══════════════════════════════════════════════════════════════════════
//  Cheat Sheet Dialog
// ═══════════════════════════════════════════════════════════════════════

void ServerDetailPanel::showCheatSheet() {
    // ── Full command reference with categories ──
    struct CmdRef {
        QString cmd;
        QString descKey;
        QString cat;  // category TR key
    };

    QVector<CmdRef> allRefs = {
        // Player
        {"help",               "cmdDesc.help",       "cmd.categoryPlayer"},
        {"list",               "cmdDesc.list",       "cmd.categoryPlayer"},
        {"op <player>",        "cmdDesc.op",         "cmd.categoryPlayer"},
        {"deop <player>",      "cmdDesc.deop",       "cmd.categoryPlayer"},
        {"kick <player>",      "cmdDesc.kick",       "cmd.categoryPlayer"},
        {"ban <player>",       "cmdDesc.ban",        "cmd.categoryPlayer"},
        {"pardon <player>",    "cmdDesc.pardon",     "cmd.categoryPlayer"},
        {"ban-ip <address>",   "cmdDesc.banip",      "cmd.categoryPlayer"},
        {"pardon-ip <address>","cmdDesc.pardonip",   "cmd.categoryPlayer"},
        {"whitelist add <p>",  "cmdDesc.whitelist",  "cmd.categoryPlayer"},
        {"gamemode <mode> <p>","cmdDesc.gamemode",   "cmd.categoryPlayer"},
        {"tp <from> <to>",     "cmdDesc.tp",         "cmd.categoryPlayer"},
        {"give <player> <item>","cmdDesc.give",      "cmd.categoryPlayer"},
        {"kill <player>",      "cmdDesc.kill",       "cmd.categoryPlayer"},
        {"msg <player> <msg>", "cmdDesc.msg",        "cmd.categoryPlayer"},
        {"xp add <player> <n>","cmdDesc.xp",         "cmd.categoryPlayer"},
        {"effect give @p <id>","cmdDesc.effect",     "cmd.categoryPlayer"},
        {"enchant @p <id> <lvl>","cmdDesc.enchant",  "cmd.categoryPlayer"},
        // World
        {"time set day",       "cmdDesc.time",       "cmd.categoryWorld"},
        {"time set night",     "cmdDesc.time",       "cmd.categoryWorld"},
        {"time set noon",      "cmdDesc.time",       "cmd.categoryWorld"},
        {"weather clear",      "cmdDesc.weather",    "cmd.categoryWorld"},
        {"weather rain",       "cmdDesc.weather",    "cmd.categoryWorld"},
        {"weather thunder",    "cmdDesc.weather",    "cmd.categoryWorld"},
        {"gamerule <rule> <v>","cmdDesc.gamerule",   "cmd.categoryWorld"},
        {"seed",               "cmdDesc.seed",       "cmd.categoryWorld"},
        {"difficulty <level>", "cmdDesc.difficulty", "cmd.categoryWorld"},
        {"defaultgamemode <m>","cmdDesc.defaultgamemode","cmd.categoryWorld"},
        {"spawnpoint <player>","cmdDesc.spawnpoint", "cmd.categoryWorld"},
        {"worldborder <...>",  "cmdDesc.worldborder","cmd.categoryWorld"},
        {"fill <from> <to> <block>","cmdDesc.fill",  "cmd.categoryWorld"},
        {"setblock <x> <y> <z> <b>","cmdDesc.setblock","cmd.categoryWorld"},
        {"summon <entity>",    "cmdDesc.summon",     "cmd.categoryWorld"},
        // Server
        {"save-all",           "cmdDesc.saveall",    "cmd.categoryServer"},
        {"save-off",           "cmdDesc.saveoff",    "cmd.categoryServer"},
        {"save-on",            "cmdDesc.saveon",     "cmd.categoryServer"},
        {"stop",               "cmdDesc.stop",       "cmd.categoryServer"},
        {"reload",             "cmdDesc.reload",     "cmd.categoryServer"},
        {"say <message>",      "cmdDesc.say",        "cmd.categoryServer"},
        {"scoreboard <...>",   "cmdDesc.scoreboard", "cmd.categoryServer"},
        {"banlist",            "cmdDesc.banlist",    "cmd.categoryServer"},
        {"advancement <...>",  "cmdDesc.advancement","cmd.categoryServer"},
    };

    // ── Build dialog ──
    QDialog dlg;
    dlg.setWindowTitle(TR("cmd.cheatSheetTitle"));
    dlg.setMinimumSize(580, 500);
    dlg.resize(620, 560);
    dlg.setStyleSheet(
        "QDialog { background: #12121e; border: 1px solid rgba(108,92,231,0.3); border-radius: 16px; }");

    auto* outerLayout = new QVBoxLayout(&dlg);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Title bar
    auto* titleBar = new QWidget;
    titleBar->setStyleSheet("background: transparent; padding: 18px 24px 10px 24px;");
    auto* titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(0, 0, 0, 0);
    auto* dlgTitle = new QLabel(TR("cmd.cheatSheetTitle"));
    dlgTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #a29bfe;");
    titleRow->addWidget(dlgTitle);
    titleRow->addStretch();
    auto* tipLabel = new QLabel(TR("cmd.clickToFill"));
    tipLabel->setStyleSheet("color: #555577; font-size: 11px;");
    titleRow->addWidget(tipLabel);
    outerLayout->addWidget(titleBar);

    // Search bar
    auto* searchWidget = new QWidget;
    searchWidget->setStyleSheet("background: transparent; padding: 0 24px 8px 24px;");
    auto* searchLayout = new QHBoxLayout(searchWidget);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    auto* searchInput = new QLineEdit;
    searchInput->setPlaceholderText(TR("cmd.searchPlaceholder"));
    searchInput->setStyleSheet(
        "QLineEdit { background: #0a0a16; border: 1px solid #2a2a4a; border-radius: 8px;"
        "  color: #e0e0f0; padding: 8px 14px; font-size: 13px; }"
        "QLineEdit:focus { border-color: #6c5ce7; }");
    searchLayout->addWidget(searchInput);
    outerLayout->addWidget(searchWidget);

    // Category tabs
    auto* catWidget = new QWidget;
    catWidget->setStyleSheet("background: transparent; padding: 0 24px 8px 24px;");
    auto* catLayout = new QHBoxLayout(catWidget);
    catLayout->setContentsMargins(0, 0, 0, 0);
    catLayout->setSpacing(8);
    catLayout->addStretch();

    QStringList catKeys = {"cmd.categoryAll", "cmd.categoryPlayer", "cmd.categoryWorld", "cmd.categoryServer", "cmd.categoryAdmin"};
    QVector<QPushButton*> catBtns;
    for (const QString& ck : catKeys) {
        auto* cb = new QPushButton(TR(ck));
        cb->setMinimumHeight(30);
        cb->setCheckable(true);
        cb->setCursor(Qt::PointingHandCursor);
        cb->setStyleSheet(
            "QPushButton { background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3);"
            "  border-radius: 15px; color: #8888aa; font-size: 12px; padding: 4px 16px; }"
            "QPushButton:hover { background: rgba(108,92,231,0.1); color: #a29bfe; }"
            "QPushButton:checked { background: rgba(108,92,231,0.2); border-color: rgba(108,92,231,0.4);"
            "  color: #a29bfe; font-weight: bold; }");
        catLayout->addWidget(cb);
        catBtns.append(cb);
    }
    catLayout->addStretch();
    catBtns[0]->setChecked(true);
    outerLayout->addWidget(catWidget);

    // Scroll area with command list
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: #12121e; width: 8px; }"
        "QScrollBar::handle:vertical { background: #3a3a5a; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    auto* listWidget = new QWidget;
    auto* listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(24, 4, 24, 16);
    listLayout->setSpacing(2);

    // Build rows for each command
    struct CmdRow { QWidget* row; QString cmd; QString desc; QString cat; };
    QVector<CmdRow> rows;

    for (const auto& ref : allRefs) {
        QString catTr = TR(ref.cat);
        QString descTr = TR(ref.descKey);

        // Clickable row widget
        auto* row = new QWidget;
        row->setCursor(Qt::PointingHandCursor);
        row->setStyleSheet("background: transparent; border-radius: 6px; padding: 4px 10px;");
        row->setProperty("hoverBg", "QWidget:hover { background: rgba(108,92,231,0.1); }");

        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 4, 12, 4);
        rowLayout->setSpacing(0);

        auto* cmdLabel = new QLabel(ref.cmd);
        cmdLabel->setStyleSheet("color:#a29bfe; font-family:'Consolas','JetBrains Mono',monospace; font-size:13px; min-width:180px;");
        cmdLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        rowLayout->addWidget(cmdLabel);

        auto* descLabel = new QLabel(QString::fromUtf8(" — ") + descTr);
        descLabel->setStyleSheet("color:#8888aa; font-size:12px;");
        rowLayout->addWidget(descLabel, 1);

        listLayout->addWidget(row);
        rows.append({row, ref.cmd, descTr, catTr});

        // Click handler via event filter
        row->installEventFilter(this);
        row->setProperty("cmdFill", ref.cmd);
        row->setProperty("closeDlg", true);
    }
    listLayout->addStretch();
    scrollArea->setWidget(listWidget);
    outerLayout->addWidget(scrollArea, 1);

    // ── Search filter logic ──
    QObject::connect(searchInput, &QLineEdit::textChanged, [rows, catBtns](const QString& text) mutable {
        QString filter = text.trimmed().toLower();
        for (auto& r : rows) {
            if (filter.isEmpty()) {
                r.row->setVisible(true);
            } else {
                bool match = r.cmd.toLower().contains(filter) || r.desc.toLower().contains(filter);
                r.row->setVisible(match);
            }
        }
    });

    // ── Category filter ──
    QString catToTR[5] = {
        "", // All - no filter
        TR("cmd.categoryPlayer"),
        TR("cmd.categoryWorld"),
        TR("cmd.categoryServer"),
        TR("cmd.categoryAdmin"),
    };

    for (int i = 0; i < catBtns.size(); i++) {
        QObject::connect(catBtns[i], &QPushButton::toggled, [i, catBtns, rows, catToTR, searchInput](bool checked) {
            if (!checked) return;
            // Uncheck others
            for (int j = 0; j < catBtns.size(); j++)
                if (j != i) catBtns[j]->setChecked(false);

            QString selCat = catToTR[i];
            for (auto& r : rows) {
                if (selCat.isEmpty() || r.cat == selCat) {
                    // Also respect search filter
                    QString filter = searchInput->text().trimmed().toLower();
                    if (filter.isEmpty()) r.row->setVisible(true);
                    else r.row->setVisible(r.cmd.toLower().contains(filter) || r.desc.toLower().contains(filter));
                } else {
                    r.row->setVisible(false);
                }
            }
        });
    }

    dlg.exec();
}

void ServerDetailPanel::onStartStop() {
    if (m_process->state() == ServerProcess::Running) m_process->stop();
    else { ui.consoleOutput->clear(); m_process->start(m_config); }
    updateUI();
}

void ServerDetailPanel::onKill() {
    auto reply = QMessageBox::question(this, TR("detail.confirm"), TR("detail.forceStopMsg"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) m_process->kill();
}

void ServerDetailPanel::onDelete() {
    QDialog dlg;
    dlg.setWindowTitle(TR("detail.confirmDelete"));
    dlg.setFixedSize(380, 220);
    dlg.setStyleSheet("QDialog { background: #12121e; border: 1px solid rgba(255,71,87,0.3); border-radius: 16px; }");
    QVBoxLayout* dlgLayout = new QVBoxLayout(&dlg);
    dlgLayout->setSpacing(16);

    QLabel* dlgTitle = new QLabel(TR("detail.confirmDelete"));
    dlgTitle->setAlignment(Qt::AlignCenter);
    dlgTitle->setStyleSheet("font-size:16px; font-weight:bold; color:#e0e0f0;");
    dlgLayout->addWidget(dlgTitle);

    QLabel* dlgMsg = new QLabel(TR("detail.deleteMsg").arg(m_config.name));
    dlgMsg->setAlignment(Qt::AlignCenter); dlgMsg->setWordWrap(true);
    dlgMsg->setStyleSheet("color:#8888aa; font-size:13px;");
    dlgLayout->addWidget(dlgMsg);

    QHBoxLayout* btnRow = new QHBoxLayout; btnRow->setSpacing(12);
    QPushButton* cancelBtn = new QPushButton(TR("detail.cancel"));
    cancelBtn->setMinimumHeight(38);
    cancelBtn->setStyleSheet("QPushButton { background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3); border-radius: 12px; color: #8888aa; padding: 8px 20px; font-size: 13px; } QPushButton:hover { background: rgba(255,255,255,0.05); color: #e0e0f0; }");
    QPushButton* confirmBtn = new QPushButton(TR("detail.delete"));
    confirmBtn->setMinimumHeight(38);
    confirmBtn->setStyleSheet("QPushButton { background: rgba(255,71,87,0.15); border: 1px solid rgba(255,71,87,0.2); border-radius: 12px; color: #ff4757; padding: 8px 20px; font-size: 13px; font-weight: 500; } QPushButton:hover { background: rgba(255,71,87,0.25); }");
    btnRow->addWidget(cancelBtn, 1); btnRow->addWidget(confirmBtn, 1);
    dlgLayout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(confirmBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted) {
        if (m_process->state() == ServerProcess::Running) m_process->kill();
        m_store->deleteServer(m_serverId);
        emit backRequested();
    }
}

void ServerDetailPanel::onEditConfig() {
    QDialog dlg;
    dlg.setWindowTitle(TR("detail.editConfig"));
    dlg.setFixedSize(460, 520);
    dlg.setStyleSheet("QDialog { background: #12121e; border: 1px solid rgba(108,92,231,0.3); border-radius: 16px; }");

    QVBoxLayout* outerLayout = new QVBoxLayout(&dlg);
    outerLayout->setContentsMargins(0, 0, 0, 0); outerLayout->setSpacing(0);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* contentWidget = new QWidget;
    QVBoxLayout* dlgLayout = new QVBoxLayout(contentWidget);
    dlgLayout->setContentsMargins(24, 20, 24, 16); dlgLayout->setSpacing(10);

    QLabel* dlgTitle = new QLabel(TR("detail.editConfigTitle"));
    dlgTitle->setStyleSheet("font-size:17px; font-weight:bold; color:#a29bfe;");
    dlgLayout->addWidget(dlgTitle);

    auto addField = [&](const QString& label) -> QLineEdit* {
        QLabel* lbl = new QLabel(label); lbl->setStyleSheet("color:#8888aa; font-size:12px; font-weight:500;");
        dlgLayout->addWidget(lbl);
        QLineEdit* edit = new QLineEdit; edit->setMinimumHeight(36); edit->setStyleSheet("font-size:13px;");
        dlgLayout->addWidget(edit);
        return edit;
    };

    QLineEdit* nameEdit = addField(TR("detail.serverName"));
    nameEdit->setText(m_config.name);

    QLabel* pathLbl = new QLabel(TR("detail.serverPath"));
    pathLbl->setStyleSheet("color:#8888aa; font-size:12px; font-weight:500;");
    dlgLayout->addWidget(pathLbl);
    QHBoxLayout* pathRow = new QHBoxLayout; pathRow->setSpacing(8);
    QLineEdit* pathEdit = new QLineEdit;
    pathEdit->setMinimumHeight(36); pathEdit->setReadOnly(true); pathEdit->setStyleSheet("font-size:13px;");
    pathEdit->setText(m_config.path);
    QPushButton* browsePath = new QPushButton(TR("detail.browse"));
    browsePath->setFixedWidth(80); browsePath->setMinimumHeight(36); browsePath->setStyleSheet("font-size:12px;");
    connect(browsePath, &QPushButton::clicked, [&]() { QString dir = QFileDialog::getExistingDirectory(&dlg, TR("detail.selectDir")); if (!dir.isEmpty()) pathEdit->setText(dir); });
    pathRow->addWidget(pathEdit, 1); pathRow->addWidget(browsePath);
    dlgLayout->addLayout(pathRow);

    QHBoxLayout* memRow = new QHBoxLayout; memRow->setSpacing(12);
    QLabel* memLabel = new QLabel(TR("detail.memory")); memLabel->setStyleSheet("color:#8888aa; font-size:12px;");
    memRow->addWidget(memLabel);
    QComboBox* minRamCombo = new QComboBox;
    for (int mb : {512, 1024, 2048, 4096, 6144, 8192, 12288, 16384}) {
        minRamCombo->addItem(TR("detail.minRam").arg(mb), mb);
        if (mb == m_config.minRamMB) minRamCombo->setCurrentIndex(minRamCombo->count() - 1);
    }
    memRow->addWidget(minRamCombo, 1);
    QComboBox* maxRamCombo = new QComboBox;
    for (int mb : {1024, 2048, 4096, 6144, 8192, 12288, 16384}) {
        maxRamCombo->addItem(TR("detail.maxRam").arg(mb), mb);
        if (mb == m_config.maxRamMB) maxRamCombo->setCurrentIndex(maxRamCombo->count() - 1);
    }
    memRow->addWidget(maxRamCombo, 1);
    dlgLayout->addLayout(memRow);

    QLabel* javaLbl = new QLabel(TR("detail.javaPath"));
    javaLbl->setStyleSheet("color:#8888aa; font-size:12px; font-weight:500;");
    dlgLayout->addWidget(javaLbl);
    QHBoxLayout* javaRow = new QHBoxLayout; javaRow->setSpacing(8);
    QLineEdit* javaEdit = new QLineEdit;
    javaEdit->setMinimumHeight(36); javaEdit->setStyleSheet("font-size:13px;");
    javaEdit->setText(m_config.javaPath);
    QPushButton* browseJava = new QPushButton(TR("detail.browse"));
    browseJava->setFixedWidth(80); browseJava->setMinimumHeight(36); browseJava->setStyleSheet("font-size:12px;");
    connect(browseJava, &QPushButton::clicked, [&]() { QString file = QFileDialog::getOpenFileName(&dlg, TR("detail.selectJava"), QString(), TR("detail.selectJavaFilter")); if (!file.isEmpty()) javaEdit->setText(file); });
    javaRow->addWidget(javaEdit, 1); javaRow->addWidget(browseJava);
    dlgLayout->addLayout(javaRow);

    QLineEdit* argsEdit = addField(TR("detail.jvmArgs"));
    argsEdit->setText(m_config.javaArgs);
    argsEdit->setPlaceholderText(TR("detail.jvmArgsHint"));

    dlgLayout->addSpacing(8);

    QHBoxLayout* btnRow = new QHBoxLayout; btnRow->setSpacing(12);
    QPushButton* cancelBtn = new QPushButton(TR("detail.cancel"));
    cancelBtn->setMinimumHeight(38);
    cancelBtn->setStyleSheet("QPushButton { background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3); border-radius: 12px; color: #8888aa; font-size: 13px; padding: 8px 20px; } QPushButton:hover { background: rgba(255,255,255,0.05); color: #e0e0f0; }");
    QPushButton* saveBtn = new QPushButton(TR("detail.save"));
    saveBtn->setMinimumHeight(38);
    saveBtn->setStyleSheet("QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #6c5ce7, stop:1 #a29bfe); border: none; color: white; font-weight: bold; border-radius: 12px; font-size: 13px; padding: 8px 24px; } QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #7d6ff0, stop:1 #b3a8ff); }");
    btnRow->addWidget(cancelBtn, 1); btnRow->addWidget(saveBtn, 2);
    dlgLayout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, [&]() {
        m_config.name = nameEdit->text().trimmed(); m_config.path = pathEdit->text().trimmed();
        m_config.minRamMB = minRamCombo->currentData().toInt(); m_config.maxRamMB = maxRamCombo->currentData().toInt();
        m_config.javaPath = javaEdit->text().trimmed(); m_config.javaArgs = argsEdit->text().trimmed();
        if (m_config.javaPath.isEmpty()) m_config.javaPath = "java";
        if (m_config.name.isEmpty()) { QMessageBox::warning(&dlg, TR("detail.error"), TR("detail.nameEmpty")); return; }
        if (m_config.path.isEmpty()) { QMessageBox::warning(&dlg, TR("detail.error"), TR("detail.pathEmpty")); return; }
        m_store->saveServer(m_config);
        ui.nameLabel->setText(m_config.name);
        QString capType = m_config.type; capType[0] = capType[0].toUpper();
        ui.versionPathLabel->setText(QString("Minecraft %1  |  %2  |  %3").arg(m_config.version, capType, m_config.path));
        ui.statsMemVal->setText(QString("%1M - %2M").arg(m_config.minRamMB).arg(m_config.maxRamMB));
        ui.statsJavaVal->setText(m_config.javaPath.isEmpty() ? TR("detail.default") : TR("detail.javaDefault"));
        dlg.accept();
    });

    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea);
    dlg.exec();
}

void ServerDetailPanel::onSendCommand() {
    QString cmd = ui.cmdInput->text().trimmed();
    if (cmd.isEmpty()) return;
    m_process->sendCommand(cmd);
    ui.cmdInput->clear();
}

void ServerDetailPanel::onConsoleOutput(const QString& line) {
    if (m_process->serverId() != m_serverId) return;
    QString color = "rgba(224,224,240,0.7)";
    if (line.contains("ERROR") || line.contains("[ERROR]")) color = "#ff4757";
    else if (line.contains("WARN") || line.contains("[WARN]")) color = "#facc15";
    else if (line.contains("Done")) color = "#00d2a0";
    else if (line.contains("joined") || line.contains("logged in")) color = "#a29bfe";
    else if (line.contains("left") || line.contains("lost connection")) color = "#8888aa";
    ui.consoleOutput->append(QString("<span style='color:%1;'>%2</span>").arg(color, line.toHtmlEscaped()));
    ui.logCountLabel->setText(TR("detail.lines").arg(ui.consoleOutput->document()->blockCount()));
}

void ServerDetailPanel::onStateChanged(ServerProcess::State state) {
    Q_UNUSED(state);
    if (m_process->serverId() == m_serverId) updateUI();
}

void ServerDetailPanel::tickUptime() {
    if (m_process->state() == ServerProcess::Running && m_process->serverId() == m_serverId) {
        qint64 secs = m_process->uptimeSecs();
        qint64 h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
        QString text;
        if (h > 0) text = QString("%1h %2m %3s").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        else if (m > 0) text = QString("%1m %2s").arg(m).arg(s, 2, 10, QChar('0'));
        else text = QString("%1s").arg(s);
        ui.statsUptimeVal->setText(text);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Server Properties Editor (Native Qt Dialog)
// ═══════════════════════════════════════════════════════════════════════

void ServerDetailPanel::onEditProperties() {
    QString propsPath = m_config.path + "/server.properties";
    QMap<QString, QString> currentProps = readPropertiesFile(propsPath);

    // Build dialog
    QDialog dlg;
    dlg.setWindowTitle(TR("detail.serverProps"));
    dlg.setMinimumSize(600, 620);
    dlg.resize(620, 680);
    dlg.setStyleSheet("QDialog { background: #12121e; border: 1px solid rgba(34,211,238,0.3); border-radius: 16px; }");

    auto* outerLayout = new QVBoxLayout(&dlg);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Title bar
    auto* titleBar = new QWidget;
    titleBar->setStyleSheet("background: transparent; padding: 18px 24px 10px 24px;");
    auto* titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(0, 0, 0, 0);
    auto* dlgTitle = new QLabel(TR("detail.serverProps"));
    dlgTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #22d3ee;");
    titleRow->addWidget(dlgTitle);
    titleRow->addStretch();

    // Toggle advanced checkbox
    QCheckBox* showAdvancedCheck = new QCheckBox(TR("detail.showAdvanced"));
    showAdvancedCheck->setStyleSheet("QCheckBox { color: #8888aa; font-size: 12px; spacing: 6px; }");
    titleRow->addWidget(showAdvancedCheck);
    outerLayout->addWidget(titleBar);

    // Scroll area
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: #12121e; width: 8px; }"
        "QScrollBar::handle:vertical { background: #3a3a5a; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    auto* contentWidget = new QWidget;
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(24, 4, 24, 16);
    contentLayout->setSpacing(14);

    // ── Build section groups ──
    QVector<QWidget*> commonSections;
    QVector<QWidget*> advancedSections;

    // Keep references to all input widgets for collecting values later
    struct PropWidget {
        QString key;
        QWidget* widget = nullptr;  // QLineEdit, QComboBox, or QCheckBox
        QString type;
    };
    QVector<PropWidget> allPropWidgets;

    for (int ci = 0; ci < kPropCategoryCount; ci++) {
        const PropCategory& cat = kPropCategories[ci];
        QString catId = QString::fromLatin1(cat.id);

        // Find all properties in this category
        QVector<const PropDef*> catProps;
        for (int i = 0; i < kPropDefCount; i++) {
            if (catId == QString::fromLatin1(kPropDefs[i].category))
                catProps.append(&kPropDefs[i]);
        }
        if (catProps.isEmpty()) continue;

        // Build group box
        auto* groupBox = new QGroupBox(TR(cat.label));
        QString borderColor = cat.common ? "#6c5ce7" : "#f59e0b";
        groupBox->setStyleSheet(QString(
            "QGroupBox { border: 1px solid %1; border-radius: 10px; margin-top: 12px; "
            "  padding: 16px 14px 10px 14px; font-weight: bold; color: #a29bfe; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 6px; }"
        ).arg(borderColor));

        auto* gLayout = new QFormLayout(groupBox);
        gLayout->setSpacing(6);
        gLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        gLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

        for (const PropDef* pd : catProps) {
            QString key = QString::fromLatin1(pd->key);
            QString val = currentProps.contains(key) ? currentProps[key]
                        : QString::fromLatin1(pd->defaultVal);
            QString type = QString::fromLatin1(pd->type);

            PropWidget pw;
            pw.key = key;
            pw.type = type;

            QLabel* fieldLabel = new QLabel(TR(pd->label));
            fieldLabel->setStyleSheet("color: #8888aa; font-size: 11px; min-width: 140px;");

            if (type == "bool") {
                auto* cb = new QCheckBox;
                cb->setChecked(val.toLower() == "true");
                cb->setStyleSheet("QCheckBox { color: #e0e0f0; font-size: 12px; spacing: 8px; }");
                pw.widget = cb;
                gLayout->addRow(fieldLabel, cb);
            } else if (type == "enum") {
                auto* combo = new QComboBox;
                combo->setStyleSheet(
                    "QComboBox { background: #0a0a16; border: 1px solid #2a2a4a; border-radius: 4px; "
                    "  color: #e0e0f0; padding: 4px 8px; font-size: 12px; }"
                    "QComboBox::drop-down { border: none; width: 20px; }"
                    "QComboBox QAbstractItemView { background: #12121e; border: 1px solid #2a2a4a; "
                    "  color: #e0e0f0; selection-background-color: rgba(108,92,231,0.3); }"
                );
                QStringList opts = QString::fromLatin1(pd->options).split(',', Qt::SkipEmptyParts);
                int selIdx = 0;
                for (int oi = 0; oi < opts.size(); oi++) {
                    combo->addItem(opts[oi].trimmed());
                    if (opts[oi].trimmed() == val) selIdx = oi;
                }
                combo->setCurrentIndex(selIdx);
                pw.widget = combo;
                gLayout->addRow(fieldLabel, combo);
            } else {
                auto* edit = new QLineEdit(val);
                edit->setStyleSheet(
                    "QLineEdit { background: #0a0a16; border: 1px solid #2a2a4a; border-radius: 4px; "
                    "  color: #e0e0f0; padding: 4px 8px; font-size: 12px; }"
                    "QLineEdit:focus { border-color: #6c5ce7; }"
                );
                if (type == "number") {
                    QString opts = QString::fromLatin1(pd->options);
                    if (!opts.isEmpty()) {
                        QStringList range = opts.split(',');
                        if (range.size() >= 2)
                            edit->setPlaceholderText(QString("[%1, %2]").arg(range[0], range[1]));
                    }
                }
                pw.widget = edit;
                gLayout->addRow(fieldLabel, edit);
            }

            allPropWidgets.append(pw);
        }

        if (cat.common)
            commonSections.append(groupBox);
        else
            advancedSections.append(groupBox);
    }

    // Add common sections (always visible)
    for (auto* w : commonSections)
        contentLayout->addWidget(w);

    // Separator and advanced sections header
    auto* advHeader = new QWidget;
    auto* advHeaderLayout = new QHBoxLayout(advHeader);
    advHeaderLayout->setContentsMargins(0, 8, 0, 4);
    auto* advLabel = new QLabel(TR("detail.advanced"));
    advLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #f59e0b;");
    advHeaderLayout->addWidget(advLabel);
    advHeaderLayout->addStretch();
    contentLayout->addWidget(advHeader);

    // Advanced sections container (togglable)
    auto* advContainer = new QWidget;
    auto* advContainerLayout = new QVBoxLayout(advContainer);
    advContainerLayout->setContentsMargins(0, 0, 0, 0);
    advContainerLayout->setSpacing(14);
    for (auto* w : advancedSections)
        advContainerLayout->addWidget(w);
    contentLayout->addWidget(advContainer);

    // Connect toggle
    QObject::connect(showAdvancedCheck, &QCheckBox::toggled, [advContainer, showAdvancedCheck]() {
        advContainer->setVisible(showAdvancedCheck->isChecked());
        showAdvancedCheck->setText(showAdvancedCheck->isChecked() ? TR("detail.hideAdvanced") : TR("detail.showAdvanced"));
    });
    advContainer->setVisible(false); // hidden by default

    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea, 1);

    // Bottom buttons
    auto* btnRow = new QHBoxLayout;
    btnRow->setContentsMargins(24, 12, 24, 16);
    btnRow->addStretch();

    auto* cancelBtn = new QPushButton(TR("detail.cancel"));
    cancelBtn->setMinimumHeight(38);
    cancelBtn->setMinimumWidth(90);
    cancelBtn->setStyleSheet(
        "QPushButton { background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3); "
        "  border-radius: 10px; color: #8888aa; font-size: 13px; padding: 8px 20px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.05); color: #e0e0f0; }"
    );

    auto* saveBtn = new QPushButton(TR("detail.saveProps"));
    saveBtn->setMinimumHeight(38);
    saveBtn->setMinimumWidth(130);
    saveBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #06b6d4, stop:1 #22d3ee); "
        "  border: none; color: white; font-weight: bold; border-radius: 10px; font-size: 13px; padding: 8px 24px; }"
        "QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0891b2, stop:1 #06b6d4); }"
    );

    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    outerLayout->addLayout(btnRow);

    // ── Connections ──
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    QObject::connect(saveBtn, &QPushButton::clicked, [&]() {
        QMap<QString, QString> newProps = currentProps;

        for (const auto& pw : allPropWidgets) {
            QString val;
            if (pw.type == "bool") {
                auto* cb = qobject_cast<QCheckBox*>(pw.widget);
                val = cb && cb->isChecked() ? "true" : "false";
            } else if (pw.type == "enum") {
                auto* combo = qobject_cast<QComboBox*>(pw.widget);
                val = combo ? combo->currentText() : "";
            } else {
                auto* edit = qobject_cast<QLineEdit*>(pw.widget);
                val = edit ? edit->text().trimmed() : "";
            }
            newProps[pw.key] = val;
        }

        if (!writePropertiesFile(propsPath, newProps)) {
            QMessageBox::warning(&dlg, TR("detail.error"), TR("detail.saveError") + propsPath);
        } else {
            dlg.accept();
        }
    });

    dlg.exec();
}
