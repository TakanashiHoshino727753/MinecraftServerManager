#include "PlayerWorldPanel.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollArea>
#include <QDateTime>
#include <QTimer>

PlayerWorldPanel::PlayerWorldPanel(ConfigStore* store, ServerProcess* process, QWidget* parent)
    : QWidget(parent), m_store(store), m_process(process)
{
    m_playerManager = new PlayerManager(this);
    m_worldManager  = new WorldManager(this);
    connect(m_process, &ServerProcess::consoleOutput, this, &PlayerWorldPanel::onConsoleLine);
    connect(m_playerManager, &PlayerManager::onlinePlayersChanged, this, [this]() { refreshPlayerTable(); });
    connect(m_worldManager, &WorldManager::worldsChanged, this, [this]() { refreshWorldTable(); });
    connect(m_worldManager, &WorldManager::error, this, [this](const QString& msg) { QMessageBox::warning(this, TR("panel.world.error"), msg); });

    ui.setupUi(this);

    // ── Pointer aliases ──
    m_tabWidget     = ui.tabWidget;
    m_playerTable   = ui.playerTable;
    m_worldTable    = ui.worldTable;
    m_playerCountLabel = ui.playerCountLabel;
    m_kickBtn       = ui.kickBtn;
    m_banBtn        = ui.banBtn;
    m_opBtn         = ui.opBtn;
    m_deopBtn       = ui.deopBtn;
    m_whitelistBtn  = ui.whitelistBtn;
    m_gamemodeCombo = ui.gamemodeCombo;
    m_gamemodeBtn   = ui.gamemodeBtn;
    m_playerReasonEdit = ui.playerReasonEdit;
    m_backupBtn     = ui.backupBtn;
    m_restoreBtn    = ui.restoreBtn;
    m_deleteWorldBtn= ui.deleteWorldBtn;

    // ── Tab widget tabs ──
    m_tabWidget->setTabText(0, TR("panel.player.title"));
    m_tabWidget->setTabText(1, TR("panel.world.title"));

    // ── Setup player table ──
    m_playerTable->setHorizontalHeaderLabels({TR("panel.player.colName"), TR("panel.player.colUUID"), TR("panel.player.colWorld"), TR("panel.player.colHealth"), TR("panel.player.colStatus")});
    m_playerTable->horizontalHeader()->setStretchLastSection(true);
    m_playerTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_playerTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_playerTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_playerTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    // ── Setup world table ──
    m_worldTable->setHorizontalHeaderLabels({TR("panel.world.colName"), TR("panel.world.colSize"), TR("panel.world.colSeed"), TR("panel.world.colGameType"), TR("panel.world.colLastPlayed")});
    m_worldTable->horizontalHeader()->setStretchLastSection(true);
    m_worldTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_worldTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_worldTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_worldTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    // ── Style buttons ──
    QString ghostStyle = "QPushButton { background: rgba(108,92,231,0.1); border: 1px solid #2a2a4a; border-radius: 6px; color: #e0e0f0; padding: 6px 14px; font-size: 12px; } QPushButton:hover { background: rgba(108,92,231,0.25); }";
    m_kickBtn->setStyleSheet(ghostStyle); m_banBtn->setStyleSheet(ghostStyle);
    m_opBtn->setStyleSheet(ghostStyle); m_deopBtn->setStyleSheet(ghostStyle); m_whitelistBtn->setStyleSheet(ghostStyle);
    m_gamemodeBtn->setStyleSheet(ghostStyle);
    m_backupBtn->setStyleSheet(ghostStyle); m_restoreBtn->setStyleSheet(ghostStyle);
    m_deleteWorldBtn->setStyleSheet("QPushButton { background: rgba(255,71,87,0.1); border: 1px solid rgba(255,71,87,0.3); border-radius: 6px; color: #ff4757; padding: 8px 16px; font-size: 12px; } QPushButton:hover { background: rgba(255,71,87,0.3); }");

    // ── Connects ──
    connect(m_kickBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onKickPlayer);
    connect(m_banBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onBanPlayer);
    connect(m_opBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onOpPlayer);
    connect(m_deopBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onDeopPlayer);
    connect(m_whitelistBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onWhitelistAdd);
    connect(m_gamemodeBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onGamemodeApply);
    connect(ui.playerRefreshBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onRefreshPlayers);
    connect(m_backupBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onBackupWorld);
    connect(m_restoreBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onRestoreWorld);
    connect(m_deleteWorldBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onDeleteWorld);
    connect(ui.worldRefreshBtn, &QPushButton::clicked, this, &PlayerWorldPanel::onRefreshWorlds);

    // ── i18n texts ──
    ui.titleLabel->setText(TR("panel.playerworld.title"));
    m_playerCountLabel->setText(TR("panel.player.onlineCount").arg("0"));
    ui.playerActionGroup->setTitle(TR("panel.player.actions"));
    m_kickBtn->setText(TR("panel.player.kick"));
    m_banBtn->setText(TR("panel.player.ban"));
    m_opBtn->setText(TR("panel.player.op"));
    m_deopBtn->setText(TR("panel.player.deop"));
    m_whitelistBtn->setText(TR("panel.player.whitelist"));
    m_gamemodeBtn->setText(TR("panel.player.gamemode"));
    m_playerReasonEdit->setPlaceholderText(TR("panel.player.reason"));
    ui.playerRefreshBtn->setText(TR("panel.refresh"));
    ui.worldActionGroup->setTitle(TR("panel.world.actions"));
    m_backupBtn->setText(TR("panel.world.backup"));
    m_restoreBtn->setText(TR("panel.world.restore"));
    m_deleteWorldBtn->setText(TR("panel.world.delete"));
    ui.worldRefreshBtn->setText(TR("panel.refresh"));
}

