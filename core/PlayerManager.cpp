#include "PlayerManager.h"
#include <QRegularExpression>

// ─── PlayerInfo serialization ─────────────────────────────────────────

QJsonObject PlayerInfo::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["uuid"] = uuid;
    obj["ip"] = ip;
    obj["x"] = x;
    obj["y"] = y;
    obj["z"] = z;
    obj["world"] = world;
    obj["health"] = health;
    obj["isOp"] = isOp;
    obj["isOnline"] = isOnline;
    obj["joinTime"] = joinTime.toMSecsSinceEpoch();
    return obj;
}

// ─── PlayerManager ────────────────────────────────────────────────────

PlayerManager::PlayerManager(QObject* parent) : QObject(parent) {}

void PlayerManager::parseConsoleOutput(const QString& line) {
    // ── Player joined ──
    // Vanilla: "Herobrine joined the game"
    // Paper/Spigot: "Herobrine[/127.0.0.1:12345] logged in with entity id 123"
    QRegularExpression joinRe(R"((\w+)\s+joined the game)");
    auto joinMatch = joinRe.match(line);
    if (joinMatch.hasMatch()) {
        PlayerInfo p;
        p.name = joinMatch.captured(1);
        p.isOnline = true;
        p.joinTime = QDateTime::currentDateTime();
        addPlayer(p);
        return;
    }

    // Paper/Spigot format: "Herobrine[/127.0.0.1:12345] logged in"
    QRegularExpression joinPaperRe(R"((\w+)\[/([\d\.]+):\d+\]\s+logged in)");
    auto joinPMatch = joinPaperRe.match(line);
    if (joinPMatch.hasMatch()) {
        PlayerInfo p;
        p.name = joinPMatch.captured(1);
        p.ip = joinPMatch.captured(2);
        p.isOnline = true;
        p.joinTime = QDateTime::currentDateTime();
        addPlayer(p);
        return;
    }

    // ── Player left ──
    // Vanilla: "Herobrine left the game"
    QRegularExpression leaveRe(R"((\w+)\s+left the game)");
    auto leaveMatch = leaveRe.match(line);
    if (leaveMatch.hasMatch()) {
        removePlayer(leaveMatch.captured(1));
        return;
    }

    // Paper/Spigot: "Herobrine lost connection: Disconnected"
    QRegularExpression leavePaperRe(R"((\w+)\s+lost connection)");
    auto leavePMatch = leavePaperRe.match(line);
    if (leavePMatch.hasMatch()) {
        removePlayer(leavePMatch.captured(1));
        return;
    }
}

PlayerInfo PlayerManager::playerInfo(const QString& name) const {
    if (m_allPlayers.contains(name))
        return m_allPlayers[name];
    PlayerInfo p;
    p.name = name;
    return p;
}

void PlayerManager::addPlayer(const PlayerInfo& player) {
    // Check if already online
    for (auto& p : m_players) {
        if (p.name == player.name) {
            p.isOnline = true;
            return;
        }
    }
    m_players.append(player);
    m_allPlayers[player.name] = player;
    emit playerJoined(player.name);
    emit onlinePlayersChanged();
}

void PlayerManager::removePlayer(const QString& name) {
    for (int i = 0; i < m_players.size(); i++) {
        if (m_players[i].name == name) {
            m_players.removeAt(i);
            if (m_allPlayers.contains(name)) {
                m_allPlayers[name].isOnline = false;
            }
            emit playerLeft(name);
            emit onlinePlayersChanged();
            return;
        }
    }
}

// ─── Command generators ───────────────────────────────────────────────

QString PlayerManager::kickCommand(const QString& player, const QString& reason) {
    if (reason.isEmpty())
        return QString("kick %1").arg(player);
    return QString("kick %1 %2").arg(player, reason);
}

QString PlayerManager::banCommand(const QString& player, const QString& reason) {
    if (reason.isEmpty())
        return QString("ban %1").arg(player);
    return QString("ban %1 %2").arg(player, reason);
}

QString PlayerManager::pardonCommand(const QString& player) {
    return QString("pardon %1").arg(player);
}

QString PlayerManager::opCommand(const QString& player) {
    return QString("op %1").arg(player);
}

QString PlayerManager::deopCommand(const QString& player) {
    return QString("deop %1").arg(player);
}

QString PlayerManager::gamemodeCommand(const QString& player, const QString& mode) {
    return QString("gamemode %1 %2").arg(mode, player);
}

QString PlayerManager::whitelistAddCommand(const QString& player) {
    return QString("whitelist add %1").arg(player);
}

QString PlayerManager::whitelistRemoveCommand(const QString& player) {
    return QString("whitelist remove %1").arg(player);
}
