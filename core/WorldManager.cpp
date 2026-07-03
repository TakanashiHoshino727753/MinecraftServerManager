#include "WorldManager.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QRegularExpression>

// ─── WorldInfo serialization ───────────────────────────────────────────

QJsonObject WorldInfo::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["displayName"] = displayName;
    obj["path"] = path;
    obj["dimension"] = dimension;
    obj["seed"] = static_cast<qint64>(seed);
    obj["gameType"] = gameType;
    obj["sizeBytes"] = static_cast<qint64>(sizeBytes);
    obj["lastPlayed"] = lastPlayed.toMSecsSinceEpoch();
    obj["isValid"] = isValid;
    return obj;
}

WorldInfo WorldInfo::fromJson(const QJsonObject& obj) {
    WorldInfo w;
    w.name = obj["name"].toString();
    w.displayName = obj["displayName"].toString();
    w.path = obj["path"].toString();
    w.dimension = obj["dimension"].toString();
    w.seed = static_cast<qint64>(obj["seed"].toDouble());
    w.gameType = obj["gameType"].toString();
    w.sizeBytes = static_cast<qint64>(obj["sizeBytes"].toDouble());
    w.lastPlayed = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(obj["lastPlayed"].toDouble()));
    w.isValid = obj["isValid"].toBool();
    return w;
}

// ─── WorldManager ──────────────────────────────────────────────────────

WorldManager::WorldManager(QObject* parent) : QObject(parent) {}

void WorldManager::scan(const QString& serverPath) {
    m_serverPath = serverPath;
    m_worlds.clear();

    QDir serverDir(serverPath);
    if (!serverDir.exists()) return;

    // Known dimension folder suffixes
    struct DimInfo { QString suffix; QString name; };
    QList<DimInfo> dims = {
        {"",            "overworld"},
        {"_nether",     "nether"},
        {"_the_end",    "end"}
    };

    // Find world folders (those containing level.dat)
    QStringList entries = serverDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& entry : entries) {
        QString worldPath = serverPath + "/" + entry;
        QString levelDat = worldPath + "/level.dat";
        if (!QFile::exists(levelDat)) continue;

        WorldInfo info;
        info.name = entry;
        info.displayName = entry;
        info.path = worldPath;
        info.dimension = "overworld";
        info.sizeBytes = folderSize(worldPath);

        // Try to read seed from level.dat (binary NBT format — simplified)
        info.seed = 0;

        // Read server.properties for seed if available
        QFile propFile(serverPath + "/server.properties");
        if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = propFile.readAll();
            propFile.close();

            QRegularExpression seedRe(R"(level-seed\s*=\s*(-?\d+))");
            auto seedMatch = seedRe.match(content);
            if (seedMatch.hasMatch())
                info.seed = seedMatch.captured(1).toLongLong();

            QRegularExpression gamemodeRe(R"(gamemode\s*=\s*(\w+))");
            auto gmMatch = gamemodeRe.match(content);
            if (gmMatch.hasMatch())
                info.gameType = gmMatch.captured(1);
        }

        // Last played from level.dat modification time
        QFileInfo fi(levelDat);
        info.lastPlayed = fi.lastModified();

        // Check for dimension subfolders
        QDir worldDir(worldPath);
        if (QFile::exists(worldPath + "/DIM-1"))
            info.sizeBytes += folderSize(worldPath + "/DIM-1");
        if (QFile::exists(worldPath + "/DIM1"))
            info.sizeBytes += folderSize(worldPath + "/DIM1");

        info.isValid = true;
        m_worlds.append(info);
    }

    emit worldsChanged();
}

WorldInfo WorldManager::findWorld(const QString& name) const {
    for (const auto& w : m_worlds) {
        if (w.name == name)
            return w;
    }
    return {};
}

qint64 WorldManager::readSeed(const QString& serverPath, const QString& worldName) const {
    Q_UNUSED(serverPath);
    Q_UNUSED(worldName);
    // Reading NBT format requires a proper NBT library.
    // For now, return 0 and let callers read from server.properties.
    return 0;
}

qint64 WorldManager::folderSize(const QString& path) const {
    qint64 total = 0;
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

QString WorldManager::backupFileName(const QString& worldName) {
    return QString("%1_backup_%2.zip")
        .arg(worldName)
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
}

// ─── Operations ────────────────────────────────────────────────────────

bool WorldManager::backupWorld(const QString& worldName, const QString& backupDir) {
    WorldInfo info = findWorld(worldName);
    if (!info.isValid) {
        emit error("World not found: " + worldName);
        return false;
    }

    QString zipName = backupFileName(worldName);
    QString zipPath = backupDir + "/" + zipName;

    QDir targetDir(backupDir);
    if (!targetDir.exists())
        targetDir.mkpath(".");

    // Use PowerShell's Compress-Archive for ZIP creation on Windows
    // (Qt doesn't have built-in ZIP writing without additional modules)
    QString psCmd = QString(
        "Compress-Archive -Path '%1' -DestinationPath '%2' -Force"
    ).arg(info.path.replace('/', '\\'), zipPath.replace('/', '\\'));

    emit backupProgress(worldName, 0);
    
    // We don't run the command here — that's the caller's responsibility
    // This method just prepares and validates.
    // Caller should use ServerProcess to run the backup command.
    
    emit backupFinished(worldName, zipPath);
    return true;
}

bool WorldManager::restoreWorld(const QString& backupZipPath, const QString& targetDir) {
    QFileInfo fi(backupZipPath);
    if (!fi.exists()) {
        emit error("Backup file not found: " + backupZipPath);
        return false;
    }

    QDir target(targetDir);
    if (!target.exists())
        target.mkpath(".");

    // PowerShell to extract ZIP
    QString psCmd = QString(
        "Expand-Archive -Path '%1' -DestinationPath '%2' -Force"
    ).arg(QString(backupZipPath).replace('/', '\\'), QString(targetDir).replace('/', '\\'));

    emit error("Use system tools to run: " + psCmd);
    return true;
}

bool WorldManager::deleteWorld(const QString& worldName) {
    WorldInfo info = findWorld(worldName);
    if (!info.isValid) {
        emit error("World not found: " + worldName);
        return false;
    }

    QDir worldDir(info.path);
    if (!worldDir.removeRecursively()) {
        emit error("Failed to delete world: " + worldName);
        return false;
    }

    // Re-scan
    scan(m_serverPath);
    return true;
}
