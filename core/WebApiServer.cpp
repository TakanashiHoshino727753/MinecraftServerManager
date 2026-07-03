#include "WebApiServer.h"
#include "WebUiPage.h"
#include "ServerProperties.h"
#include "ConfigStore.h"
#include "ServerProcess.h"
#include "DownloadManager.h"
#include "AppSettings.h"
#include "LanguageManager.h"
#include "ModManager.h"
#include "PlayerManager.h"
#include "WorldManager.h"
#include "ModpackImporter.h"
#include <QUrlQuery>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDateTime>
#include <QSet>
#include <QTextStream>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// ── PluginEvent ──
QJsonObject WebApiServer::PluginEvent::toJson() const {
    QJsonObject o;
    o["type"]       = type;
    o["serverId"]   = serverId;
    o["serverName"] = serverName;
    o["data"]       = data;
    return o;
}

// ── Construction ──
WebApiServer::WebApiServer(ConfigStore* store, ServerProcess* proc, DownloadManager* dl, AppSettings* appSettings, QObject* parent)
    : QObject(parent), m_store(store), m_proc(proc), m_dl(dl), m_appSettings(appSettings)
{
    connect(m_proc, &ServerProcess::consoleOutput, this, &WebApiServer::onProcessConsole);
    if (m_dl) {
        connect(m_dl, &DownloadManager::versionsReady, this, [this](const QStringList& v) {
            m_cachedVersions = v;
        });
        connect(m_dl, &DownloadManager::downloadProgress, this, [this](const QString& status, int pct) {
            m_downloadActive = (pct >= 0 && pct < 100);
            m_downloadPercent = pct;
            m_downloadStatusText = status;
        });
        connect(m_dl, &DownloadManager::downloadFinished, this, [this](bool ok, const QString& msg) {
            m_downloadActive = false;
            m_downloadPercent = ok ? 100 : -1;
            m_downloadStatusText = msg;
        });
    }
}

WebApiServer::~WebApiServer() {
    stop();
}

bool WebApiServer::start(int port, const QString& token) {
    if (m_server) stop();

    m_port  = port;
    m_token = token;
    m_server = new QTcpServer(this);

    connect(m_server, &QTcpServer::newConnection, this, &WebApiServer::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, port)) {
        delete m_server;
        m_server = nullptr;
        return false;
    }
    return true;
}

void WebApiServer::stop() {
    if (!m_server) return;
    m_server->close();
    for (auto* sock : m_clients.keys()) {
        sock->disconnect();
        sock->close();
        sock->deleteLater();
    }
    m_clients.clear();
    delete m_server;
    m_server = nullptr;
}

bool WebApiServer::isRunning() const {
    return m_server && m_server->isListening();
}

// ── Console buffer ──
void WebApiServer::onProcessConsole(const QString& line) {
    QString sid = m_proc->serverId();
    if (sid.isEmpty()) return;

    auto& buf = m_consoleBuffers[sid];
    buf.append(line);
    while (buf.size() > MAX_CONSOLE) buf.removeFirst();

    // Detect player join/leave from console output
    // Typical format: "PlayerName joined the game" / "PlayerName left the game"
    if (line.contains("joined the game")) {
        int idx = line.indexOf("joined the game");
        QString name = line.left(idx).trimmed();
        // The name might have brackets like [Server], extract last part
        QStringList parts = name.split(' ');
        if (!parts.isEmpty()) {
            name = parts.last();
            name.remove('['); name.remove(']');
            pushEvent("player_joined", sid, "", {{"player", name}});
        }
    } else if (line.contains("left the game")) {
        int idx = line.indexOf("left the game");
        QString name = line.left(idx).trimmed();
        QStringList parts = name.split(' ');
        if (!parts.isEmpty()) {
            name = parts.last();
            name.remove('['); name.remove(']');
            pushEvent("player_left", sid, "", {{"player", name}});
        }
    }
}

// ── Plugin events ──
void WebApiServer::pushEvent(const QString& type, const QString& serverId, const QString& serverName, QJsonObject data) {
    PluginEvent ev;
    ev.type       = type;
    ev.serverId   = serverId;
    ev.serverName = serverName;
    ev.data       = data;
    m_events.append(ev);
    while (m_events.size() > MAX_EVENTS) m_events.removeFirst();

    emit serverEvent(type, serverId, data);
}

QVector<WebApiServer::PluginEvent> WebApiServer::pollEvents() {
    QVector<PluginEvent> result = m_events;
    m_events.clear();
    return result;
}

// ── TCP Connection handling ──

void WebApiServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* sock = m_server->nextPendingConnection();
        ClientState cs;
        cs.timer.start();
        m_clients.insert(sock, cs);
        connect(sock, &QTcpSocket::readyRead,    this, &WebApiServer::onReadyRead);
        connect(sock, &QTcpSocket::disconnected, this, &WebApiServer::onDisconnected);
    }
}

void WebApiServer::onReadyRead() {
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock || !m_clients.contains(sock)) return;
    auto& cs = m_clients[sock];

    cs.buffer.append(sock->readAll());

    // Check for complete HTTP request (ends with \r\n\r\n and body if Content-Length)
    int headerEnd = cs.buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        // Timeout check
        if (cs.timer.elapsed() > 10000) { sock->close(); }
        return;
    }

    QByteArray headerPart = cs.buffer.left(headerEnd);
    int contentLength = 0;
    for (const auto& line : headerPart.split('\n')) {
        if (line.trimmed().toLower().startsWith("content-length:")) {
            contentLength = line.mid(line.indexOf(':') + 1).trimmed().toInt();
            break;
        }
    }

    int totalNeeded = headerEnd + 4 + contentLength;
    if (cs.buffer.size() < totalNeeded) return; // body not complete yet

    // Got full request
    handleRequest(sock);
}

void WebApiServer::onDisconnected() {
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    m_clients.remove(sock);
    sock->deleteLater();
}

// ── HTTP parsing and routing ──

