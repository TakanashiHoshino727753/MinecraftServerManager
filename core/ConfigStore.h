#pragma once
#include <QObject>
#include <QVector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include "models/ServerConfig.h"

class ConfigStore : public QObject {
    Q_OBJECT
public:
    explicit ConfigStore(QObject* parent = nullptr);

    QVector<ServerConfig> servers() const;
    void saveServer(const ServerConfig& cfg);
    void deleteServer(const QString& id);
    ServerConfig* findServer(const QString& id);
    QVector<ServerConfig> loadAll();

signals:
    void serversChanged();

private:
    QString dataFilePath() const;
    void writeFile(const QVector<ServerConfig>& list);
    QVector<ServerConfig> m_servers;
};
