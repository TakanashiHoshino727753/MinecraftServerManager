#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <memory>

class DownloadManager : public QObject {
    Q_OBJECT
public:
    explicit DownloadManager(QObject* parent = nullptr);

    void fetchVersions();
    void fetchPaperBuilds(const QString& version);
    void downloadVanilla(const QString& version, const QString& dir);
    void downloadPaper(const QString& version, const QString& build, const QString& dir);
    void downloadFabric(const QString& version, const QString& dir);

signals:
    void versionsReady(const QStringList& versions);
    void buildsReady(const QStringList& builds);
    void downloadProgress(const QString& status, int percent);
    void downloadFinished(bool success, const QString& msg);
    void error(const QString& msg);

private:
    QNetworkReply* httpGet(const QUrl& url);
    bool downloadFile(const QUrl& url, const QString& filepath);
    bool downloadVanillaSync(const QString& version, const QString& dir);

    QNetworkAccessManager* m_nam;
    static const char* BMCLAPI;
    static const char* FABRIC_META;
    static const char* MAVEN_CENTRAL;
    static const char* FABRIC_MAVEN;
};