void WebApiServer::handleRequest(QTcpSocket* sock) {
    if (!m_clients.contains(sock)) return;
    const QByteArray& raw = m_clients[sock].buffer;

    int headerEnd = raw.indexOf("\r\n\r\n");
    QByteArray headerPart = raw.left(headerEnd);
    QList<QByteArray> headerLines = headerPart.split('\n');

    if (headerLines.isEmpty()) { sock->close(); return; }

    // Parse request line: METHOD /path?query HTTP/1.1
    QList<QByteArray> reqParts = headerLines.first().trimmed().split(' ');
    if (reqParts.size() < 2) { sock->close(); return; }

    QString method = QString::fromUtf8(reqParts[0]).toUpper();
    QString fullPath = QString::fromUtf8(reqParts[1]);
    QString path = fullPath.section('?', 0, 0);
    QUrlQuery queryParams(fullPath.section('?', 1, -1));

    // Parse headers
    QMap<QString, QString> headers;
    for (int i = 1; i < headerLines.size(); i++) {
        QByteArray line = headerLines[i].trimmed();
        int colon = line.indexOf(':');
        if (colon > 0) {
            headers[QString::fromUtf8(line.left(colon)).trimmed().toLower()] =
                QString::fromUtf8(line.mid(colon + 1)).trimmed();
        }
    }

    // Parse body
    QByteArray body;
    if (headerEnd + 4 < raw.size()) {
        body = raw.mid(headerEnd + 4);
    }

    // Detect WebUI browser access (no auth needed)
    bool isWebUi = (path == "/" || path == "/webui" || path == "/index.html") && method == "GET";

    // Auth check — skip for WebUI (browser doesn't send token) and OPTIONS
    if (!isWebUi && method != "OPTIONS" && !checkAuth(headers, queryParams)) {
        sendJson(sock, 401, {{"error", "unauthorized"}, {"message", "Invalid or missing API token"}});
        m_clients.remove(sock);
        return;
    }

    // CORS preflight
    if (method == "OPTIONS") {
        QByteArray cors = "HTTP/1.1 204 No Content\r\n"
                          "Access-Control-Allow-Origin: *\r\n"
                          "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                          "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                          "Content-Length: 0\r\n\r\n";
        sock->write(cors);
        sock->flush();
        m_clients.remove(sock);
        return;
    }

    // Route
    QJsonObject result;
    int statusCode = 200;
    bool isHtml = false;
    QByteArray htmlBody;

    // GET / — WebUI dashboard (no auth required, token injected)
    if (isWebUi) {
        isHtml = true;
        QByteArray raw = QByteArray(kWebUiHtml);
        raw.replace("\"__TOKEN__\"", "\"" + m_token.toUtf8() + "\"");

        // Inject language
        LanguageManager* lm = LanguageManager::instance();
        QString webUICode = lm->webUILangCode();
        // Resolve "auto" → follow launcher language
        QString langCode = (webUICode == "auto") ? lm->currentLangCode() : webUICode;
        raw.replace("\"__LANG__\"", "\"" + langCode.toUtf8() + "\"");

        // Inject translations JSON for WebUI
        QJsonObject transJson = lm->translationsJson(langCode);
        QByteArray transBytes = QJsonDocument(transJson).toJson(QJsonDocument::Compact);
        raw.replace("__TRANSLATIONS__", transBytes);

        htmlBody = raw;
    }
    // /api/health
    else if (path == "/api/health") {
        result = apiHealth();
    }
    // /api/servers
    else if (path == "/api/servers") {
        if (method == "GET")
            result = apiGetServers();
        else if (method == "POST") {
            QJsonObject reqBody = QJsonDocument::fromJson(body).object();
            result = apiCreateServer(reqBody);
        } else {
            statusCode = 405;
            result = {{"error", "method_not_allowed"}};
        }
    }
    // /api/servers/{id}/...
    else if (path.startsWith("/api/servers/")) {
        QString subPath = path.mid(QString("/api/servers/").length());
        QStringList segs = subPath.split('/', Qt::SkipEmptyParts);
        if (segs.isEmpty()) {
            result = apiGetServers();
        } else {
            QString serverId = segs[0];
            QString action = segs.size() > 1 ? segs[1] : "";

            if (action.isEmpty() || action == "info") {
                if (method == "GET")
                    result = apiGetServer(serverId);
                else if (method == "PUT") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiUpdateServer(serverId, reqBody);
                } else if (method == "DELETE")
                    result = apiDeleteServer(serverId);
                else {
                    statusCode = 405; result = {{"error", "method_not_allowed"}};
                }
            } else if (action == "status") {
                result = apiGetStatus(serverId);
            } else if (action == "start" && method == "POST") {
                result = apiStartServer(serverId);
                ServerConfig* cfg = m_store->findServer(serverId);
                pushEvent("server_started", serverId, cfg ? cfg->name : "");
            } else if (action == "stop" && method == "POST") {
                result = apiStopServer(serverId);
                ServerConfig* cfg = m_store->findServer(serverId);
                pushEvent("server_stopped", serverId, cfg ? cfg->name : "");
            } else if (action == "kill" && method == "POST") {
                result = apiKillServer(serverId);
            } else if (action == "command" && method == "POST") {
                QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                result = apiSendCommand(serverId, reqBody);
            } else if (action == "console") {
                result = apiGetConsole(serverId, queryParams);
            } else if (action == "properties") {
                if (method == "GET")
                    result = apiGetProperties(serverId);
                else if (method == "PUT") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiUpdateProperties(serverId, reqBody);
                } else {
                    statusCode = 405; result = {{"error", "method_not_allowed"}};
                }
            } else if (action == "mods") {
                QString modSub = segs.size() > 2 ? segs[2] : "";
                if (modSub == "remove" && method == "POST") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiRemoveMod(serverId, reqBody);
                } else if (modSub == "toggle" && method == "PUT") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiToggleMod(serverId, reqBody);
                } else if (modSub == "install-modrinth" && method == "POST") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiDownloadModrinthMod(serverId, reqBody);
                } else if (method == "GET") {
                    result = apiGetMods(serverId);
                } else {
                    statusCode = 405; result = {{"error", "method_not_allowed"}};
                }
            } else if (action == "players") {
                QString playerSub = segs.size() > 2 ? segs[2] : "";
                if (playerSub == "kick" && method == "POST") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiKickPlayer(serverId, reqBody);
                } else if (playerSub == "ban" && method == "POST") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiBanPlayer(serverId, reqBody);
                } else if (playerSub == "op" && method == "POST") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiOpPlayer(serverId, reqBody);
                } else if (playerSub == "deop" && method == "POST") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiDeopPlayer(serverId, reqBody);
                } else if (method == "GET") {
                    result = apiGetPlayers(serverId);
                } else {
                    statusCode = 405; result = {{"error", "method_not_allowed"}};
                }
            } else if (action == "worlds") {
                QString worldSub = segs.size() > 2 ? segs[2] : "";
                if (worldSub == "backup" && method == "POST") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiBackupWorld(serverId, reqBody);
                } else if (worldSub == "delete" && method == "POST") {
                    QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                    result = apiDeleteWorld(serverId, reqBody);
                } else if (method == "GET") {
                    result = apiGetWorlds(serverId);
                } else {
                    statusCode = 405; result = {{"error", "method_not_allowed"}};
                }
            } else if (action == "upload-mod" && method == "POST") {
                QJsonObject reqBody = QJsonDocument::fromJson(body).object();
                result = apiUploadMod(serverId, reqBody);
            } else {
                statusCode = 404;
                result = {{"error", "not_found"}, {"message", "Unknown endpoint"}};
            }
        }
    }
    // /api/versions
    else if (path == "/api/versions") {
        result = apiGetVersions();
    }
    // /api/download/status
    else if (path == "/api/download/status") {
        result = apiGetDownloadStatus();
    }
    // /api/settings
    else if (path == "/api/settings") {
        if (method == "GET")
            result = apiGetSettings();
        else if (method == "PUT") {
            QJsonObject reqBody = QJsonDocument::fromJson(body).object();
            result = apiUpdateSettings(reqBody);
        } else {
            statusCode = 405;
            result = {{"error", "method_not_allowed"}};
        }
    }
    // /api/plugin/execute — Bot command gateway (simplified)
    else if (path == "/api/plugin/execute" && method == "POST") {
        QJsonObject reqBody = QJsonDocument::fromJson(body).object();
        result = apiPluginExecute(reqBody);
    }
    // /api/plugin/info — Plugin API docs
    else if (path == "/api/plugin" || path == "/api/plugin/info") {
        result = apiPluginInfo();
    }
    // /api/mods/market — popular mods catalog
    else if (path == "/api/mods/market") {
        result = apiGetMarketMods();
    }
    // /api/mods/categories — market category list
    else if (path == "/api/mods/categories") {
        result = apiGetMarketCategories();
    }
    // /api/mods/search — Modrinth search (PCL2-style)
    else if (path == "/api/mods/search") {
        result = apiSearchModsModrinth(queryParams);
    }
    // /api/mods/modrinth/{projectId}/versions — get project versions from Modrinth
    else if (path.startsWith("/api/mods/modrinth/")) {
        QString subPath = path.mid(QString("/api/mods/modrinth/").length());
        QStringList segs = subPath.split('/', Qt::SkipEmptyParts);
        if (segs.size() >= 2 && segs[1] == "versions") {
            result = apiGetModrinthVersions(segs[0], queryParams);
        } else {
            statusCode = 404; result = {{"error", "not_found"}};
        }
    }
    // /api/modpack/analyze — analyze uploaded modpack (base64)
    else if (path == "/api/modpack/analyze" && method == "POST") {
        QJsonObject reqBody = QJsonDocument::fromJson(body).object();
        result = apiAnalyzeModpack(reqBody);
    }
    // /api/modpack/install — install modpack to server
    else if (path == "/api/modpack/install" && method == "POST") {
        QJsonObject reqBody = QJsonDocument::fromJson(body).object();
        result = apiInstallModpack(reqBody);
    }
    else {
        statusCode = 404;
        result = {{"error", "not_found"}, {"message", "Unknown endpoint"}};
    }

    if (isHtml) {
        sendResponse(sock, 200, "text/html; charset=utf-8", htmlBody);
    } else {
        sendJson(sock, statusCode, result);
    }
    m_clients.remove(sock);
}

