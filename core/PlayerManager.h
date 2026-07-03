#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

// ─── Player Info ──────────────────────────────────────────────────────

struct PlayerInfo {
    QString name;
    QString uuid;
    QString ip;
    double  x = 0, y = 0, z = 0;
    QString world;     // world name/dimension
    double  health = 20;
    bool    isOp = false;
    bool    isOnline = false;
    QDateTime joinTime;

    QJsonObject toJson() const;
};

// ─── Player Manager ───────────────────────────────────────────────────

class PlayerManager : public QObject {
    Q_OBJECT
public:
    explicit PlayerManager(QObject* parent = nullptr);

    // Parse console output to detect player join/leave
    void parseConsoleOutput(const QString& line);

    // Get current online players
    QList<PlayerInfo> onlinePlayers() const { return m_players; }

    // Player history (all players ever seen)
    QStringList allPlayerNames() const { return m_allPlayers.keys(); }
    PlayerInfo playerInfo(const QString& name) const;

    // Generate Minecraft commands
    static QString kickCommand(const QString& player, const QString& reason = "");
    static QString banCommand(const QString& player, const QString& reason = "");
    static QString pardonCommand(const QString& player);
    static QString opCommand(const QString& player);
    static QString deopCommand(const QString& player);
    static QString gamemodeCommand(const QString& player, const QString& mode);
    static QString whitelistAddCommand(const QString& player);
    static QString whitelistRemoveCommand(const QString& player);

signals:
    void playerJoined(const QString& name);
    void playerLeft(const QString& name);
    void onlinePlayersChanged();

private:
    void addPlayer(const PlayerInfo& player);
    void removePlayer(const QString& name);

    QList<PlayerInfo> m_players;               // currently online
    QMap<QString, PlayerInfo> m_allPlayers;     // all known players (history)
};
