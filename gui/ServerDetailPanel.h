#pragma once
#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QLineEdit>
#include <QDialog>
#include <QCompleter>
#include <QStringListModel>
#include <QTabWidget>
#include "core/ConfigStore.h"
#include "core/ServerProcess.h"
#include "ui_ServerDetailPanel.h"

// ─── MC Command entry for reference / autocomplete ────────────────────
struct McCmdEntry {
    QString cmd;        // Full command (e.g., "time set day")
    QString brief;      // Quick label (e.g., "Set to daytime")
    QString descKey;    // TR key for longer description
    QString catKey;     // Category TR key ("cmd.categoryPlayer", etc.)
};

class ModManagerPanel;
class ModpackPanel;
class PlayerWorldPanel;

class ServerDetailPanel : public QWidget {
    Q_OBJECT
public:
    explicit ServerDetailPanel(ConfigStore* store, ServerProcess* proc, QWidget* parent = nullptr);
    void loadServer(const QString& id);
    void onEditConfig();
    void onEditProperties();

signals:
    void backRequested();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onStartStop();
    void onKill();
    void onDelete();
    void onSendCommand();
    void onConsoleOutput(const QString& line);
    void onStateChanged(ServerProcess::State state);
    void tickUptime();

private:
    void updateUI();
    void buildQuickActions();
    void buildCmdCompleter();
    void showCheatSheet();
    void fillCommand(const QString& cmd);

    Ui::ServerDetailPanel ui;

    ConfigStore*  m_store;
    ServerProcess*m_process;
    QString       m_serverId;
    QString       m_prevServerId;
    ServerConfig  m_config;

    QTimer*           m_uptimeTimer;
    bool              m_autoScroll = true;

    QCompleter*       m_cmdCompleter = nullptr;
    QVector<McCmdEntry> m_allCommands;

    // Per-server sub-tab panels
    QTabWidget*        m_tabWidget = nullptr;
    ModManagerPanel*   m_modPanel = nullptr;
    ModpackPanel*      m_modpackPanel = nullptr;
    PlayerWorldPanel*  m_playerWorldPanel = nullptr;
};