// ── Auth ──
bool WebApiServer::checkAuth(const QMap<QString, QString>& headers, const QUrlQuery& params) {
    if (m_token.isEmpty()) return true; // no token required

    // Query param: ?token=xxx
    if (params.hasQueryItem("token") && params.queryItemValue("token") == m_token)
        return true;

    // Header: Authorization: Bearer xxx
    if (headers.contains("authorization")) {
        QString auth = headers["authorization"];
        if (auth.startsWith("Bearer ") && auth.mid(7) == m_token)
            return true;
    }

    // Custom header: X-Auth-Token
    if (headers.contains("x-auth-token") && headers["x-auth-token"] == m_token)
        return true;

    return false;
}

// ── Response helpers ──
void WebApiServer::sendResponse(QTcpSocket* sock, int code, const QByteArray& contentType, const QByteArray& body) {
    QByteArray resp;
    resp += "HTTP/1.1 " + QByteArray::number(code) + " " +
            (code == 200 ? "OK" : code == 401 ? "Unauthorized" : code == 404 ? "Not Found" : "Error") + "\r\n";
    resp += "Content-Type: " + contentType + "\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n";
    resp += "Access-Control-Allow-Headers: Content-Type, Authorization, X-Auth-Token\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;
    sock->write(resp);
    sock->flush();
    sock->disconnectFromHost();
}

void WebApiServer::sendJson(QTcpSocket* sock, int code, const QJsonObject& obj) {
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    sendResponse(sock, code, "application/json; charset=utf-8", body);
}

// ── API Handlers ──

QJsonObject WebApiServer::apiHealth() {
    return {{"status", "ok"}, {"version", "1.0.0"}, {"name", "MC Server Manager API"}};
}

QJsonObject WebApiServer::apiGetServers() {
    QJsonArray arr;
    for (const auto& s : m_store->servers()) {
        bool running = (m_proc->state() == ServerProcess::Running && m_proc->serverId() == s.id);
        QJsonObject o = s.toJson();
        o["running"]    = running;
        o["state"]      = running ? "running" : "stopped";
        // WebUI convenience fields
        o["serverDir"]  = s.path;
        o["serverPort"] = 25565;  // default MC port
        o["javaArgs"]   = !s.javaArgs.isEmpty() ? s.javaArgs
                          : QString("-Xms%1M -Xmx%2M").arg(s.minRamMB).arg(s.maxRamMB);
        arr.append(o);
    }
    QJsonObject result;
    result["servers"] = arr;
    result["total"]   = arr.size();
    return result;
}

QJsonObject WebApiServer::apiGetServer(const QString& id) {
    ServerConfig* cfg = m_store->findServer(id);
    if (!cfg) return {{"error", "not_found"}, {"message", "Server not found"}};

    bool running = (m_proc->state() != ServerProcess::Stopped && m_proc->serverId() == id);
    QJsonObject o = cfg->toJson();
    o["running"] = running;
    o["state"]   = running ? (m_proc->state() == ServerProcess::Running ? "running" : "starting") : "stopped";
    QJsonObject result;
    result["server"] = o;
    return result;
}

QJsonObject WebApiServer::apiGetStatus(const QString& id) {
    ServerConfig* cfg = m_store->findServer(id);
    if (!cfg) return {{"error", "not_found"}, {"message", "Server not found"}};

    bool isMe = (m_proc->serverId() == id);
    ServerProcess::State st = isMe ? m_proc->state() : ServerProcess::Stopped;

    QString stateStr = "stopped";
    if (st == ServerProcess::Starting) stateStr = "starting";
    else if (st == ServerProcess::Running) stateStr = "running";
    else if (st == ServerProcess::Stopping) stateStr = "stopping";

    QJsonObject result;
    result["serverId"]  = id;
    result["serverName"] = cfg->name;
    result["running"]   = (st == ServerProcess::Running);
    result["state"]     = stateStr;
    result["uptimeSecs"] = (st == ServerProcess::Running) ? (qint64)m_proc->uptimeSecs() : 0;
    return result;
}

QJsonObject WebApiServer::apiStartServer(const QString& id) {
    ServerConfig* cfg = m_store->findServer(id);
    if (!cfg) return {{"error", "not_found"}, {"message", "Server not found"}};

    if (m_proc->state() == ServerProcess::Running) {
        if (m_proc->serverId() == id)
            return {{"success", true}, {"message", "Server already running"}, {"serverId", id}};
        return {{"error", "busy"}, {"message", "Another server is already running"}};
    }

    m_proc->start(*cfg);
    return {{"success", true}, {"message", "Server starting..."}, {"serverId", id}};
}

QJsonObject WebApiServer::apiStopServer(const QString& id) {
    if (m_proc->serverId() != id || m_proc->state() == ServerProcess::Stopped)
        return {{"error", "not_running"}, {"message", "Server is not running"}};

    m_proc->stop();
    return {{"success", true}, {"message", "Server stopping..."}, {"serverId", id}};
}

QJsonObject WebApiServer::apiSendCommand(const QString& id, const QJsonObject& body) {
    if (m_proc->serverId() != id || m_proc->state() != ServerProcess::Running)
        return {{"error", "not_running"}, {"message", "Server is not running"}};

    QString cmd = body["command"].toString();
    if (cmd.isEmpty())
        return {{"error", "bad_request"}, {"message", "Missing 'command' field"}};

    m_proc->sendCommand(cmd);
    return {{"success", true}, {"serverId", id}, {"command", cmd}};
}

QJsonObject WebApiServer::apiGetConsole(const QString& id, const QUrlQuery& params) {
    int limit = params.queryItemValue("limit", QUrl::FullyDecoded).toInt();
    if (limit <= 0 || limit > MAX_CONSOLE) limit = 50;

    QJsonArray arr;
    const auto& buf = m_consoleBuffers.value(id);
    int start = qMax(0, buf.size() - limit);
    for (int i = start; i < buf.size(); i++)
        arr.append(buf[i]);

    QJsonObject result;
    result["serverId"] = id;
    result["lines"]    = arr;
    result["total"]    = buf.size();
    return result;
}

// ── CRUD: Create / Update / Delete ──────────────────────────────────

