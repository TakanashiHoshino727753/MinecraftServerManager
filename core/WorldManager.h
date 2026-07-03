#pragma once
#include <QObject>
#include <QString>
#include <QDir>
#include <QDateTime>
#include <QJsonObject>

// ─── World Info ────────────────────────────────────────────────────────

struct WorldInfo {
    QString name;          // world folder name (e.g. "world", "world_nether")
    QString displayName;   // user-friendly name
    QString path;          // full path to world folder
    QString dimension;     // "overworld", "nether", "end"
    qint64  seed = 0;      // world seed
    QString gameType;      // "survival", "creative", "adventure", "spectator"
    qint64  sizeBytes = 0; // world folder size
    QDateTime lastPlayed;
    bool    isValid = false;

    QJsonObject toJson() const;
    static WorldInfo fromJson(const QJsonObject& obj);
};

// ─── World Manager ─────────────────────────────────────────────────────

class WorldManager : public QObject {
    Q_OBJECT
public:
    explicit WorldManager(QObject* parent = nullptr);

    // Scan server directory for world folders
    void scan(const QString& serverPath);
    QList<WorldInfo> worlds() const { return m_worlds; }
    WorldInfo findWorld(const QString& name) const;

    // World operations
    bool backupWorld(const QString& worldName, const QString& backupDir);
    bool restoreWorld(const QString& backupZipPath, const QString& targetDir);
    bool deleteWorld(const QString& worldName);

    // Parse seed from server.properties or world level.dat
    qint64 readSeed(const QString& serverPath, const QString& worldName) const;
    qint64 folderSize(const QString& path) const;

    // Generate the backup filename
    static QString backupFileName(const QString& worldName);

signals:
    void worldsChanged();
    void backupProgress(const QString& worldName, int percent);
    void backupFinished(const QString& worldName, const QString& backupPath);
    void error(const QString& message);

private:
    QString m_serverPath;
    QList<WorldInfo> m_worlds;

    void scanDimension(const QDir& worldDir, const QString& dimName, const QString& dimFolder);
};
