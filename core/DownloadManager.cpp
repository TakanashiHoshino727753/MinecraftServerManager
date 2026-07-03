#include "DownloadManager.h"
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QTextStream>

const char* DownloadManager::BMCLAPI    = "https://bmclapi2.bangbang93.com";
const char* DownloadManager::FABRIC_META = "https://meta.fabricmc.net";
const char* DownloadManager::MAVEN_CENTRAL = "https://maven.fabricmc.net";
const char* DownloadManager::FABRIC_MAVEN = "https://maven.fabricmc.net";

DownloadManager::DownloadManager(QObject* parent) : QObject(parent) {
    m_nam = new QNetworkAccessManager(this);
}

QNetworkReply* DownloadManager::httpGet(const QUrl& url) {
    QNetworkRequest req(url);
    req.setTransferTimeout(15000);
    req.setRawHeader("User-Agent", "MCServerManager/1.0");
    return m_nam->get(req);
}

bool DownloadManager::downloadFile(const QUrl& url, const QString& filepath) {
    QNetworkRequest req(url);
    req.setTransferTimeout(300000); // 5 minutes
    req.setRawHeader("User-Agent", "MCServerManager/1.0");

    QNetworkReply* reply = m_nam->get(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, [&]() { reply->abort(); loop.quit(); });
    timer.start(300000);

    // Progress reporting
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
        if (total > 0) {
            int pct = static_cast<int>(received * 100 / total);
            emit downloadProgress(QString("下载中 %1/%2 MB")
                .arg(received / 1048576.0, 0, 'f', 1)
                .arg(total / 1048576.0, 0, 'f', 1), pct);
        }
    });

    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        emit error("Download failed: " + reply->errorString());
        reply->deleteLater();
        return false;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Cannot write file: " + filepath);
        reply->deleteLater();
        return false;
    }
    file.write(reply->readAll());
    file.close();
    reply->deleteLater();
    return true;
}

void DownloadManager::fetchVersions() {
    QNetworkReply* reply = httpGet(QUrl(QString(BMCLAPI) + "/mc/game/version_manifest.json"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit error("Failed to fetch versions");
            reply->deleteLater();
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray versions = doc["versions"].toArray();
        QStringList list;
        for (const auto& v : versions) {
            QJsonObject vo = v.toObject();
            // Only include release versions, skip snapshots and betas
            QString type = vo["type"].toString();
            if (type != "release") continue;
            list << vo["id"].toString();
        }
        emit versionsReady(list);
        reply->deleteLater();
    });
}

void DownloadManager::fetchPaperBuilds(const QString& version) {
    // BMCLAPI mirrors PaperMC API at /paper/api/v2/... (NOT /maven/paper/...)
    QNetworkReply* reply = httpGet(
        QUrl(QString(BMCLAPI) + "/paper/api/v2/projects/paper/versions/" + version));
    connect(reply, &QNetworkReply::finished, this, [this, reply, version]() {
        QStringList list;
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            for (const auto& b : doc["builds"].toArray()) {
                list << QString::number(b.toInt());
            }
        } else {
            // Fallback to official PaperMC API
            QNetworkReply* fbReply = httpGet(
                QUrl(QString("https://api.papermc.io/v2/projects/paper/versions/") + version));
            QEventLoop fbLoop;
            QTimer fbTimer;
            fbTimer.setSingleShot(true);
            connect(fbReply, &QNetworkReply::finished, &fbLoop, &QEventLoop::quit);
            connect(&fbTimer, &QTimer::timeout, [&]() { fbReply->abort(); fbLoop.quit(); });
            fbTimer.start(10000);
            fbLoop.exec();
            if (fbReply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(fbReply->readAll());
                for (const auto& b : doc["builds"].toArray()) {
                    list << QString::number(b.toInt());
                }
            }
            fbReply->deleteLater();
        }
        emit buildsReady(list);
        reply->deleteLater();
    });
}

