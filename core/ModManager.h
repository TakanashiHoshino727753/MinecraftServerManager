#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>

// ─── Mod Info ─────────────────────────────────────────────────────────

struct ModInfo {
    QString id;           // mod filename or unique ID
    QString name;         // display name
    QString version;      // mod version
    QString description;  // short description
    QString fileName;     // actual .jar filename
    bool    enabled = true;
    QStringList dependencies;    // required mod IDs
    QStringList conflicts;       // conflicting mod IDs
    QString loader;       // "forge", "fabric", "neoforge", "quilt", "any"

    QJsonObject toJson() const;
    static ModInfo fromJson(const QJsonObject& obj);
};

// ─── Mod Manager ──────────────────────────────────────────────────────

class ModManager : public QObject {
    Q_OBJECT
public:
    explicit ModManager(QObject* parent = nullptr);

    // Scan server mods folder
    void scan(const QString& serverPath, const QString& loader);
    QList<ModInfo> mods() const { return m_mods; }

    // Install a mod (.jar file) to server
    bool installMod(const QString& jarPath);
    // Remove a mod
    bool removeMod(const QString& fileName);
    // Enable/disable a mod (rename .jar → .jar.disabled)
    bool setModEnabled(const QString& fileName, bool enabled);

    // Check for conflicts among installed mods
    QList<QPair<ModInfo,ModInfo>> checkConflicts() const;

    // Simulated "market" — predefined popular mods
    struct MarketMod {
        QString id;
        QString name;
        QString description;
        QString loader;       // "forge", "fabric", "both"
        QStringList mcVersions;
        QString downloadUrl;  // placeholder
    };
    static QList<MarketMod> marketMods();

    // Settings path for storing enabled states
    QString settingsPath() const;

signals:
    void modsChanged();
    void conflictDetected(const QString& modA, const QString& modB);
    void error(const QString& message);

private:
    void saveSettings();
    void loadSettings();
    ModInfo parseModJar(const QString& jarPath) const;

    QString m_serverPath;
    QString m_loader;
    QList<ModInfo> m_mods;
    QMap<QString, bool> m_enabledStates;  // fileName → enabled
};