QJsonObject WebApiServer::apiCreateServer(const QJsonObject& body) {
    QString type    = body["type"].toString().toLower();
    QString version = body["version"].toString();
    QString build   = body["build"].toString();
    QString name    = body["name"].toString().trimmed();
    QString path    = body["path"].toString().trimmed();
    QString javaPath = body["javaPath"].toString().trimmed();
    int     minRam  = body["minRamMB"].toInt(1024);
    int     maxRam  = body["maxRamMB"].toInt(4096);
    QString javaArgs= body["javaArgs"].toString().trimmed();
    bool    doDownload = body["download"].toBool(false);  // trigger server jar download

    if (name.isEmpty())  return {{"success", false}, {"message", "Server name is required"}};
    if (path.isEmpty())  return {{"success", false}, {"message", "Server path is required"}};
    if (type.isEmpty())  return {{"success", false}, {"message", "Server type is required"}};
    if (version.isEmpty()) return {{"success", false}, {"message", "Version is required"}};
    if (javaPath.isEmpty()) javaPath = "java";

    // Create config
    ServerConfig cfg;
    cfg.id      = QUuid::createUuid().toString(QUuid::WithoutBraces);
    cfg.name    = name;
    cfg.type    = type;
    cfg.version = version;
    cfg.build   = build;
    cfg.path    = path;
    cfg.javaPath= javaPath;
    cfg.minRamMB= minRam;
    cfg.maxRamMB= maxRam;
    cfg.javaArgs= javaArgs;

    // Ensure directory exists
    QDir dir(path);
    if (!dir.exists()) dir.mkpath(".");

    // Write eula.txt
    QFile eulaFile(path + "/eula.txt");
    if (eulaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        eulaFile.write("eula=true\n");
        eulaFile.close();
    }

    // Save config
    m_store->saveServer(cfg);

    // Trigger download if requested
    QJsonObject result;
    result["success"]  = true;
    result["server"]   = cfg.toJson();
    result["message"]  = "Server created";

    if (doDownload && m_dl) {
        if (type == "vanilla") {
            m_dl->downloadVanilla(version, path);
            result["downloading"] = true;
            result["message"] = "Server created, downloading Vanilla " + version;
        } else if (type == "paper") {
            QString bld = build.isEmpty() ? "latest" : build;
            m_dl->downloadPaper(version, bld, path);
            result["downloading"] = true;
            result["message"] = "Server created, downloading Paper " + version;
        } else if (type == "fabric") {
            m_dl->downloadFabric(version, path);
            result["downloading"] = true;
            result["message"] = "Server created, downloading Fabric " + version;
        }
    }

    return result;
}

QJsonObject WebApiServer::apiUpdateServer(const QString& id, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(id);
    if (!cfg) return {{"success", false}, {"message", "Server not found"}};

    if (body.contains("name"))     cfg->name     = body["name"].toString();
    if (body.contains("type"))     cfg->type     = body["type"].toString();
    if (body.contains("version"))  cfg->version  = body["version"].toString();
    if (body.contains("build"))    cfg->build    = body["build"].toString();
    if (body.contains("path"))     cfg->path     = body["path"].toString();
    if (body.contains("javaPath")) cfg->javaPath = body["javaPath"].toString();
    if (body.contains("minRamMB")) cfg->minRamMB = body["minRamMB"].toInt();
    if (body.contains("maxRamMB")) cfg->maxRamMB = body["maxRamMB"].toInt();
    if (body.contains("javaArgs")) cfg->javaArgs = body["javaArgs"].toString();

    m_store->saveServer(*cfg);
    return {{"success", true}, {"message", "Server updated"}, {"server", cfg->toJson()}};
}

QJsonObject WebApiServer::apiDeleteServer(const QString& id) {
    ServerConfig* cfg = m_store->findServer(id);
    if (!cfg) return {{"success", false}, {"message", "Server not found"}};

    // Stop if running
    if (m_proc->serverId() == id && m_proc->state() != ServerProcess::Stopped) {
        m_proc->kill();
    }

    m_store->deleteServer(id);
    return {{"success", true}, {"message", "Server deleted"}};
}

QJsonObject WebApiServer::apiKillServer(const QString& id) {
    if (m_proc->serverId() != id || m_proc->state() == ServerProcess::Stopped)
        return {{"success", false}, {"message", "Server is not running"}};

    m_proc->kill();
    return {{"success", true}, {"message", "Server forcefully terminated"}};
}

QJsonObject WebApiServer::apiGetVersions() {
    // Trigger async fetch if not yet loaded
    if (m_dl && m_cachedVersions.isEmpty()) {
        m_dl->fetchVersions();
    }

    QJsonArray arr;
    for (const auto& v : m_cachedVersions) arr.append(v);
    QJsonObject result;
    result["versions"] = arr;
    result["count"]    = arr.size();
    result["cached"]   = !m_cachedVersions.isEmpty();
    return result;
}

QJsonObject WebApiServer::apiGetDownloadStatus() {
    QJsonObject result;
    result["downloading"] = m_downloadActive;
    result["percent"]     = m_downloadPercent;
    result["status"]      = m_downloadStatusText;
    return result;
}

// ── Settings ────────────────────────────────────────────────────────

QJsonObject WebApiServer::apiGetSettings() {
    QJsonObject result;
    result["webApiEnabled"] = isRunning();
    result["webApiPort"]    = m_port;
    result["webApiToken"]   = m_token;
    result["webApiUrl"]     = QString("http://localhost:%1").arg(m_port);
    result["botEnabled"]    = false;
    result["botPort"]       = 0;

    if (m_appSettings) {
        result["autoStart"]     = m_appSettings->data.autoStart;
        result["botEnabled"]    = m_appSettings->data.botEnabled;
        result["botPort"]       = m_appSettings->data.botPluginPort;
        result["language"]      = m_appSettings->data.language;
        result["webUILang"]     = m_appSettings->data.webUILang;
    }

    if (m_server) {
        result["uptime"] = QDateTime::currentSecsSinceEpoch(); // approximate
    }

    return result;
}

QJsonObject WebApiServer::apiUpdateSettings(const QJsonObject& body) {
    if (!m_appSettings)
        return {{"success", false}, {"message", "Settings not available"}};

    bool portChanged = false;
    int  newPort = 0;

    if (body.contains("autoStart"))
        m_appSettings->setAutoStart(body["autoStart"].toBool());

    if (body.contains("botEnabled"))
        m_appSettings->data.botEnabled = body["botEnabled"].toBool();

    if (body.contains("botPort"))
        m_appSettings->data.botPluginPort = body["botPort"].toInt(25576);

    // WebUI port change — schedule restart after response is sent
    if (body.contains("webApiPort")) {
        int p = body["webApiPort"].toInt(m_port);
        if (p > 0 && p <= 65535 && p != m_port) {
            m_appSettings->data.webApiPort = p;
            m_appSettings->data.webApiEnabled = true;
            portChanged = true;
            newPort = p;
        }
    }

    // WebUI language (can be set by WebUI settings page)
    if (body.contains("webUILang")) {
        QString lang = body["webUILang"].toString();
        // Accept any valid language code or "auto"
        auto validCodes = LanguageManager::webUILanguageCodes();
        if (validCodes.contains(lang) || lang == "auto") {
            m_appSettings->data.webUILang = lang;
            LanguageManager::instance()->setWebUILangCode(lang);
        }
    }

    // Token can be regenerated
    if (body.contains("regenerateToken") && body["regenerateToken"].toBool()) {
        QString newToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_token = newToken;
        m_appSettings->data.webApiToken = newToken;
    }

    m_appSettings->save();

    QJsonObject result;
    result["success"] = true;
    result["message"] = "Settings saved";

    if (portChanged) {
        result["portChanged"] = true;
        result["newPort"] = newPort;
        result["newUrl"] = QString("http://localhost:%1").arg(newPort);

        // Restart server on new port after response finishes
        QTimer::singleShot(300, this, [this, newPort]() {
            stop();
            start(newPort, m_token);
        });
    }

    return result;
}

// ── Server Properties (uses shared ServerProperties.h) ────────────────

QJsonObject WebApiServer::apiGetProperties(const QString& serverId) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    QString propsPath = cfg->path + "/server.properties";
    QMap<QString, QString> props = readPropertiesFile(propsPath);

    QJsonObject result;
    result["success"] = true;

    // Return all known properties, using defaults for missing ones
    QJsonObject fields;
    for (int i = 0; i < kPropDefCount; i++) {
        const PropDef& d = kPropDefs[i];
        QString key = QString::fromLatin1(d.key);
        QString val = props.contains(key) ? props[key] : QString::fromLatin1(d.defaultVal);
        fields[key] = val;
    }
    result["properties"] = fields;

    // Also return the definitions (category, label, type, options) for frontend rendering
    LanguageManager* lm = LanguageManager::instance();
    QJsonArray defs;
    for (int i = 0; i < kPropDefCount; i++) {
        const PropDef& d = kPropDefs[i];
        QJsonObject def;
        def["key"]      = QString::fromLatin1(d.key);
        def["default"]  = QString::fromLatin1(d.defaultVal);
        def["category"] = QString::fromLatin1(d.category);
        def["label"]    = lm->t(QString("prop.") + QString::fromLatin1(d.key));
        def["type"]     = QString::fromLatin1(d.type);
        def["options"]  = QString::fromLatin1(d.options);
        defs.append(def);
    }
    result["definitions"] = defs;

    return result;
}