void DownloadManager::downloadVanilla(const QString& version, const QString& dir) {
    emit downloadProgress("获取版本信息...", 0);

    // Get version manifest for jar URL
    QNetworkReply* reply = httpGet(
        QUrl(QString(BMCLAPI) + "/mc/game/version_manifest.json"));
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        emit downloadFinished(false, "Failed to get version manifest");
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray versions = doc["versions"].toArray();
    QString versionUrl;
    for (const auto& v : versions) {
        QJsonObject vo = v.toObject();
        if (vo["id"].toString() == version) {
            versionUrl = vo["url"].toString();
            break;
        }
    }
    reply->deleteLater();

    if (versionUrl.isEmpty()) {
        emit downloadFinished(false, "Version not found: " + version);
        return;
    }

    // Convert to BMCLAPI mirror
    versionUrl.replace("launchermeta.mojang.com", "bmclapi2.bangbang93.com");
    versionUrl.replace("piston-meta.mojang.com", "bmclapi2.bangbang93.com");

    // Get version JSON for server jar URL
    QNetworkReply* vReply = httpGet(QUrl(versionUrl));
    QEventLoop vLoop;
    connect(vReply, &QNetworkReply::finished, &vLoop, &QEventLoop::quit);
    vLoop.exec();

    if (vReply->error() != QNetworkReply::NoError) {
        emit downloadFinished(false, "Failed to get version info");
        vReply->deleteLater();
        return;
    }

    QJsonObject vObj = QJsonDocument::fromJson(vReply->readAll()).object();
    QString jarUrl = vObj["downloads"].toObject()["server"].toObject()["url"].toString();
    vReply->deleteLater();

    // Mirror the jar URL
    jarUrl.replace("launcher.mojang.com", "bmclapi2.bangbang93.com");

    QDir().mkpath(dir);
    emit downloadProgress("下载服务端核心...", 10);
    bool ok = downloadFile(QUrl(jarUrl), dir + "/server.jar");
    emit downloadFinished(ok, ok ? "Vanilla server downloaded" : "Download failed");
}

bool DownloadManager::downloadVanillaSync(const QString& version, const QString& dir) {
    // Get version manifest for jar URL (synchronous, returns bool, no signals)
    QNetworkReply* reply = httpGet(
        QUrl(QString(BMCLAPI) + "/mc/game/version_manifest.json"));
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray versions = doc["versions"].toArray();
    QString versionUrl;
    for (const auto& v : versions) {
        QJsonObject vo = v.toObject();
        if (vo["id"].toString() == version) {
            versionUrl = vo["url"].toString();
            break;
        }
    }
    reply->deleteLater();

    if (versionUrl.isEmpty()) return false;

    // Convert to BMCLAPI mirror
    versionUrl.replace("launchermeta.mojang.com", "bmclapi2.bangbang93.com");
    versionUrl.replace("piston-meta.mojang.com", "bmclapi2.bangbang93.com");

    QNetworkReply* vReply = httpGet(QUrl(versionUrl));
    QEventLoop vLoop;
    connect(vReply, &QNetworkReply::finished, &vLoop, &QEventLoop::quit);
    vLoop.exec();

    if (vReply->error() != QNetworkReply::NoError) {
        vReply->deleteLater();
        return false;
    }

    QJsonObject vObj = QJsonDocument::fromJson(vReply->readAll()).object();
    QString jarUrl = vObj["downloads"].toObject()["server"].toObject()["url"].toString();
    vReply->deleteLater();

    jarUrl.replace("launcher.mojang.com", "bmclapi2.bangbang93.com");

    QDir().mkpath(dir);
    return downloadFile(QUrl(jarUrl), dir + "/server.jar");
}

void DownloadManager::downloadPaper(const QString& version, const QString& build,
                                     const QString& dir) {
    // BMCLAPI PaperMC mirror path
    QString url = QString(BMCLAPI) + "/paper/api/v2/projects/paper/versions/"
                + version + "/builds/" + build
                + "/downloads/paper-" + version + "-" + build + ".jar";

    emit downloadProgress("下载 Paper 服务端...", 10);
    QDir().mkpath(dir);
    bool ok = downloadFile(QUrl(url), dir + "/server.jar");
    if (!ok) {
        // Fallback to official Paper API
        QString fallback = "https://api.papermc.io/v2/projects/paper/versions/"
                         + version + "/builds/" + build
                         + "/downloads/paper-" + version + "-" + build + ".jar";
        ok = downloadFile(QUrl(fallback), dir + "/server.jar");
    }
    emit downloadFinished(ok, ok ? "Paper server downloaded" : "Download failed");
}

