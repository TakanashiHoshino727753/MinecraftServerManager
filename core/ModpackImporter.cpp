#include "ModpackImporter.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QUuid>

ModpackImporter::ModpackImporter(QObject* parent) : QObject(parent) {}

// ─── Public API ───────────────────────────────────────────────────────

ModpackImporter::ModpackInfo ModpackImporter::analyze(const QString& path) {
    QFileInfo fi(path);
    if (!fi.exists()) {
        emit error("Path does not exist: " + path);
        return {};
    }
    if (fi.isDir())
        return detectFromDirectory(path);
    else if (fi.suffix().toLower() == "zip")
        return detectFromZip(path);
    else {
        emit error("Unsupported format: " + fi.suffix());
        return {};
    }
}

ServerConfig ModpackImporter::generateConfig(const ModpackInfo& info, const QString& targetDir) {
    ServerConfig cfg;
    cfg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    cfg.name = info.name;
    cfg.path = targetDir;
    cfg.version = info.mcVersion;
    
    if (info.loader == "forge")
        cfg.type = "forge";
    else if (info.loader == "fabric" || info.loader == "quilt")
        cfg.type = "fabric";
    else
        cfg.type = "vanilla";

    return cfg;
}

bool ModpackImporter::install(const ModpackInfo& info, const QString& targetDir) {
    QDir(targetDir).mkpath(".");
    QDir(targetDir).mkpath("mods");
    QDir(targetDir).mkpath("config");

    // Copy mod files
    for (const auto& mod : info.modFiles) {
        QString dest = targetDir + "/mods/" + QFileInfo(mod).fileName();
        emit progress("Copying: " + QFileInfo(mod).fileName());
        QFile::copy(mod, dest);
    }

    // Write a modpack manifest
    QJsonObject manifest;
    manifest["name"] = info.name;
    manifest["mcVersion"] = info.mcVersion;
    manifest["loader"] = info.loader;
    manifest["loaderVersion"] = info.loaderVersion;
    manifest["description"] = info.description;

    QFile manifestFile(targetDir + "/modpack-manifest.json");
    if (manifestFile.open(QIODevice::WriteOnly)) {
        manifestFile.write(QJsonDocument(manifest).toJson());
        manifestFile.close();
    }

    emit finished(info);
    return true;
}

// ─── Detection ────────────────────────────────────────────────────────

ModpackImporter::ModpackInfo ModpackImporter::detectFromDirectory(const QString& dirPath) {
    ModpackInfo info;
    QDir dir(dirPath);
    info.name = dir.dirName();

    // 1. Check for fabric-loader / forge installer files
    QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    // Detect loader from version-specific folders or manifest files
    bool hasFabric = false, hasForge = false;
    
    for (const auto& entry : entries) {
        QString lower = entry.toLower();
        if (lower.contains("fabric"))
            hasFabric = true;
        if (lower.contains("forge") || lower.contains("neoforge"))
            hasForge = true;
    }

    // 2. Scan mods folder
    QString modsDir = dirPath + "/mods";
    if (QDir(modsDir).exists()) {
        QDirIterator it(modsDir, {"*.jar"}, QDir::Files);
        while (it.hasNext()) {
            it.next();
            info.modFiles.append(it.filePath());
            
            // Detect loader from mod filenames
            QString lower = QFileInfo(it.fileName()).baseName().toLower();
            if (lower.contains("fabric"))
                hasFabric = true;
            if (lower.contains("forge") || lower.contains("neoforge"))
                hasForge = true;
        }
    }

    // 3. Check manifest files
    // Fabric: fabric-installer.json or fabric-loader-version
    if (QFile::exists(dirPath + "/fabric-installer.json")) {
        hasFabric = true;
    }
    
    // Forge: installer.jar, forge-version, or libraries/forge
    if (QDir(dirPath + "/libraries/net/minecraftforge").exists())
        hasForge = true;

    // NCMS (NeoForge CurseForge Modpack Sync)
    if (QFile::exists(dirPath + "/manifest.json")) {
        QFile f(dirPath + "/manifest.json");
        if (f.open(QIODevice::ReadOnly)) {
            QJsonObject manifest = QJsonDocument::fromJson(f.readAll()).object();
            info.name = manifest["name"].toString(info.name);
            info.mcVersion = manifest["minecraft"].toObject()["version"].toString();
            info.description = manifest["name"].toString();
            f.close();
        }
    }

    // 4. Determine loader
    if (hasFabric && hasForge)
        info.loader = "forge";  // forge packs sometimes include fabric libs
    else if (hasFabric)
        info.loader = "fabric";
    else if (hasForge)
        info.loader = "forge";
    else
        info.loader = "vanilla";

    // 5. Try to extract MC version from filenames
    for (const auto& mod : info.modFiles) {
        QString ver = extractVersion(QFileInfo(mod).fileName());
        if (!ver.isEmpty() && ver.count('.') >= 1) {
            info.mcVersion = ver;
            break;
        }
    }

    info.valid = !info.modFiles.isEmpty() || hasFabric || hasForge;
    return info;
}

ModpackImporter::ModpackInfo ModpackImporter::detectFromZip(const QString& zipPath) {
    // For ZIP analysis, we'd need QZipReader or external zip tool
    // For now, provide a basic detection from the filename
    ModpackInfo info;
    QFileInfo fi(zipPath);
    info.name = fi.baseName();
    
    QString lower = info.name.toLower();
    if (lower.contains("fabric"))
        info.loader = "fabric";
    else if (lower.contains("forge") || lower.contains("neoforge"))
        info.loader = "forge";
    else
        info.loader = "vanilla";

    info.mcVersion = extractVersion(info.name);
    info.valid = true;

    emit progress("ZIP analysis is limited. For full detection, please extract the modpack first.");
    emit finished(info);
    return info;
}

QString ModpackImporter::extractVersion(const QString& text) {
    // Match Minecraft version patterns: 1.20.1, 1.21, 1.20.4, etc.
    QRegularExpression re(R"((1\.\d+(?:\.\d+)?))");
    auto match = re.match(text);
    return match.hasMatch() ? match.captured(1) : QString();
}