void PlayerWorldPanel::loadServer(const QString& serverId) {
    m_serverId = serverId;
    m_hasServer = !serverId.isEmpty();
    if (m_hasServer) {
        ServerConfig* cfg = m_store->findServer(serverId);
        if (cfg) {
            m_serverPath = cfg->path;
            m_worldManager->scan(cfg->path);
        }
    }
    refresh();
}

void PlayerWorldPanel::refresh() {
    refreshPlayerTable();
    if (m_hasServer) refreshWorldTable();
}

void PlayerWorldPanel::onConsoleLine(const QString& line) {
    m_playerManager->parseConsoleOutput(line);
}

void PlayerWorldPanel::refreshPlayerTable() {
    m_playerTable->setRowCount(0);
    auto players = m_playerManager->onlinePlayers();
    m_playerCountLabel->setText(TR("panel.player.onlineCount").arg(QString::number(players.size())));
    for (int i = 0; i < players.size(); i++) {
        m_playerTable->insertRow(i);
        m_playerTable->setItem(i, 0, new QTableWidgetItem(players[i].name));
        m_playerTable->setItem(i, 1, new QTableWidgetItem(players[i].uuid));
        m_playerTable->setItem(i, 2, new QTableWidgetItem(players[i].world));
        m_playerTable->setItem(i, 3, new QTableWidgetItem(QString::number(players[i].health)));
        auto* statusItem = new QTableWidgetItem(players[i].isOnline ? TR("panel.player.online") : TR("panel.player.offline"));
        statusItem->setForeground(players[i].isOnline ? QColor("#00d2a0") : QColor("#555577"));
        m_playerTable->setItem(i, 4, statusItem);
        m_playerTable->setRowHeight(i, 36);
    }
}