// Helper: fetch loader version for a given MC version from a base URL
// Returns empty string on failure
static QString tryFetchFabricLoader(const QString& baseUrl, const QString& mcVersion,
                                     QNetworkAccessManager* nam) {
    QNetworkRequest req(QUrl(baseUrl + "/v2/versions/loader/" + mcVersion));
    req.setTransferTimeout(15000);
    req.setRawHeader("User-Agent", "MCServerManager/1.0");
    QNetworkReply* reply = nam->get(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, [&]() { reply->abort(); loop.quit(); });
    timer.start(15000);
    loop.exec();

    QString loaderVer;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonArray loaders = QJsonDocument::fromJson(reply->readAll()).array();

        // Helper: extract version & stable flag from one loader entry
        auto extractLoader = [&](const QJsonObject& lo) -> QString {
            // Fabric API format: { "loader": { "version": "0.16.9", "stable": true }, ... }
            QJsonObject inner = lo["loader"].toObject();
            if (!inner.isEmpty() && inner.contains("version"))
                return inner["version"].toString();
            // Simplified mirror format: { "version": "0.16.9", "stable": true }
            if (lo.contains("version"))
                return lo["version"].toString();
            return {};
        };
        auto isLoaderStable = [&](const QJsonObject& lo) -> bool {
            QJsonObject inner = lo["loader"].toObject();
            if (!inner.isEmpty() && inner.contains("stable"))
                return inner["stable"].toBool();
            return lo["stable"].toBool();
        };

        // Prefer stable loader
        for (const auto& l : loaders) {
            QJsonObject lo = l.toObject();
            QString v = extractLoader(lo);
            if (v.isEmpty()) continue;
            if (isLoaderStable(lo)) {
                loaderVer = v;
            }
        }
        // Fallback: any loader
        if (loaderVer.isEmpty()) {
            for (const auto& l : loaders) {
                QJsonObject lo = l.toObject();
                QString v = extractLoader(lo);
                if (!v.isEmpty()) {
                    loaderVer = v;
                }
            }
        }
    }
    reply->deleteLater();
    return loaderVer;
}

// Helper: fetch Fabric-supported game versions from a base URL
static QStringList fetchFabricGameVersions(const QString& baseUrl,
                                            QNetworkAccessManager* nam) {
    QNetworkRequest req(QUrl(baseUrl + "/v2/versions/game"));
    req.setTransferTimeout(10000);
    req.setRawHeader("User-Agent", "MCServerManager/1.0");
    QNetworkReply* reply = nam->get(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, [&]() { reply->abort(); loop.quit(); });
    timer.start(10000);
    loop.exec();

    QStringList vers;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
        for (const auto& v : arr) {
            QJsonObject vo = v.toObject();
            if (vo["stable"].toBool()) {
                vers << vo["version"].toString();
            }
        }
        // Also include non-stable as fallback
        for (const auto& v : arr) {
            QJsonObject vo = v.toObject();
            if (!vo["stable"].toBool()) {
                vers << vo["version"].toString();
            }
        }
    }
    reply->deleteLater();
    return vers;
}