QJsonObject WebApiServer::apiUpdateProperties(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    // Build a set of valid keys
    QSet<QString> validKeys;
    for (int i = 0; i < kPropDefCount; i++)
        validKeys.insert(QString::fromLatin1(kPropDefs[i].key));

    QString propsPath = cfg->path + "/server.properties";
    QMap<QString, QString> props = readPropertiesFile(propsPath);

    // Apply updates from request body (only known keys)
    QJsonObject updates = body["properties"].toObject();
    for (auto it = updates.begin(); it != updates.end(); ++it) {
        if (validKeys.contains(it.key())) {
            QString val = it.value().toVariant().toString();
            props[it.key()] = val;
        }
    }

    if (!writePropertiesFile(propsPath, props))
        return {{"success", false}, {"message", "Failed to write server.properties"}};

    return {{"success", true}, {"message", "Properties saved"}, {"count", updates.size()}};
}

// ── Plugin Command Gateway ─────────────────────────────────────────
// The bot (NapCat/NoneBot) reads QQ messages, parses user intent,
// and sends a structured command here. This server just executes.
//
// POST /api/plugin/execute
// Body: {
//   "action":   "list"|"status"|"start"|"stop"|"command"|"console",
//   "serverId": "...",     // required except for "list"
//   "command":  "...",     // required for "command" action
//   "limit":    20         // optional for "console" action (default 20)
// }
// Response: { "success":true, "action":"...", "data":{...}, "message":"ok" }
// ────────────────────────────────────────────────────────────────────

QJsonObject WebApiServer::apiPluginExecute(const QJsonObject& body) {
    QString action   = body["action"].toString().trimmed().toLower();
    QString serverId = body["serverId"].toString().trimmed();

    if (action.isEmpty())
        return {{"success", false}, {"message", "Missing 'action' field"}};

    // ── list: return all servers with status ──
    if (action == "list") {
        QJsonObject d = apiGetServers();
        QJsonObject r;
        r["success"]  = true;
        r["action"]   = "list";
        r["data"]     = d;
        r["message"]  = QString("Found %1 server(s)").arg(d["total"].toInt());
        return r;
    }

    // All other actions require serverId
    if (serverId.isEmpty())
        return {{"success", false}, {"message", "Missing 'serverId' field"}};

    ServerConfig* cfg = m_store->findServer(serverId);
    QString srvName = cfg ? cfg->name : serverId;

    // ── status: server running state ──
    if (action == "status") {
        QJsonObject d = apiGetStatus(serverId);
        QJsonObject r;
        r["success"]  = !d.contains("error");
        r["action"]   = "status";
        r["data"]     = d;
        r["message"]  = d.contains("error") ? d["message"].toString()
                       : QString("Server '%1' is %2").arg(srvName, d["state"].toString());
        return r;
    }

    // ── start: launch a server ──
    if (action == "start") {
        QJsonObject d = apiStartServer(serverId);
        QJsonObject r;
        r["success"]  = d.contains("success") ? d["success"].toBool() : false;
        r["action"]   = "start";
        r["data"]     = d;
        r["message"]  = d.contains("error") ? d["message"].toString()
                       : QString("Server '%1' is starting...").arg(srvName);
        return r;
    }

    // ── stop: stop a running server ──
    if (action == "stop") {
        QJsonObject d = apiStopServer(serverId);
        QJsonObject r;
        r["success"]  = d.contains("success") ? d["success"].toBool() : false;
        r["action"]   = "stop";
        r["data"]     = d;
        r["message"]  = d.contains("error") ? d["message"].toString()
                       : QString("Server '%1' is stopping...").arg(srvName);
        return r;
    }

    // ── command: send console command ──
    if (action == "command") {
        QString cmd = body["command"].toString().trimmed();
        if (cmd.isEmpty())
            return {{"success", false}, {"message", "Missing 'command' field"}};
        QJsonObject cmdBody;
        cmdBody["command"] = cmd;
        QJsonObject d = apiSendCommand(serverId, cmdBody);
        QJsonObject r;
        r["success"]  = d.contains("success") ? d["success"].toBool() : false;
        r["action"]   = "command";
        r["data"]     = d;
        r["message"]  = d.contains("error") ? d["message"].toString()
                       : QString("Command '%1' sent to '%2'").arg(cmd, srvName);
        return r;
    }

    // ── console: get recent console output ──
    if (action == "console") {
        QUrlQuery q;
        int limit = body["limit"].toInt(20);
        if (limit < 1) limit = 20;
        if (limit > MAX_CONSOLE) limit = MAX_CONSOLE;
        q.addQueryItem("limit", QString::number(limit));
        QJsonObject d = apiGetConsole(serverId, q);
        QJsonObject r;
        r["success"]  = true;
        r["action"]   = "console";
        r["data"]     = d;
        r["message"]  = QString("Last %1 lines of console output").arg(limit);
        return r;
    }

    return {{"success", false}, {"message", QString("Unknown action: '%1'").arg(action)}};
}

QJsonObject WebApiServer::apiPluginInfo() {
    QJsonObject result;
    result["name"]     = "MC Server Manager Plugin API";
    result["version"]  = "1.1.0";
    result["protocol"] = "HTTP — Simplified Command Gateway";

    QJsonArray endpoints;
    endpoints.append(QJsonObject{
        {"method", "POST"}, {"path", "/api/plugin/execute"},
        {"desc", "Execute server command (bot sends parsed action)"}
    });
    result["endpoints"]  = endpoints;

    QJsonObject auth;
    auth["type"]        = "Bearer Token";
    auth["header"]      = "Authorization: Bearer <token>";
    auth["alt_header"]  = "X-Auth-Token: <token>";
    auth["alt_query"]   = "?token=<token>";
    result["authentication"] = auth;

    // Documentation for bot developers
    QJsonObject actions;
    actions["list"]    = "List all servers and their status. No serverId needed.";
    actions["status"]  = "Get server running state (requires serverId).";
    actions["start"]   = "Start a server (requires serverId).";
    actions["stop"]    = "Stop a running server (requires serverId).";
    actions["command"] = "Send a console command (requires serverId + command string).";
    actions["console"] = "Get recent console output (requires serverId, optional limit).";
    result["actions"]  = actions;

    // NapCat integration guide
    QJsonObject napcat;
    napcat["name"]        = "NapCat (QQ Bot)";
    napcat["description"] = "QQ bot framework — reads messages, parses intent, sends structured commands here.";
    napcat["endpoint"]    = "POST /api/plugin/execute";
    napcat["flow"]        = "QQ msg → NapCat reads → parse intent → POST execute → return result to QQ";
    QJsonObject napcatCode;
    napcatCode["javascript"] = R"(
// NapCat handler example: parse QQ msg and call execute
async function handleMcCommand(msg, userId) {
  const text = msg.trim();
  let action, serverId = 'default';

  // Simple intent parsing (you can use NLP/AI here)
  if (text.includes('列表') || text.includes('服务器列表')) {
    action = 'list';
  } else if (text.includes('状态') || text.includes('status')) {
    action = 'status';
  } else if (text.includes('启动') || text.includes('开启')) {
    action = 'start';
  } else if (text.includes('停止') || text.includes('关闭')) {
    action = 'stop';
  } else if (text.includes('命令') || text.includes('执行')) {
    action = 'command';
    const cmdMatch = text.match(/命令\s+(\S+)/);
    // Extract command from message...
  } else { return 'Unknown command'; }

  const resp = await fetch('http://localhost:25575/api/plugin/execute', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', 'Authorization': 'Bearer TOKEN' },
    body: JSON.stringify({ action, serverId })
  });
  const data = await resp.json();
  return data.message; // Reply to QQ user
}
)";
    napcat["code_example"] = napcatCode;
    result["napcat"] = napcat;

    // NoneBot integration guide
    QJsonObject nonebot;
    nonebot["name"]        = "NoneBot (Python)";
    nonebot["description"] = "Python bot framework — write a handler that uses httpx to call this API.";
    nonebot["endpoint"]    = "POST /api/plugin/execute";
    nonebot["flow"]        = "QQ/WeChat msg → NoneBot handler → parse → POST execute → format reply";
    QJsonObject nbCode;
    nbCode["python"] = R"(
