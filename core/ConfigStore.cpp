#include "ConfigStore.h"

ConfigStore::ConfigStore(QObject* parent) : QObject(parent) {
    m_servers = loadAll();
}

QVector<ServerConfig> ConfigStore::servers() const {
    return m_servers;
}

QVector<ServerConfig> ConfigStore::loadAll() {
    QVector<ServerConfig> list;
    QFile file(dataFilePath());
    if (!file.open(QIODevice::ReadOnly)) return list;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return list;
    for (const auto& val : doc.array()) {
        list.append(ServerConfig::fromJson(val.toObject()));
    }
    return list;
}

void ConfigStore::saveServer(const ServerConfig& cfg) {
    // Update or append
    bool found = false;
    for (auto& s : m_servers) {
        if (s.id == cfg.id) { s = cfg; found = true; break; }
    }
    if (!found) m_servers.append(cfg);
    writeFile(m_servers);
    emit serversChanged();
}

void ConfigStore::deleteServer(const QString& id) {
    m_servers.erase(
        std::remove_if(m_servers.begin(), m_servers.end(),
                       [&](const ServerConfig& s) { return s.id == id; }),
        m_servers.end());
    writeFile(m_servers);
    emit serversChanged();
}

ServerConfig* ConfigStore::findServer(const QString& id) {
    for (auto& s : m_servers) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

QString ConfigStore::dataFilePath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/servers.json";
}

void ConfigStore::writeFile(const QVector<ServerConfig>& list) {
    QJsonArray arr;
    for (const auto& s : list) arr.append(s.toJson());
    QFile file(dataFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson());
        file.close();
    }
}
