#pragma once
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

struct ServerConfig {
    QString id;
    QString name;
    QString type;        // "vanilla", "paper", "fabric"
    QString version;
    QString build;       // Paper build number
    QString path;
    QString javaPath = "java";
    int minRamMB = 1024;
    int maxRamMB = 4096;
    QString javaArgs;

    static ServerConfig fromJson(const QJsonObject& obj) {
        ServerConfig cfg;
        cfg.id       = obj["id"].toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
        cfg.name     = obj["name"].toString("Unnamed");
        cfg.type     = obj["type"].toString("paper");
        cfg.version  = obj["version"].toString();
        cfg.build    = obj["build"].toString();
        cfg.path     = obj["path"].toString();
        cfg.javaPath = obj["javaPath"].toString("java");
        cfg.minRamMB = obj["minRam"].toInt(1024);
        cfg.maxRamMB = obj["maxRam"].toInt(4096);
        cfg.javaArgs = obj["javaArgs"].toString();
        return cfg;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"]       = id;
        obj["name"]     = name;
        obj["type"]     = type;
        obj["version"]  = version;
        obj["build"]    = build;
        obj["path"]     = path;
        obj["javaPath"] = javaPath;
        obj["minRam"]   = minRamMB;
        obj["maxRam"]   = maxRamMB;
        obj["javaArgs"] = javaArgs;
        return obj;
    }
};