# NoneBot handler example
import httpx
from nonebot import on_command
from nonebot.adapters import Message

mc = on_command("mc", priority=5)

@mc.handle()
async def handle_mc():
    args = str(await mc.get_arg("args")).strip().split()
    if not args:
        await mc.finish("用法: /mc <list|status|start|stop|cmd> [服务器ID]")

    action = args[0]
    server_id = args[1] if len(args) > 1 else "default"

    async with httpx.AsyncClient() as client:
        resp = await client.post(
            "http://localhost:25575/api/plugin/execute",
            json={"action": action, "serverId": server_id},
            headers={"Authorization": "Bearer YOUR_TOKEN"}
        )
        data = resp.json()
        await mc.finish(data.get("message", "执行完成"))
)";
    nonebot["code_example"] = nbCode;
    result["nonebot"] = nonebot;

    return result;
}

// ── Mod Management ──────────────────────────────────────────────────

QJsonObject WebApiServer::apiGetMods(const QString& serverId) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    ModManager mm;
    mm.scan(cfg->path, cfg->type);

    QJsonArray arr;
    for (const auto& m : mm.mods())
        arr.append(m.toJson());

    // Check conflicts
    QJsonArray conflicts;
    auto conflictPairs = mm.checkConflicts();
    for (const auto& pair : conflictPairs) {
        QJsonObject c;
        c["modA"] = pair.first.toJson();
        c["modB"] = pair.second.toJson();
        c["reason"] = (pair.first.loader != "any" && pair.second.loader != "any"
                        && pair.first.loader != pair.second.loader)
                           ? "loader_mismatch"
                           : "conflict_declared";
        conflicts.append(c);
    }

    QJsonObject result;
    result["success"] = true;
    result["mods"]    = arr;
    result["total"]   = arr.size();
    result["conflicts"] = conflicts;
    result["path"]     = cfg->path;
    result["loader"]   = cfg->type;
    return result;
}

QJsonObject WebApiServer::apiRemoveMod(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    QString fileName = body["fileName"].toString();
    if (fileName.isEmpty())
        return {{"success", false}, {"message", "Missing 'fileName'"}};

    ModManager mm;
    mm.scan(cfg->path, cfg->type);
    if (mm.removeMod(fileName))
        return {{"success", true}, {"message", "Mod removed"}};
    return {{"success", false}, {"message", "Failed to remove mod"}};
}

QJsonObject WebApiServer::apiToggleMod(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    QString fileName = body["fileName"].toString();
    if (fileName.isEmpty())
        return {{"success", false}, {"message", "Missing 'fileName'"}};

    bool enabled = body["enabled"].toBool(true);

    ModManager mm;
    mm.scan(cfg->path, cfg->type);
    if (mm.setModEnabled(fileName, enabled)) {
        QString state = enabled ? "enabled" : "disabled";
        return {{"success", true}, {"message", "Mod " + state}};
    }
    return {{"success", false}, {"message", "Failed to toggle mod"}};
}

QJsonObject WebApiServer::apiGetMarketMods() {
    QJsonArray arr;
    const auto& marketMods = ModManager::marketMods();
    for (const auto& m : marketMods) {
        QJsonObject obj;
        obj["id"]          = m.id;
        obj["name"]        = m.name;
        obj["description"] = m.description;
        obj["loader"]      = m.loader;
        QJsonArray vers;
        for (const auto& v : m.mcVersions)
            vers.append(v);
        obj["mcVersions"]  = vers;
        arr.append(obj);
    }
    QJsonObject result;
    result["success"] = true;
    result["mods"]    = arr;
    result["total"]   = arr.size();
    return result;
}

// ── Mod Upload (base64) ──────────────────────────────────────────────

QJsonObject WebApiServer::apiUploadMod(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    QString fileName = body["fileName"].toString();
    QString base64Data = body["data"].toString();
    if (fileName.isEmpty() || base64Data.isEmpty())
        return {{"success", false}, {"message", "Missing 'fileName' or 'data'"}};

    // Decode base64
    QByteArray fileData = QByteArray::fromBase64(base64Data.toUtf8());
    if (fileData.isEmpty())
        return {{"success", false}, {"message", "Invalid or empty file data"}};

    // Save to mods folder
    QString modsPath = cfg->path + "/mods";
    QDir().mkpath(modsPath);
    QFile outFile(modsPath + "/" + fileName);
    if (!outFile.open(QIODevice::WriteOnly))
        return {{"success", false}, {"message", "Cannot write file to mods folder"}};

    outFile.write(fileData);
    outFile.close();

    return {{"success", true}, {"message", "Mod uploaded successfully"}, {"fileName", fileName}};
}

// ── Player Management ────────────────────────────────────────────────

QJsonObject WebApiServer::apiGetPlayers(const QString& serverId) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    // Execute "list" command to get online players
    bool wasRunning = (m_proc->state() != ServerProcess::Stopped && m_proc->serverId() == serverId);
    if (!wasRunning)
        return {{"success", true}, {"players", QJsonArray()}, {"total", 0},
                {"serverOnline", false}, {"message", "Server is not running"}};

    // Parse players from console — use m_consoleBuffers' recent lines for joined events
    PlayerManager pm;
    const auto& lines = m_consoleBuffers.value(serverId);
    for (const auto& line : lines)
        pm.parseConsoleOutput(line);

    QJsonArray arr;
    for (const auto& p : pm.onlinePlayers())
        arr.append(p.toJson());

    return {{"success", true}, {"players", arr}, {"total", arr.size()},
            {"serverOnline", true}};
}

QJsonObject WebApiServer::apiKickPlayer(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};
    if (m_proc->state() != ServerProcess::Running || m_proc->serverId() != serverId)
        return {{"success", false}, {"message", "Server is not running"}};

    QString player = body["player"].toString();
    QString reason = body["reason"].toString();
    if (player.isEmpty())
        return {{"success", false}, {"message", "Missing 'player'"}};

    QString cmd = PlayerManager::kickCommand(player, reason);
    m_proc->sendCommand(cmd);
    return {{"success", true}, {"message", "Kick command sent"}, {"command", cmd}};
}

QJsonObject WebApiServer::apiBanPlayer(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};
    if (m_proc->state() != ServerProcess::Running || m_proc->serverId() != serverId)
        return {{"success", false}, {"message", "Server is not running"}};

    QString player = body["player"].toString();
    QString reason = body["reason"].toString();
    if (player.isEmpty())
        return {{"success", false}, {"message", "Missing 'player'"}};

    QString cmd = PlayerManager::banCommand(player, reason);
    m_proc->sendCommand(cmd);
    return {{"success", true}, {"message", "Ban command sent"}, {"command", cmd}};
}

QJsonObject WebApiServer::apiOpPlayer(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};
    if (m_proc->state() != ServerProcess::Running || m_proc->serverId() != serverId)
        return {{"success", false}, {"message", "Server is not running"}};

    QString player = body["player"].toString();
    if (player.isEmpty())
        return {{"success", false}, {"message", "Missing 'player'"}};

    QString cmd = PlayerManager::opCommand(player);
    m_proc->sendCommand(cmd);
    return {{"success", true}, {"message", "Op command sent"}, {"command", cmd}};
}

