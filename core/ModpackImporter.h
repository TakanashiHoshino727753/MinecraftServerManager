#pragma once
#include <QObject>
#include <QString>
#include "models/ServerConfig.h"

// ─── Modpack Importer ─────────────────────────────────────────────────
// 自动检测整合包类型并提取配置

class ModpackImporter : public QObject {
    Q_OBJECT
public:
    explicit ModpackImporter(QObject* parent = nullptr);

    // Detect server type from a modpack directory or zip file
    struct ModpackInfo {
        QString name;
        QString mcVersion;
        QString loader;      // "vanilla" "forge" "fabric" "neoforge" "quilt"
        QString loaderVersion;
        QString description;
        QStringList modFiles;
        bool valid = false;
    };

    // Analyze a modpack (directory or zip file)
    ModpackInfo analyze(const QString& path);

    // Generate a starter ServerConfig from the modpack
    ServerConfig generateConfig(const ModpackInfo& info, const QString& targetDir);

    // Install modpack by copying files to target server directory
    bool install(const ModpackInfo& info, const QString& targetDir);

signals:
    void progress(const QString& step);
    void error(const QString& message);
    void finished(const ModpackInfo& info);

private:
    ModpackInfo detectFromDirectory(const QString& dirPath);
    ModpackInfo detectFromZip(const QString& zipPath);
    QString extractVersion(const QString& text);
};