void DownloadManager::downloadFabric(const QString& version, const QString& dir) {
    emit downloadProgress("获取 Fabric 加载器信息...", 3);

    // PCL2-compatible approach: BMCLAPI first, official Fabric CDN as fallback
    // Mirrors: bmclapi2.bangbang93.com/fabric-meta, meta.fabricmc.net
    QString effectiveVersion = version;
    QString loaderVersion;

    // 1. Try exact version on BMCLAPI first (works better in China), then official Fabric
    loaderVersion = tryFetchFabricLoader(
        QString(BMCLAPI) + "/fabric-meta", effectiveVersion, m_nam);
    if (loaderVersion.isEmpty()) {
        loaderVersion = tryFetchFabricLoader(FABRIC_META, effectiveVersion, m_nam);
    }

    // 2. If not found, resolve version suffix: "1.21" -> "1.21.x" latest
    if (loaderVersion.isEmpty()) {
        // Try BMCLAPI first for game versions, then official Fabric
        QStringList gameVers = fetchFabricGameVersions(
            QString(BMCLAPI) + "/fabric-meta", m_nam);
        if (gameVers.isEmpty()) {
            gameVers = fetchFabricGameVersions(FABRIC_META, m_nam);
        }

        // Build candidate list: matching versions sorted high → low, release first
        auto isRelease = [](const QString& v) { return !v.contains('-'); };
        QStringList candidates;
        QString prefix = version + "."; // "1.21." to match "1.21.4", "1.21.5"
        for (const QString& gv : gameVers) {
            if (gv.startsWith(prefix) && isRelease(gv))
                candidates << gv;
        }
        // Sort release candidates descending (1.21.5 > 1.21.4)
        std::sort(candidates.begin(), candidates.end(),
                  [](const QString& a, const QString& b) { return a > b; });

        // Pre-release candidates as fallback
        QStringList preCandidates;
        for (const QString& gv : gameVers) {
            if (gv.startsWith(prefix) && !isRelease(gv))
                preCandidates << gv;
        }
        std::sort(preCandidates.begin(), preCandidates.end(),
                  [](const QString& a, const QString& b) { return a > b; });
        candidates.append(preCandidates);

        if (!candidates.isEmpty()) {
            // Walk candidates high→low, pick the first one that actually has a loader
            for (const QString& cand : candidates) {
                QString lv = tryFetchFabricLoader(
                    QString(BMCLAPI) + "/fabric-meta", cand, m_nam);
                if (lv.isEmpty())
                    lv = tryFetchFabricLoader(FABRIC_META, cand, m_nam);
                if (!lv.isEmpty()) {
                    effectiveVersion = cand;
                    loaderVersion = lv;
                    break;
                }
            }

            if (loaderVersion.isEmpty()) {
                // None of the suffix-matched versions have a loader
                emit downloadFinished(false,
                    QString("Fabric 尚未支持 Minecraft %1 系列，"
                            "请在下载源中查支持的最高版本，"
                            "然后手动输入完整版本号（如 %2.4）")
                        .arg(version, version));
                return;
            }

            if (effectiveVersion != version) {
                emit downloadProgress(
                    QString("版本 %1 自动匹配至 %2 ...")
                        .arg(version, effectiveVersion), 5);
            }
        }
    }

    if (loaderVersion.isEmpty()) {
        emit downloadFinished(false,
            QString("未找到适用于 Minecraft %1 的 Fabric 加载器，请手动指定精确版本（如 1.21.4）")
                .arg(effectiveVersion));
        return;
    }

    emit downloadProgress(
        QString("找到 Fabric Loader %1，准备手动安装（绕过 Installer）").arg(loaderVersion), 8);

    // 3. Download vanilla server — try Mojang CDN first, then BMCLAPI
    emit downloadProgress("下载 Minecraft 原版服务端...", 10);
    QString vanillaJar = dir + "/minecraft_server." + effectiveVersion + ".jar";

    auto downloadAndVerify = [&](const QString& url, const QString& path) -> bool {
        if (!downloadFile(QUrl(url), path)) return false;
        // Verify JAR magic bytes (ZIP header = "PK")
        QFile jf(path);
        if (jf.open(QIODevice::ReadOnly)) {
            QByteArray head = jf.read(4);
            if (head.length() == 4 && head[0] == 'P' && head[1] == 'K') return true;
        }
        QFile::remove(path);
        return false;
    };

    // Try Mojang version manifest → get official download URL
    bool vanillaOk = false;
    {
        // Get version manifest from BMCLAPI
        QString vManifestUrl = QString("%1/mc/game/version_manifest.json").arg(BMCLAPI);
        QNetworkReply* vmReply = httpGet(QUrl(vManifestUrl));
        QEventLoop vmLoop;
        QTimer vmTimer;
        vmTimer.setSingleShot(true);
        connect(vmReply, &QNetworkReply::finished, &vmLoop, &QEventLoop::quit);
        connect(&vmTimer, &QTimer::timeout, [&]() { vmReply->abort(); vmLoop.quit(); });
        vmTimer.start(15000);
        vmLoop.exec();

        if (vmReply->error() == QNetworkReply::NoError) {
            QJsonDocument vmd = QJsonDocument::fromJson(vmReply->readAll());
            QJsonArray versions = vmd.object()["versions"].toArray();
            for (const auto& vv : versions) {
                QJsonObject vo = vv.toObject();
                if (vo["id"].toString() == effectiveVersion) {
                    QString vJsonUrl = vo["url"].toString();
                    vmReply->deleteLater();

                    // Get specific version JSON
                    QNetworkReply* vjReply = httpGet(QUrl(vJsonUrl));
                    QEventLoop vjLoop;
                    QTimer vjTimer;
                    vjTimer.setSingleShot(true);
                    connect(vjReply, &QNetworkReply::finished, &vjLoop, &QEventLoop::quit);
                    connect(&vjTimer, &QTimer::timeout, [&]() { vjReply->abort(); vjLoop.quit(); });
                    vjTimer.start(15000);
                    vjLoop.exec();

                    if (vjReply->error() == QNetworkReply::NoError) {
                        QJsonObject vjo = QJsonDocument::fromJson(vjReply->readAll()).object();
                        QString serverUrl = vjo["downloads"].toObject()
                                            ["server"].toObject()["url"].toString();
                        if (!serverUrl.isEmpty()) {
                            vanillaOk = downloadAndVerify(serverUrl, vanillaJar);
                        }
                    }
                    vjReply->deleteLater();
                    break;
                }
            }
        }
        if (!vanillaOk) vmReply->deleteLater();
    }

    // Fallback: BMCLAPI server download
    if (!vanillaOk) {
        QString mirrorUrl = QString("https://bmclapi2.bangbang93.com/version/%1/server")
                            .arg(effectiveVersion);
        vanillaOk = downloadAndVerify(mirrorUrl, vanillaJar);
    }

    if (!vanillaOk) {
        emit downloadFinished(false,
            QString("Minecraft %1 服务端下载失败，请检查网络连接").arg(effectiveVersion));
        return;
    }

    // 4. Fetch Fabric loader profile JSON from BMCLAPI
    emit downloadProgress("获取 Fabric 加载器配置...", 15);
    QJsonObject profile;
    {
        QString profileUrl = QString("%1/fabric-meta/v2/versions/loader/%2/%3/profile/json")
            .arg(BMCLAPI, effectiveVersion, loaderVersion);
        QNetworkReply* profReply = httpGet(QUrl(profileUrl));
        QEventLoop profLoop;
        QTimer profTimer;
        profTimer.setSingleShot(true);
        connect(profReply, &QNetworkReply::finished, &profLoop, &QEventLoop::quit);
        connect(&profTimer, &QTimer::timeout, [&]() { profReply->abort(); profLoop.quit(); });
        profTimer.start(20000);
        profLoop.exec();
        if (profReply->error() != QNetworkReply::NoError) {
            profReply->deleteLater();
            QFile::remove(vanillaJar);
            emit downloadFinished(false,
                QString("获取 Fabric 配置失败 (HTTP %1)").arg(profReply->error()));
            return;
        }
        QByteArray raw = profReply->readAll();
        profReply->deleteLater();
        profile = QJsonDocument::fromJson(raw).object();
        if (profile.isEmpty()) {
            QFile::remove(vanillaJar);
            emit downloadFinished(false, "Fabric 加载器配置为空");
            return;
        }
    }

    // 5. Parse libraries from profile
    // Add required core libraries explicitly (may not be in profile)
    QJsonArray allLibs = profile["libraries"].toArray();

    // Ensure fabric-loader and intermediary are in the list
    auto libExists = [&](const QString& artifactId) -> bool {
        for (const auto& lv : allLibs) {
            if (lv.toObject()["name"].toString().contains(artifactId))
                return true;
        }
        return false;
    };
    if (!libExists("fabric-loader")) {
        QJsonObject lo;
        lo["name"] = QString("net.fabricmc:fabric-loader:%1").arg(loaderVersion);
        lo["url"] = FABRIC_MAVEN;
        allLibs.append(lo);
    }
    if (!libExists("intermediary")) {
        QJsonObject io;
        io["name"] = QString("net.fabricmc:intermediary:%1").arg(effectiveVersion);
        io["url"] = FABRIC_MAVEN;
        allLibs.append(io);
    }

    int totalLibs = allLibs.size();
    emit downloadProgress(
        QString("下载 %1 个依赖库 (BMCLAPI Maven)...").arg(totalLibs), 20);

    QStringList classpathEntries;
    // Vanilla server jar comes first on classpath
    QString vanillaRel = QString("minecraft_server.%1.jar").arg(effectiveVersion);
    classpathEntries << vanillaRel;

    QString bmclMaven(QString(BMCLAPI) + "/maven");

    for (int i = 0; i < allLibs.size(); ++i) {
        QJsonObject lib = allLibs[i].toObject();
        QString mvnName = lib["name"].toString();
        QStringList parts = mvnName.split(":");
        if (parts.size() < 3) continue;
        QString group = parts[0];
        QString artifact = parts[1];
        QString ver = parts[2];
        if (ver.startsWith("${")) continue; // skip unresolved variable refs

        QString groupPath = group;
        groupPath.replace('.', '/');
        QString relPath = QString("libraries/%1/%2/%3/%4-%5.jar")
            .arg(groupPath, artifact, ver, artifact, ver);
        QString localPath = dir + "/" + relPath;

        // Ensure parent dir
        QFileInfo lfi(localPath);
        QDir().mkpath(lfi.dir().absolutePath());

        if (!QFile::exists(localPath)) {
            QString libUrl = QString("%1/%2/%3/%4/%5-%4.jar")
                .arg(bmclMaven, groupPath, artifact, ver, artifact);

            emit downloadProgress(
                QString("下载库 [%1/%2] %3").arg(i + 1).arg(totalLibs).arg(mvnName),
                20 + (i * 70 / totalLibs));

            if (!downloadFile(QUrl(libUrl), localPath)) {
                // Retry once
                QThread::msleep(500);
                if (!downloadFile(QUrl(libUrl), localPath)) {
                    QFile::remove(vanillaJar);
                    emit downloadFinished(false,
                        QString("库下载失败: %1").arg(mvnName));
                    return;
                }
            }
        }
        // Use Unix separators for JAR Class-Path
        classpathEntries << relPath;
    }

    // 6. Find Java JDK with jar.exe (needed to build launch JAR)
    QString javaBinDir;
    QString javaExePath; // for server running later

    // Helper: check if a bin dir has both java.exe and jar.exe
    auto isValidJDK = [](const QString& binDir) -> bool {
        return QFile::exists(binDir + "/jar.exe")
            && QFile::exists(binDir + "/java.exe");
    };

    {
        // 1) Scan "C:/Program Files/Java" etc FIRST (most reliable JDK locations)
        QStringList searchDirs = {
            "C:/Program Files/Java",
            "C:/Program Files/Eclipse Adoptium",
            "C:/Program Files/Microsoft",
            "C:/Program Files/Amazon Corretto",
            "C:/Program Files/ojdkbuild",
            "C:/Program Files/Zulu",
        };
        for (const QString& base : searchDirs) {
            QDir baseDir(base);
            if (!baseDir.exists()) continue;
            QStringList subDirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                    QDir::Name | QDir::Reversed);
            for (const QString& sd : subDirs) {
                QString candidate = base + "/" + sd + "/bin";
                if (isValidJDK(candidate)) {
                    javaBinDir = QDir::toNativeSeparators(candidate);
                    break;
                }
            }
            if (!javaBinDir.isEmpty()) break;
        }

        // 2) JAVA_HOME
        if (javaBinDir.isEmpty()) {
            QString javaHome = qEnvironmentVariable("JAVA_HOME");
            if (!javaHome.isEmpty() && isValidJDK(javaHome + "/bin")) {
                javaBinDir = QDir::toNativeSeparators(javaHome + "/bin");
            }
        }

        // 3) PATH (last priority — may be JRE not JDK)
        if (javaBinDir.isEmpty()) {
            QProcess which;
            which.start("where", QStringList() << "java");
            which.waitForFinished(5000);
            QString out = which.readAllStandardOutput().trimmed();
            if (!out.isEmpty()) {
                QStringList paths = out.split("\n", Qt::SkipEmptyParts);
                for (const QString& p : paths) {
                    QString cand = QFileInfo(p.trimmed()).absolutePath();
                    if (isValidJDK(cand)) {
                        javaBinDir = QDir::toNativeSeparators(cand);
                        break;
                    }
                }
            }
        }
    }
    if (javaBinDir.isEmpty()) {
        QFile::remove(vanillaJar);
        emit downloadFinished(false,
            QString("未找到 Java。请安装 Java 17+ 或更高版本，"
                    "并确保其位于系统 PATH 中，或设置 JAVA_HOME 环境变量。"));
        return;
    }

    // 7. Create server.jar — fat JAR with fabric-loader + ASM classes embedded
    emit downloadProgress("创建 server.jar 启动配置...", 90);

    QString jarTool = QDir::toNativeSeparators(javaBinDir + "/jar.exe");

    if (!QFile::exists(jarTool)) {
        QFile::remove(vanillaJar);
        emit downloadFinished(false,
            QString("未找到 jar.exe（需要 JDK，不是 JRE）\n"
                    "请在 JAVA_HOME=\"%1\" 下安装完整 JDK")
                .arg(javaBinDir));
        return;
    }

    // Work in a temp dir to build the JAR contents
    QString jarWork = dir + "/.jarwork";
    QDir().mkpath(jarWork);

    // Helper: extract a JAR into jarWork
    auto extractJar = [&](const QString& jarRelPath) -> bool {
        QString absJar = dir + "/" + jarRelPath;
        if (!QFile::exists(absJar)) return false;
        QProcess ep;
        ep.setWorkingDirectory(jarWork);
        ep.start(jarTool, QStringList() << "xf"
                  << QDir::toNativeSeparators(absJar));
        ep.waitForFinished(30000);
        return ep.exitCode() == 0;
    };

    // 7a. Extract ALL net.fabricmc:* and org.ow2.asm:* JARs into the fat JAR.
    //     These are Fabric Loader's bootstrap chain: fabric-loader itself,
    //     sponge-mixin (core Mixin engine), mixinextras, and ASM bytecode lib.
    //     All of these reference each other and use ServiceLoader
    //     (META-INF/services/) during static initialization, before knot
    //     ClassLoader exists. Cherry-picking causes missing service files
    //     and missing classes.
    //     By embedding ALL of them, META-INF/services/ files from different
    //     JARs are merged (jar xf merges directories), so ServiceLoader
    //     finds all implementations.
    emit downloadProgress("提取 Fabric 核心运行时...", 91);
    bool hasLoader = false;
    for (const QString& cp : classpathEntries) {
        bool shouldExtract = false;
        // fabric-loader: main loader
        if (cp.contains("fabric-loader")) {
            shouldExtract = true;
            hasLoader = true;
        }
        // sponge-mixin: Mixin engine
        else if (cp.contains("sponge-mixin") || cp.contains("fabricmc/sponge")) {
            shouldExtract = true;
        }
        // mixinextras: Mixin extension library
        else if (cp.contains("mixinextras")) {
            shouldExtract = true;
        }
        // ASM: bytecode manipulation (ow2.asm)
        else if (cp.contains("ow2/asm/")) {
            shouldExtract = true;
        }

        if (shouldExtract) {
            if (!extractJar(cp)) {
                qWarning() << "Failed to extract:" << cp;
            }
        }
    }

    if (!hasLoader) {
        QDir(jarWork).removeRecursively();
        QFile::remove(vanillaJar);
        emit downloadFinished(false, "未找到 fabric-loader.jar 在 classpath 中");
        return;
    }

    // 7b. Remove any JAR signature files (META-INF/*.SF, *.RSA, *.DSA)
    //     which would invalidate the fat JAR signature
    {
        QDir metaDir(jarWork + "/META-INF");
        QStringList sigFiles = metaDir.entryList(
            QStringList() << "*.SF" << "*.RSA" << "*.DSA", QDir::Files);
        for (const QString& sf : sigFiles) {
            QFile::remove(metaDir.filePath(sf));
        }
        // Also remove MANIFEST.MF from extracted JARs — we'll write our own
        QFile::remove(metaDir.filePath("MANIFEST.MF"));
    }

    // 7c. Write our custom MANIFEST.MF
    //     NO Class-Path — all needed classes are embedded in this fat JAR
    QDir().mkpath(jarWork + "/META-INF");
    QString manifestPath = jarWork + "/META-INF/MANIFEST.MF";
    {
        QFile mf(manifestPath);
        if (mf.open(QIODevice::WriteOnly)) {
            QByteArray data;
            data.append("Manifest-Version: 1.0\r\n");
            data.append("Main-Class: net.fabricmc.loader.impl.launch.knot.KnotServer\r\n");
            data.append("\r\n");
            mf.write(data);
        }
    }

    // 7d. Write fabric-installer.json
    //     EXCLUDE all net.fabricmc:* and org.ow2.asm:* libraries — their
    //     classes are embedded in the fat JAR (system 'app' ClassLoader).
    //     If listed here, knot would load them twice → ClassCastException.
    {
        QJsonObject installerJson;
        installerJson["version"] = 1;
        QJsonObject mainClass;
        mainClass["server"] = QString("net.minecraft.server.Main");
        mainClass["client"] = QString("net.minecraft.client.main.Main");
        installerJson["mainClass"] = mainClass;

        QJsonObject libs;
        QJsonArray commonLibs;
        QJsonArray allLibsArray = profile["libraries"].toArray();
        for (const auto& lv : allLibsArray) {
            QJsonObject lib = lv.toObject();
            QString name = lib["name"].toString();
            if (name.isEmpty() || name.startsWith("${")) continue;
            // ALL net.fabricmc:* libs are embedded in fat JAR
            if (name.startsWith("net.fabricmc:")) continue;
            // org.ow2.asm:* (ASM bytecode) embedded in fat JAR
            if (name.startsWith("org.ow2.asm:")) continue;
            commonLibs.append(lib);
        }
        libs["common"] = commonLibs;
        libs["server"] = QJsonArray();
        installerJson["libraries"] = libs;

        QFile ij(jarWork + "/fabric-installer.json");
        if (ij.open(QIODevice::WriteOnly)) {
            ij.write(QJsonDocument(installerJson).toJson(QJsonDocument::Compact));
        }
    }

    // 7e. Embed the full Fabric profile JSON (for debugging/reference)
    {
        QFile pf(jarWork + "/fabric-profile.json");
        if (pf.open(QIODevice::WriteOnly)) {
            pf.write(QJsonDocument(profile).toJson(QJsonDocument::Compact));
        }
    }

    // 7f. Create the fat JAR from jarWork directory
    QString serverJar = dir + "/server.jar";
    QFile::remove(serverJar);

    QProcess jarProc;
    jarProc.setWorkingDirectory(dir);
    jarProc.start(jarTool,
        QStringList() << "cfm"
                      << QDir::toNativeSeparators(serverJar)
                      << QDir::toNativeSeparators(manifestPath)
                      << "-C" << QDir::toNativeSeparators(jarWork)
                      << ".");

    bool jarDone = jarProc.waitForFinished(30000);
    QString jarStdErr = QString(jarProc.readAllStandardError());
    QString jarStdOut = QString(jarProc.readAllStandardOutput());

    // Clean up temp dir
    QDir(jarWork).removeRecursively();

    if (!jarDone || jarProc.exitCode() != 0 || !QFile::exists(serverJar)) {
        QFile::remove(vanillaJar);
        emit downloadFinished(false,
            QString("创建 server.jar 失败 (exit %1)\ncmd: %2 cfm ...\nstderr: %3\nstdout: %4")
                .arg(jarProc.exitCode())
                .arg(jarTool, jarStdErr.left(500), jarStdOut.left(300)));
        return;
    }

    // 8. Save classpath for server launch
    //    server.jar is a fat JAR containing fabric-loader + ASM classes.
    //    Only TWO JARs on system 'app' ClassLoader:
    //    (a) server.jar  — fat JAR (fabric-loader, ASM, fabric-installer.json)
    //    (b) game JAR    — vanilla Minecraft server
    //    All other libs (mixin, intermediary, mixinextras, etc.) are loaded
    //    by Fabric's 'knot' ClassLoader from disk via fabric-installer.json.
    //    Embedded classes (fabric-loader, ASM) are excluded from
    //    fabric-installer.json → knot delegates to parent 'app' for them.
    emit downloadProgress("保存启动配置...", 98);
    {
        QJsonObject cpJson;
        cpJson["mainClass"] = QString("net.fabricmc.loader.impl.launch.knot.KnotServer");
        QJsonArray cpArray;
        cpArray.append(QString("server.jar"));      // (a) fat JAR
        cpArray.append(vanillaRel);                  // (b) game JAR
        cpJson["entries"] = cpArray;

        QString cpPath = dir + "/fabric-classpath.json";
        QFile cpf(cpPath);
        if (cpf.open(QIODevice::WriteOnly)) {
            cpf.write(QJsonDocument(cpJson).toJson(QJsonDocument::Compact));
            cpf.close();
        }
    }

    emit downloadProgress("Fabric 服务端准备完成", 100);
    emit downloadFinished(true,
        QString("Fabric %1 (Loader %2) 安装完成").arg(effectiveVersion, loaderVersion));
}
