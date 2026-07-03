#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMap>
#include <QStringList>
#include <QTimer>
#include <QElapsedTimer>

class ConfigStore;
class ServerProcess;
class DownloadManager;
class AppSettings;

// ─── Web API Server ────────────────────────────────────────────────
// Serves the WebUI dashboard (browser-based server manager) at /
// Also provides REST API at /api/* for programmatic access.
//
// WebUI (browser dashboard):
//   GET  /                          → Full SPA dashboard HTML
//   GET  /api/*                     → REST API (called by the dashboard JS)
//
// Plugin (Bot) Interface — simplified command gateway:
//   The bot (NapCat/NoneBot) reads QQ messages, parses them,
//   and sends structured commands here. The server manager just executes.
//
//   POST /api/plugin/execute
//     Body: {
//       "action": "list"|"status"|"start"|"stop"|"command"|"console",
//       "serverId": "...",   // required except for "list"
//       "command": "...",    // required for "command" action
//       "limit": 20          // optional for "console" action
//     }
//     Response: {
//       "success": true,
//       "action": "status",
//       "data": { ... action-specific result ... },
//       "message": "ok"
//     }
//
// REST API (also used by WebUI):
//   GET  /api/health               → {"status":"ok"}
//   GET  /api/servers               → {"servers":[...]}
//   POST /api/servers               → create server + trigger download
//   GET  /api/servers/<id>          → {"server":{...}}
//   PUT  /api/servers/<id>          → update server config
//   DELETE /api/servers/<id>        → delete server
//   GET  /api/servers/<id>/status   → {"running":bool,"state":"..."}
//   POST /api/servers/<id>/start    → {"success":bool}
//   POST /api/servers/<id>/stop     → {"success":bool}
//   POST /api/servers/<id>/kill     → force kill server process
//   POST /api/servers/<id>/command  → {"success":bool}  body:{"command":"..."}
//   GET  /api/servers/<id>/console?limit=50 → {"lines":[...]}
//   GET  /api/versions              → {"versions":["1.21.1",...]}
//   GET  /api/download/status       → {"downloading":bool,"percent":0,"status":"..."}
// ────────────────────────────────────────────────────────────────────

class WebApiServer : public QObject {
    Q_OBJECT
public:
    explicit WebApiServer(ConfigStore* store, ServerProcess* proc, DownloadManager* dl = nullptr, AppSettings* appSettings = nullptr, QObject* parent = nullptr);
    ~WebApiServer();

    bool start(int port, const QString& token);
    void stop();
    bool isRunning() const;
    int  port() const { return m_port; }

    // Plugin interface ─ event callbacks for external bots
    struct PluginEvent {
        QString type;       // "server_started","server_stopped","console_output","player_joined","player_left"
        QString serverId;
        QString serverName;
        QJsonObject data;
        QJsonObject toJson() const;
    };
    QVector<PluginEvent> pollEvents();   // consume recent events

signals:
    // Emitted for real-time push to connected clients / plugins
    void serverEvent(const QString& eventType, const QString& serverId, QJsonObject data);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onProcessConsole(const QString& line);

private:
    struct ClientState {
        QByteArray buffer;
        QElapsedTimer timer;
    };

    void handleRequest(QTcpSocket* sock);
    void sendResponse(QTcpSocket* sock, int code, const QByteArray& contentType, const QByteArray& body);
    void sendJson(QTcpSocket* sock, int code, const QJsonObject& obj);
    bool checkAuth(const QMap<QString, QString>& headers, const QUrlQuery& params);

    // Route handlers
    QJsonObject apiHealth();
    QJsonObject apiGetServers();
    QJsonObject apiCreateServer(const QJsonObject& body);  // NEW
    QJsonObject apiGetServer(const QString& id);
    QJsonObject apiUpdateServer(const QString& id, const QJsonObject& body);  // NEW
    QJsonObject apiDeleteServer(const QString& id);   // NEW
    QJsonObject apiGetStatus(const QString& id);
    QJsonObject apiStartServer(const QString& id);
    QJsonObject apiStopServer(const QString& id);
    QJsonObject apiKillServer(const QString& id);      // NEW
    QJsonObject apiSendCommand(const QString& id, const QJsonObject& body);
    QJsonObject apiGetConsole(const QString& id, const QUrlQuery& params);
    QJsonObject apiGetVersions();                      // NEW
    QJsonObject apiGetDownloadStatus();                // NEW
    QJsonObject apiGetSettings();                      // NEW
    QJsonObject apiUpdateSettings(const QJsonObject& body);  // NEW
    QJsonObject apiGetProperties(const QString& serverId);         // NEW: server.properties
    QJsonObject apiUpdateProperties(const QString& serverId, const QJsonObject& body); // NEW
    QJsonObject apiPluginExecute(const QJsonObject& body);  // Bot command gateway
    QJsonObject apiPluginInfo();

    // Mod management
    QJsonObject apiGetMods(const QString& serverId);
    QJsonObject apiRemoveMod(const QString& serverId, const QJsonObject& body);
    QJsonObject apiToggleMod(const QString& serverId, const QJsonObject& body);
    QJsonObject apiGetMarketMods();
    QJsonObject apiUploadMod(const QString& serverId, const QJsonObject& body);

    // Player management
    QJsonObject apiGetPlayers(const QString& serverId);
    QJsonObject apiKickPlayer(const QString& serverId, const QJsonObject& body);
    QJsonObject apiBanPlayer(const QString& serverId, const QJsonObject& body);
    QJsonObject apiOpPlayer(const QString& serverId, const QJsonObject& body);
    QJsonObject apiDeopPlayer(const QString& serverId, const QJsonObject& body);

    // World management
    QJsonObject apiGetWorlds(const QString& serverId);
    QJsonObject apiBackupWorld(const QString& serverId, const QJsonObject& body);
    QJsonObject apiDeleteWorld(const QString& serverId, const QJsonObject& body);

    // Modpack management
    QJsonObject apiAnalyzeModpack(const QJsonObject& body);
    QJsonObject apiInstallModpack(const QJsonObject& body);

    // Modrinth search (PCL2-style mod download)
    QJsonObject apiSearchModsModrinth(const QUrlQuery& params);
    QJsonObject apiGetModrinthVersions(const QString& projectId, const QUrlQuery& params);
    QJsonObject apiDownloadModrinthMod(const QString& serverId, const QJsonObject& body);

    // Mod market category
    QJsonObject apiGetMarketCategories();

    ConfigStore*     m_store;
    ServerProcess*   m_proc;
    DownloadManager* m_dl = nullptr;   // NEW
    AppSettings*     m_appSettings = nullptr;  // NEW
    QTcpServer*      m_server = nullptr;
    QString          m_token;
    int              m_port = 0;
    QMap<QTcpSocket*, ClientState> m_clients;

    // Console buffer per server
    QMap<QString, QStringList> m_consoleBuffers;
    static const int MAX_CONSOLE = 500;

    // Cached versions from Mojang API
    QStringList       m_cachedVersions;   // NEW
    bool              m_downloadActive = false;  // NEW
    int               m_downloadPercent = 0;     // NEW
    QString           m_downloadStatusText;      // NEW

    // Plugin events (for WebUI / external polling)
    QVector<PluginEvent> m_events;
    static const int MAX_EVENTS = 200;
    void pushEvent(const QString& type, const QString& serverId, const QString& serverName = "", QJsonObject data = {});
};