void PlayerWorldPanel::refreshWorldTable() {
    m_worldTable->setRowCount(0);
    auto worlds = m_worldManager->worlds();
    for (int i = 0; i < worlds.size(); i++) {
        m_worldTable->insertRow(i);
        m_worldTable->setItem(i, 0, new QTableWidgetItem(worlds[i].displayName));
        m_worldTable->setItem(i, 1, new QTableWidgetItem(formatBytes(worlds[i].sizeBytes)));
        m_worldTable->setItem(i, 2, new QTableWidgetItem(QString::number(worlds[i].seed)));
        m_worldTable->setItem(i, 3, new QTableWidgetItem(worlds[i].gameType));
        m_worldTable->setItem(i, 4, new QTableWidgetItem(worlds[i].lastPlayed.toString("yyyy-MM-dd hh:mm")));
        m_worldTable->setRowHeight(i, 36);
    }
}

QString PlayerWorldPanel::selectedPlayer() const {
    int row = m_playerTable->currentRow();
    if (row < 0 || !m_playerTable->item(row, 0)) return QString();
    return m_playerTable->item(row, 0)->text();
}
QString PlayerWorldPanel::selectedWorld() const {
    int row = m_worldTable->currentRow();
    if (row < 0 || !m_worldTable->item(row, 0)) return QString();
    return m_worldTable->item(row, 0)->text();
}
QString PlayerWorldPanel::formatBytes(qint64 bytes) const {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1048576) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1073741824) return QString::number(bytes / 1048576.0, 'f', 1) + " MB";
    return QString::number(bytes / 1073741824.0, 'f', 2) + " GB";
}

void PlayerWorldPanel::onKickPlayer() {
    QString p = selectedPlayer(); if (p.isEmpty()) return;
    QString reason = m_playerReasonEdit->text();
    m_process->sendCommand(PlayerManager::kickCommand(p, reason));
}
void PlayerWorldPanel::onBanPlayer() {
    QString p = selectedPlayer(); if (p.isEmpty()) return;
    QString reason = m_playerReasonEdit->text();
    m_process->sendCommand(PlayerManager::banCommand(p, reason));
}
void PlayerWorldPanel::onOpPlayer() {
    QString p = selectedPlayer(); if (p.isEmpty()) return;
    m_process->sendCommand(PlayerManager::opCommand(p));
}
void PlayerWorldPanel::onDeopPlayer() {
    QString p = selectedPlayer(); if (p.isEmpty()) return;
    m_process->sendCommand(PlayerManager::deopCommand(p));
}
void PlayerWorldPanel::onWhitelistAdd() {
    QString p = selectedPlayer(); if (p.isEmpty()) return;
    m_process->sendCommand(PlayerManager::whitelistAddCommand(p));
}
void PlayerWorldPanel::onGamemodeApply() {
    QString p = selectedPlayer(); if (p.isEmpty()) return;
    m_process->sendCommand(PlayerManager::gamemodeCommand(p, m_gamemodeCombo->currentText()));
}
void PlayerWorldPanel::onRefreshPlayers() { refreshPlayerTable(); }
void PlayerWorldPanel::onBackupWorld() {
    QString w = selectedWorld(); if (w.isEmpty()) return;
    QString dir = QFileDialog::getExistingDirectory(this, TR("panel.world.backupDir"));
    if (!dir.isEmpty()) { m_worldManager->backupWorld(w, dir); QMessageBox::information(this, TR("panel.world.backupDone"), TR("panel.world.backupMsg").arg(w, dir)); }
}
void PlayerWorldPanel::onRestoreWorld() {
    QString zip = QFileDialog::getOpenFileName(this, TR("panel.world.selectBackup"), QString(), "ZIP files (*.zip)");
    if (!zip.isEmpty()) { QString target = m_serverPath; m_worldManager->restoreWorld(zip, target); refreshWorldTable(); }
}
void PlayerWorldPanel::onDeleteWorld() {
    QString w = selectedWorld(); if (w.isEmpty()) return;
    if (QMessageBox::question(this, TR("panel.world.confirmDelete"), TR("panel.world.deleteMsg").arg(w)) == QMessageBox::Yes) {
        m_worldManager->deleteWorld(w);
    }
}
void PlayerWorldPanel::onRefreshWorlds() { refreshWorldTable(); }
