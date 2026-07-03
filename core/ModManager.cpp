#include "ModManager.h"
#include <QJsonDocument>
#include <QFile>
#include <QDirIterator>
#include <QRegularExpression>

// ─── ModInfo serialization ────────────────────────────────────────────

QJsonObject ModInfo::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["version"] = version;
    obj["description"] = description;
    obj["fileName"] = fileName;
    obj["enabled"] = enabled;
    QJsonArray deps;
    for (const auto& d : dependencies) deps.append(d);
    obj["dependencies"] = deps;
    QJsonArray confs;
    for (const auto& c : conflicts) confs.append(c);
    obj["conflicts"] = confs;
    obj["loader"] = loader;
    return obj;
}

ModInfo ModInfo::fromJson(const QJsonObject& obj) {
    ModInfo m;
    m.id = obj["id"].toString();
    m.name = obj["name"].toString();
    m.version = obj["version"].toString();
    m.description = obj["description"].toString();
    m.fileName = obj["fileName"].toString();
    m.enabled = obj["enabled"].toBool(true);
    for (const auto& v : obj["dependencies"].toArray())
        m.dependencies.append(v.toString());
    for (const auto& v : obj["conflicts"].toArray())
        m.conflicts.append(v.toString());
    m.loader = obj["loader"].toString();
    return m;
}

// ─── ModManager ───────────────────────────────────────────────────────

ModManager::ModManager(QObject* parent) : QObject(parent) {}

void ModManager::scan(const QString& serverPath, const QString& loader) {
    m_serverPath = serverPath;
    m_loader = loader;
    m_mods.clear();

    loadSettings();

    QString modsDir = serverPath + "/mods";
    QDir dir(modsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
        return;
    }

    QDirIterator it(modsDir, {"*.jar"}, QDir::Files);
    while (it.hasNext()) {
        it.next();
        ModInfo mod = parseModJar(it.filePath());
        mod.fileName = it.fileName();
        // Restore enabled state
        if (m_enabledStates.contains(mod.fileName))
            mod.enabled = m_enabledStates[mod.fileName];
        else
            mod.enabled = true;
        m_mods.append(mod);
    }

    emit modsChanged();
}

ModInfo ModManager::parseModJar(const QString& jarPath) const {
    ModInfo mod;
    QFileInfo fi(jarPath);
    mod.fileName = fi.fileName();
    mod.name = fi.baseName();
    mod.enabled = true;

    // For now, extract info from filename pattern:
    // e.g. "jei-1.20.1-forge-15.2.0.jar" or "fabric-api-0.92.0+1.20.1.jar"
    QString base = fi.baseName();
    
    // Try to detect loader from filename
    QString lower = base.toLower();
    if (lower.contains("forge") || lower.contains("neoforge"))
        mod.loader = "forge";
    else if (lower.contains("fabric"))
        mod.loader = "fabric";
    else if (lower.contains("quilt"))
        mod.loader = "quilt";
    else
        mod.loader = "any";

    // Try to extract version (e.g. "1.20.1" pattern)
    QRegularExpression verRe(R"((\d+\.\d+(?:\.\d+)?))");
    auto match = verRe.match(base);
    if (match.hasMatch())
        mod.version = match.captured(1);

    mod.id = mod.fileName;
    return mod;
}

bool ModManager::installMod(const QString& jarPath) {
    QFileInfo fi(jarPath);
    if (!fi.exists() || fi.suffix().toLower() != "jar") {
        emit error("Invalid mod file: " + jarPath);
        return false;
    }
    QString targetDir = m_serverPath + "/mods";
    QDir().mkpath(targetDir);
    QString target = targetDir + "/" + fi.fileName();
    if (QFile::exists(target)) {
        emit error("Mod already installed: " + fi.fileName());
        return false;
    }
    if (!QFile::copy(jarPath, target)) {
        emit error("Failed to copy mod file: " + jarPath);
        return false;
    }
    // Re-scan
    scan(m_serverPath, m_loader);
    
    // Check conflicts
    auto conflicts = checkConflicts();
    for (const auto& pair : conflicts)
        emit conflictDetected(pair.first.name, pair.second.name);
    
    return true;
}