QJsonObject WebApiServer::apiDeopPlayer(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};
    if (m_proc->state() != ServerProcess::Running || m_proc->serverId() != serverId)
        return {{"success", false}, {"message", "Server is not running"}};

    QString player = body["player"].toString();
    if (player.isEmpty())
        return {{"success", false}, {"message", "Missing 'player'"}};

    QString cmd = PlayerManager::deopCommand(player);
    m_proc->sendCommand(cmd);
    return {{"success", true}, {"message", "Deop command sent"}, {"command", cmd}};
}

// ── World Management ─────────────────────────────────────────────────

QJsonObject WebApiServer::apiGetWorlds(const QString& serverId) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    WorldManager wm;
    wm.scan(cfg->path);

    QJsonArray arr;
    for (const auto& w : wm.worlds())
        arr.append(w.toJson());

    return {{"success", true}, {"worlds", arr}, {"total", arr.size()}};
}

QJsonObject WebApiServer::apiBackupWorld(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    QString worldName = body["world"].toString();
    if (worldName.isEmpty())
        return {{"success", false}, {"message", "Missing 'world'"}};

    // Default backup dir: serverPath/backups/
    QString backupDir = cfg->path + "/backups";
    QDir().mkpath(backupDir);

    WorldManager wm;
    wm.scan(cfg->path);
    if (wm.backupWorld(worldName, backupDir)) {
        QString fname = WorldManager::backupFileName(worldName);
        return {{"success", true}, {"message", "World backup created"},
                {"backupFile", fname}, {"backupPath", backupDir + "/" + fname}};
    }
    return {{"success", false}, {"message", "Backup failed — world not found or disk error"}};
}

QJsonObject WebApiServer::apiDeleteWorld(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    QString worldName = body["world"].toString();
    if (worldName.isEmpty())
        return {{"success", false}, {"message", "Missing 'world'"}};

    WorldManager wm;
    wm.scan(cfg->path);
    if (wm.deleteWorld(worldName))
        return {{"success", true}, {"message", "World deleted"}};
    return {{"success", false}, {"message", "Delete failed — world not found"}};
}

// ── Modpack Management ───────────────────────────────────────────────

QJsonObject WebApiServer::apiAnalyzeModpack(const QJsonObject& body) {
    QString base64Data = body["data"].toString();
    QString fileName = body["fileName"].toString();

    if (base64Data.isEmpty() && body.contains("path")) {
        // Direct server path analysis
        QString localPath = body["path"].toString();
        QFileInfo fi(localPath);
        if (!fi.exists())
            return {{"success", false}, {"message", "Path does not exist: " + localPath}};

        ModpackImporter importer;
        auto info = importer.analyze(localPath);
        if (!info.valid)
            return {{"success", false}, {"message", "Could not detect modpack type"}};

        QJsonObject result;
        result["success"]        = true;
        result["name"]           = info.name;
        result["mcVersion"]      = info.mcVersion;
        result["loader"]         = info.loader;
        result["loaderVersion"]  = info.loaderVersion;
        result["description"]    = info.description;
        result["modCount"]       = info.modFiles.size();
        result["sourcePath"]     = localPath;
        return result;
    }

    if (base64Data.isEmpty())
        return {{"success", false}, {"message", "Missing 'data' (base64) or 'path'"}};

    // Decode base64 zip to temp dir, then analyze
    QByteArray zipData = QByteArray::fromBase64(base64Data.toUtf8());
    if (zipData.isEmpty())
        return {{"success", false}, {"message", "Empty file data"}};

    // Write to temp file
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempZip = tempDir + "/mcsm_modpack_upload.zip";
    QFile f(tempZip);
    if (!f.open(QIODevice::WriteOnly))
        return {{"success", false}, {"message", "Cannot create temp file"}};
    f.write(zipData);
    f.close();

    ModpackImporter importer;
    auto info = importer.analyze(tempZip);
    if (!info.valid)
        return {{"success", false}, {"message", "Could not detect modpack type. Ensure it contains a mods/ folder or manifest.json"}};

    QJsonObject result;
    result["success"]        = true;
    result["name"]           = info.name;
    result["mcVersion"]      = info.mcVersion;
    result["loader"]         = info.loader;
    result["loaderVersion"]  = info.loaderVersion;
    result["description"]    = info.description;
    result["modCount"]       = info.modFiles.size();
    result["tempFile"]       = tempZip;
    return result;
}

QJsonObject WebApiServer::apiInstallModpack(const QJsonObject& body) {
    QString tempFile = body["tempFile"].toString();
    QString targetPath = body["targetPath"].toString();
    QString serverName = body["name"].toString();
    QString mcVersion = body["mcVersion"].toString();
    QString loader = body["loader"].toString();
    QString loaderVersion = body["loaderVersion"].toString();
    int minRam = body["minRamMB"].toInt(1024);
    int maxRam = body["maxRamMB"].toInt(4096);

    if (tempFile.isEmpty() || targetPath.isEmpty())
        return {{"success", false}, {"message", "Missing 'tempFile' or 'targetPath'"}};

    if (serverName.isEmpty()) serverName = "Modpack Server";
    if (!mcVersion.isEmpty()) {
        // Clean MC version from tempFile analysis
        // We'll use what was analyzed
    }

    ModpackImporter importer;
    auto info = importer.analyze(tempFile);
    if (!info.valid)
        return {{"success", false}, {"message", "Modpack analysis failed"}};

    // Create target directory
    QDir targetDir(targetPath);
    if (!targetDir.exists()) {
        if (!targetDir.mkpath("."))
            return {{"success", false}, {"message", "Cannot create target directory"}};
    }

    // Install modpack files
    if (!importer.install(info, targetPath))
        return {{"success", false}, {"message", "Failed to install modpack files"}};

    // Generate and save server config
    ServerConfig cfg = importer.generateConfig(info, targetPath);
    cfg.name = serverName;
    if (!mcVersion.isEmpty()) cfg.version = mcVersion;
    if (!loader.isEmpty()) cfg.type = loader;
    cfg.minRamMB = minRam;
    cfg.maxRamMB = maxRam;

    m_store->saveServer(cfg);

    // Clean up temp file
    QFile::remove(tempFile);

    QJsonObject result;
    result["success"]     = true;
    result["message"]     = "Modpack installed successfully";
    result["serverId"]    = cfg.id;
    result["serverName"]  = cfg.name;
    result["serverPath"]  = cfg.path;
    return result;
}

// ── Modrinth Search (PCL2-style) ─────────────────────────────────────

static QJsonObject fetchModrinthObject(const QString& url) {
    QNetworkAccessManager mgr;
    QUrl qurl(url);
    QNetworkRequest req{qurl};
    req.setRawHeader("User-Agent", "MCServerManager/2.0 (mod-search)");
    req.setRawHeader("Accept", "application/json");
    QNetworkReply* reply = mgr.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return {};
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();
    return QJsonDocument::fromJson(data).object();
}

static QJsonArray fetchModrinthArray(const QString& url) {
    QNetworkAccessManager mgr;
    QUrl qurl(url);
    QNetworkRequest req{qurl};
    req.setRawHeader("User-Agent", "MCServerManager/2.0 (mod-search)");
    req.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = mgr.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    return QJsonDocument::fromJson(data).array();
}

