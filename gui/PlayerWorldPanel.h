#pragma once
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollArea>
#include "core/ConfigStore.h"
#include "core/ServerProcess.h"
#include "core/PlayerManager.h"
#include "core/WorldManager.h"
#include "ui_PlayerWorldPanel.h"

class PlayerWorldPanel : public QWidget {
    Q_OBJECT
public:
    explicit PlayerWorldPanel(ConfigStore* store, ServerProcess* process, QWidget* parent = nullptr);
    void loadServer(const QString& serverId);
    void refresh();

signals:
    void commandSent(const QString& cmd);

private slots:
    void onKickPlayer();
    void onBanPlayer();
    void onOpPlayer();
    void onDeopPlayer();
    void onWhitelistAdd();
    void onGamemodeApply();
    void onRefreshPlayers();
    void onBackupWorld();
    void onRestoreWorld();
    void onDeleteWorld();
    void onRefreshWorlds();
    void onConsoleLine(const QString& line);

private:
    void refreshPlayerTable();
    void refreshWorldTable();
    QString selectedPlayer() const;
    QString selectedWorld() const;
    QString formatBytes(qint64 bytes) const;

    Ui::PlayerWorldPanel ui;
    ConfigStore*   m_store;
    ServerProcess* m_process;
    PlayerManager* m_playerManager;
    WorldManager*  m_worldManager;
    QString        m_serverId;
    QString        m_serverPath;

    QTabWidget*    m_tabWidget;
    QTableWidget*  m_playerTable;
    QLabel*        m_playerCountLabel;
    QPushButton*   m_kickBtn, *m_banBtn, *m_opBtn, *m_deopBtn, *m_whitelistBtn;
    QComboBox*     m_gamemodeCombo;
    QPushButton*   m_gamemodeBtn;
    QLineEdit*     m_playerReasonEdit;
    QTableWidget*  m_worldTable;
    QPushButton*   m_backupBtn, *m_restoreBtn, *m_deleteWorldBtn;
    bool           m_hasServer = false;
};
