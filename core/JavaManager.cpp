#include "JavaManager.h"
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDirIterator>

JavaManager::JavaManager(QObject* parent) : QObject(parent) {
    m_nam = new QNetworkAccessManager(this);
}

// ── MC version → Java version mapping ──
int JavaManager::requiredJavaVersion(const QString& mcVersion) {
    // Parse major.minor from version string like "1.20.4" or "1.21"
    QStringList parts = mcVersion.split('.');
    if (parts.size() < 2) return 17; // default

    int major = parts[0].toInt();
    int minor = parts[1].toInt();
    int patch = (parts.size() >= 3) ? parts[2].toInt() : 0;

    if (major == 1) {
        if (minor <= 16) return 8;
        if (minor <= 20) return 17;
        // 1.20.5+ needs Java 21
        if (minor == 20 && patch < 5) return 17;
        return 21;
    }
    return 21; // future versions
}

// ── Find all Java installations on the system ──
QList<JavaInfo> JavaManager::findSystemJavas() {
    QList<JavaInfo> result;
    QList<QString> paths = scanCommonPaths();

    for (const auto& p : paths) {
        QProcess proc;
        proc.start(p, {"-version"});
        proc.waitForFinished(5000);
        QString output = proc.readAllStandardError(); // java -version → stderr
        if (output.isEmpty()) output = proc.readAllStandardOutput();

        QRegularExpression re(R"(version\s+\"(\d+)");
        auto match = re.match(output);
        if (match.hasMatch()) {
            JavaInfo info;
            info.majorVersion = match.captured(1).toInt();
            info.path = p;
            info.vendor = output.contains("Temurin") ? "Adoptium" :
                          output.contains("Oracle") ? "Oracle" :
                          output.contains("OpenJDK") ? "OpenJDK" : "Unknown";
            info.isValid = true;
            result.append(info);

            // 避免重复主版本（优先 keep 高补丁）
            // 简单策略：只保留第一个
            // 实际按主版本去重
        }
    }

    // 去重：每个主版本只保留一个
    QList<JavaInfo> deduped;
    QSet<int> seen;
    for (const auto& j : result) {
        if (!seen.contains(j.majorVersion)) {
            seen.insert(j.majorVersion);
            deduped.append(j);
        }
    }
    return deduped;
}

JavaInfo JavaManager::findBestJava(const QString& mcVersion) {
    int required = requiredJavaVersion(mcVersion);
    auto javas = findSystemJavas();

    // 精确匹配
    for (const auto& j : javas) {
        if (j.majorVersion == required) return j;
    }
    // 找 >= required 的最小版本
    JavaInfo best;
    for (const auto& j : javas) {
        if (j.majorVersion >= required) {
            if (!best.isValid || j.majorVersion < best.majorVersion)
                best = j;
        }
    }
    return best;
}

// ── Scan common Java install paths ──
QList<QString> JavaManager::scanCommonPaths() {
    QList<QString> result;

    // JAVA_HOME
    QString javaHome = qEnvironmentVariable("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString exe = QDir(javaHome).filePath("bin/java.exe");
        if (QFile::exists(exe)) result << exe;
    }

    // PATH
    QStringList pathDirs = qEnvironmentVariable("PATH").split(';');
    for (const auto& d : pathDirs) {
        QString exe = QDir(d.trimmed()).filePath("java.exe");
        if (!exe.isEmpty() && QFile::exists(exe) && !result.contains(exe))
            result << exe;
    }

    // Program Files common locations
    QStringList baseRoots = {
        "C:/Program Files/Java",
        "C:/Program Files/Eclipse Adoptium",
        "C:/Program Files/AdoptOpenJDK",
        "C:/Program Files/OpenJDK",
        "C:/Program Files/Microsoft"
    };

    for (const auto& root : baseRoots) {
        QDir baseDir(root);
        if (!baseDir.exists()) continue;

        QFileInfoList entries = baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& entry : entries) {
            QString binDir = entry.absoluteFilePath() + "/bin";
            QString exe = QDir(binDir).filePath("java.exe");
            if (QFile::exists(exe) && !result.contains(exe))
                result << exe;
        }
    }

    return result;
}

// ── Adoptium download URL ──
QString JavaManager::adoptiumUrl(int javaVersion) {
    // Use Adoptium v3 API: latest LTS/GA for Windows x64
    QString featureVersion = QString::number(javaVersion);
    return QString(
        "https://api.adoptium.net/v3/binary/latest/%1/ga/windows/x64/jdk/hotspot/normal/eclipse"
    ).arg(featureVersion);
}

// ── Download Java ──
void JavaManager::downloadJava(int javaVersion, const QString& destDir) {
    if (m_downloading) return;
    m_downloading = true;

    QString url = adoptiumUrl(javaVersion);
    emit log(QString("Downloading Java %1 from Adoptium...").arg(javaVersion));

    QNetworkReply* reply = m_nam->get(QNetworkRequest(QUrl(url)));
    reply->setReadBufferSize(0); // Don't buffer in memory (large file)

    // Stream to temp file
    QString tmpFile = destDir + "/jdk_temp.msi";
    QFile* file = new QFile(tmpFile, this);
    if (!file->open(QIODevice::WriteOnly)) {
        emit downloadError("Cannot write to " + tmpFile);
        m_downloading = false;
        reply->deleteLater();
        return;
    }

    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        file->write(reply->readAll());
    });

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 recv, qint64 total) {
        if (total > 0)
            emit downloadProgress((int)(recv * 100 / total));
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, file, tmpFile, destDir]() {
        file->close();
        m_downloading = false;

        if (reply->error() != QNetworkReply::NoError) {
            file->remove();
            emit downloadError(reply->errorString());
            reply->deleteLater();
            return;
        }

        // Run the MSI installer silently
        emit log("Download complete. Installing...");
        QString installDir = QString(destDir).replace('/', '\\');
        QProcess proc;
        proc.start("msiexec.exe", {"/i", tmpFile, "/quiet", "/norestart",
                   "INSTALLDIR=" + installDir});
        proc.waitForFinished(300000); // 5 min timeout

        file->remove(); // Cleanup MSI
        QFile::remove(tmpFile);

        // Find installed java.exe
        QDirIterator it(destDir, {"java.exe"}, QDir::Files,
                        QDirIterator::Subdirectories);
        if (it.hasNext()) {
            QString javaPath = it.next();
            emit downloadFinished(javaPath);
        } else {
            emit downloadError("Java installed but java.exe not found in " + destDir);
        }

        reply->deleteLater();
    });
}