QJsonObject WebApiServer::apiSearchModsModrinth(const QUrlQuery& params) {
    QString query = params.queryItemValue("query");
    QString loader = params.queryItemValue("loader");
    QString version = params.queryItemValue("version");
    int offset = params.queryItemValue("offset").toInt();
    int limit = qBound(1, params.queryItemValue("limit").toInt(), 20);

    if (query.isEmpty())
        return {{"success", false}, {"message", "Missing 'query' parameter"}};

    // Build Modrinth search facets
    QStringList facets;
    facets << "[\"project_type:mod\"]";

    if (!loader.isEmpty() && loader != "vanilla") {
        // Map loader names to Modrinth categories
        QString cat = loader.toLower();
        if (cat == "fabric") facets << "[\"categories:fabric\"]";
        else if (cat == "forge") facets << "[\"categories:forge\"]";
        else if (cat == "neoforge") facets << "[\"categories:neoforge\"]";
        else if (cat == "quilt") facets << "[\"categories:quilt\"]";
    }

    if (!version.isEmpty())
        facets << "[\"versions:" + version + "\"]";

    QString facetsStr = "[" + facets.join(",") + "]";

    QString url = "https://api.modrinth.com/v2/search?query="
        + QUrl::toPercentEncoding(query)
        + "&facets=" + QUrl::toPercentEncoding(facetsStr)
        + "&offset=" + QString::number(offset)
        + "&limit=" + QString::number(limit);

    QJsonObject modrinthResp = fetchModrinthObject(url);
    if (modrinthResp.isEmpty())
        return {{"success", false}, {"message", "Modrinth API unreachable"}};

    QJsonArray hits = modrinthResp["hits"].toArray();
    QJsonArray results;
    for (const auto& hit : hits) {
        QJsonObject h = hit.toObject();
        QJsonObject mod;
        mod["projectId"]   = h["project_id"];
        mod["slug"]        = h["slug"];
        mod["name"]        = h["title"];
        mod["description"] = h["description"];
        mod["iconUrl"]     = h["icon_url"];
        mod["downloads"]   = h["downloads"];
        mod["author"]      = h["author"];
        mod["categories"]  = h["categories"];
        mod["versions"]    = h["versions"]; // compatible MC versions
        results.append(mod);
    }

    return {{"success", true}, {"results", results}, {"total", modrinthResp["total_hits"].toInt()},
            {"offset", offset}, {"limit", limit}};
}

QJsonObject WebApiServer::apiGetModrinthVersions(const QString& projectId, const QUrlQuery& params) {
    QString loader = params.queryItemValue("loader");
    QString mcVersion = params.queryItemValue("version");

    QString url = "https://api.modrinth.com/v2/project/" + projectId + "/version";

    // Filter by loader + version using query
    QStringList loaders;
    if (!loader.isEmpty() && loader != "vanilla")
        loaders << loader;

    QStringList gameVersions;
    if (!mcVersion.isEmpty())
        gameVersions << mcVersion;

    if (!loaders.isEmpty() || !gameVersions.isEmpty()) {
        QJsonObject filter;
        if (!loaders.isEmpty()) {
            QJsonArray lArr;
            for (const auto& l : loaders) lArr.append(l);
            filter["loaders"] = lArr;
        }
        if (!gameVersions.isEmpty()) {
            QJsonArray gArr;
            for (const auto& v : gameVersions) gArr.append(v);
            filter["game_versions"] = gArr;
        }
        QString filterStr = QJsonDocument(filter).toJson(QJsonDocument::Compact);
        url += "?filter=" + QUrl::toPercentEncoding(filterStr);
    }


    QJsonArray versions = fetchModrinthArray(url);
    if (versions.isEmpty()) {
        // Try without filter
        url = "https://api.modrinth.com/v2/project/" + projectId + "/version";
        versions = fetchModrinthArray(url);
    }

    QJsonArray result;
    for (const auto& v : versions) {
        QJsonObject vo = v.toObject();
        QJsonObject rv;
        rv["id"]            = vo["id"];
        rv["name"]          = vo["name"];
        rv["versionNumber"] = vo["version_number"];
        rv["loaders"]       = vo["loaders"];
        rv["gameVersions"]  = vo["game_versions"];

        // Extract primary download URL
        QJsonArray files = vo["files"].toArray();
        if (!files.isEmpty()) {
            QJsonObject primary = files[0].toObject();
            rv["downloadUrl"]  = primary["url"];
            rv["fileName"]     = primary["filename"];
            rv["fileSize"]     = primary["size"];
        }
        result.append(rv);
    }

    return {{"success", true}, {"projectId", projectId}, {"versions", result}};
}

QJsonObject WebApiServer::apiDownloadModrinthMod(const QString& serverId, const QJsonObject& body) {
    ServerConfig* cfg = m_store->findServer(serverId);
    if (!cfg)
        return {{"success", false}, {"message", "Server not found"}};

    QString downloadUrl = body["downloadUrl"].toString();
    QString fileName = body["fileName"].toString();
    QString projectName = body["projectName"].toString();
    QString versionId = body["versionId"].toString();

    // If versionId is provided, resolve download URL from Modrinth API
    if (downloadUrl.isEmpty() && !versionId.isEmpty()) {
        QString verUrl = "https://api.modrinth.com/v2/version/" + versionId;
        QJsonObject verInfo = fetchModrinthObject(verUrl);
        if (verInfo.isEmpty())
            return {{"success", false}, {"message", "Failed to fetch version info from Modrinth"}};
        QJsonArray files = verInfo["files"].toArray();
        if (files.isEmpty())
            return {{"success", false}, {"message", "No download files available"}};
        // Pick primary file or first available
        QString primaryUrl;
        for (const auto& f : files) {
            QJsonObject fo = f.toObject();
            if (fo["primary"].toBool()) {
                primaryUrl = fo["url"].toString();
                fileName = fo["filename"].toString();
                break;
            }
        }
        if (primaryUrl.isEmpty()) {
            QJsonObject fo = files.first().toObject();
            primaryUrl = fo["url"].toString();
            fileName = fo["filename"].toString();
        }
        if (primaryUrl.isEmpty())
            return {{"success", false}, {"message", "No download URL found"}};
        downloadUrl = primaryUrl;
        if (projectName.isEmpty())
            projectName = verInfo["name"].toString();
    }

    if (downloadUrl.isEmpty())
        return {{"success", false}, {"message", "Missing 'downloadUrl' or 'versionId'"}};

    // Download the mod file
    QNetworkAccessManager mgr;
    QUrl dUrl(downloadUrl);
    QNetworkRequest req{dUrl};
    req.setRawHeader("User-Agent", "MCServerManager/2.0 (mod-download)");

    QNetworkReply* reply = mgr.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return {{"success", false}, {"message", "Download failed: " + reply->errorString()}};
    }

    QByteArray modData = reply->readAll();
    reply->deleteLater();

    if (modData.isEmpty())
        return {{"success", false}, {"message", "Downloaded empty file"}};

    // Ensure mods folder exists
    QString modsPath = cfg->path + "/mods";
    QDir().mkpath(modsPath);

    // Determine filename
    if (fileName.isEmpty()) {
        // Extract from URL or use project name
        QStringList urlParts = downloadUrl.split('/');
        fileName = urlParts.last();
        if (fileName.contains('?')) fileName = fileName.section('?', 0, 0);
    }

    QString targetPath = modsPath + "/" + fileName;
    QFile outFile(targetPath);
    if (!outFile.open(QIODevice::WriteOnly))
        return {{"success", false}, {"message", "Cannot write mod to server"}};

    outFile.write(modData);
    outFile.close();

    return {{"success", true}, {"message", "Mod installed successfully"},
            {"fileName", fileName}, {"projectName", projectName}};
}

// ── Market Categories ────────────────────────────────────────────────

QJsonObject WebApiServer::apiGetMarketCategories() {
    QJsonArray categories;
    QStringList cats = {
        "optimization", "worldgen", "technology", "magic", "storage",
        "transport", "decoration", "adventure", "utility", "library"
    };
    for (const auto& c : cats) {
        QJsonObject cat;
        cat["id"] = c;
        cat["name"] = c;
        categories.append(cat);
    }
    return {{"success", true}, {"categories", categories}};
}