bool ModManager::removeMod(const QString& fileName) {
    QString target = m_serverPath + "/mods/" + fileName;
    if (!QFile::remove(target)) {
        emit error("Failed to remove mod: " + fileName);
        return false;
    }
    m_enabledStates.remove(fileName);
    saveSettings();
    scan(m_serverPath, m_loader);
    return true;
}

bool ModManager::setModEnabled(const QString& fileName, bool enabled) {
    m_enabledStates[fileName] = enabled;
    saveSettings();
    for (auto& mod : m_mods) {
        if (mod.fileName == fileName) {
            mod.enabled = enabled;
            break;
        }
    }
    emit modsChanged();
    return true;
}

QList<QPair<ModInfo,ModInfo>> ModManager::checkConflicts() const {
    QList<QPair<ModInfo,ModInfo>> result;
    for (int i = 0; i < m_mods.size(); i++) {
        for (int j = i + 1; j < m_mods.size(); j++) {
            const ModInfo& a = m_mods[i];
            const ModInfo& b = m_mods[j];
            if (!a.enabled || !b.enabled) continue;
            // Check if a conflicts with b or vice versa
            if (a.conflicts.contains(b.id) || b.conflicts.contains(a.id)) {
                result.append({a, b});
                continue;
            }
            // Loader mismatch check
            if (a.loader != "any" && b.loader != "any" && a.loader != b.loader) {
                result.append({a, b});
            }
        }
    }
    return result;
}

QString ModManager::settingsPath() const {
    return m_serverPath + "/mods/mod-manager.json";
}

void ModManager::saveSettings() {
    QJsonObject obj;
    QJsonObject states;
    for (auto it = m_enabledStates.begin(); it != m_enabledStates.end(); ++it)
        states[it.key()] = it.value();
    obj["enabledStates"] = states;
    QDir().mkpath(m_serverPath + "/mods");
    QFile f(settingsPath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson());
        f.close();
    }
}

void ModManager::loadSettings() {
    QFile f(settingsPath());
    if (f.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
        QJsonObject states = obj["enabledStates"].toObject();
        for (auto it = states.begin(); it != states.end(); ++it)
            m_enabledStates[it.key()] = it.value().toBool();
        f.close();
    }
}

// ─── Market ───────────────────────────────────────────────────────────

QList<ModManager::MarketMod> ModManager::marketMods() {
    return {
        {"jei", "Just Enough Items (JEI)", "Item and recipe viewer", "both", {"1.20.1","1.21","1.20.4"}, ""},
        {"fabric-api", "Fabric API", "Core library for Fabric mods", "fabric", {"1.20.1","1.21","1.20.4"}, ""},
        {"sodium", "Sodium", "Performance optimization mod", "fabric", {"1.20.1","1.21","1.20.4"}, ""},
        {"optifine", "OptiFine", "Performance and shader support", "forge", {"1.20.1","1.21","1.20.4"}, ""},
        {"journeymap", "JourneyMap", "Real-time mapping mod", "both", {"1.20.1","1.21","1.20.4"}, ""},
        {"lithium", "Lithium", "Game logic optimization", "fabric", {"1.20.1","1.21","1.20.4"}, ""},
        {"create", "Create", "Mechanical engineering mod", "forge", {"1.20.1"}, ""},
        {"xaeros-minimap", "Xaero's Minimap", "Minimap HUD", "both", {"1.20.1","1.21","1.20.4"}, ""},
        {"phosphor", "Phosphor", "Lighting engine optimization", "fabric", {"1.20.1","1.20.4"}, ""},
        {"rei", "Roughly Enough Items (REI)", "Item and recipe viewer", "both", {"1.20.1","1.21","1.20.4"}, ""},
        {"modmenu", "Mod Menu", "In-game mod list", "fabric", {"1.20.1","1.21","1.20.4"}, ""},
        {"dynmap", "Dynmap", "Real-time web map", "forge", {"1.20.1","1.21","1.20.4"}, ""},
    };
}
