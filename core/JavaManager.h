#pragma once
#include <QString>
#include <QStringList>
#include <QObject>
#include <QNetworkAccessManager>

// ── Java Manager ────────────────────────────────────────────────────
// 根据 MC 版本自动选择 Java 版本；检测系统已有 Java；支持下载 Adoptium

struct JavaInfo {
    int     majorVersion = 0;
    QString path;
    QString vendor;
    bool    isValid = false;
};

class JavaManager : public QObject {
    Q_OBJECT
public:
    explicit JavaManager(QObject* parent = nullptr);

    // 根据 MC 版本号返回所需 Java 主版本
    static int requiredJavaVersion(const QString& mcVersion);

    // 扫描系统找到的 Java
    QList<JavaInfo> findSystemJavas();

    // 在已找到的列表中匹配最优
    JavaInfo findBestJava(const QString& mcVersion);

    // 下载 Java（Adoptium API）
    void downloadJava(int javaVersion, const QString& destDir);
    bool isDownloading() const { return m_downloading; }
    static QString adoptiumUrl(int javaVersion);

signals:
    void downloadProgress(int percent);
    void downloadFinished(const QString& javaPath);
    void downloadError(const QString& err);
    void log(const QString& msg);

private:
    QString detectJavaPath(const QString& exePath);
    QList<QString> scanCommonPaths();

    QNetworkAccessManager* m_nam = nullptr;
    bool m_downloading = false;
};
