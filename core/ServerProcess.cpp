#include "ServerProcess.h"
#include "Watchdog.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

ServerProcess::ServerProcess(QObject* parent) : QObject(parent) {
    m_uptimeTimer = new QTimer(this);
    connect(m_uptimeTimer, &QTimer::timeout, this, &ServerProcess::tickUptime);

    // ── Watchdog ──
    m_watchdog = new Watchdog(this);
    connect(m_watchdog, &Watchdog::timeout, this, [this]() {
        emit consoleOutput("[Watchdog] Server did not start within 60s — killing process");
        if (m_process && m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
        m_state = Stopped;
        m_uptimeTimer->stop();
        emit error("Watchdog: server startup timed out (60s). The server may be stuck or incompatible.");
        emit stateChanged(Stopped);
    });
    connect(m_watchdog, &Watchdog::tick, this, [this](int remaining) {
        emit watchdogTick(remaining);
    });
}

ServerProcess::~ServerProcess() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void ServerProcess::start(const ServerConfig& cfg) {
    if (m_state == Running) return;
    m_serverId = cfg.id;

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(cfg.path);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyRead, this, &ServerProcess::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ServerProcess::onFinished);

    QStringList args;
    args << QString("-Xms%1M").arg(cfg.minRamMB)
         << QString("-Xmx%1M").arg(cfg.maxRamMB);
    if (!cfg.javaArgs.isEmpty()) {
        for (const auto& a : cfg.javaArgs.split(' ', Qt::SkipEmptyParts))
            args << a;
    }

    // Fabric: use -cp with explicit classpath (official Fabric approach)
    bool useClasspath = false;
    if (cfg.type == "fabric") {
        QString cpFile = QDir(cfg.path).filePath("fabric-classpath.json");
        QFile cpf(cpFile);
        if (cpf.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(cpf.readAll());
            cpf.close();
            QJsonObject root = doc.object();
            QString mainClass = root["mainClass"].toString();
            QJsonArray entries = root["entries"].toArray();
            if (!mainClass.isEmpty() && !entries.isEmpty()) {
                useClasspath = true;
                // Build classpath string with native path separator
                QStringList cpList;
                for (const auto& e : entries) {
                    cpList << e.toString();
                }
                args << "-cp" << cpList.join(';');
                args << mainClass;
                args << "nogui";
            }
        }
    }

    if (!useClasspath) {
        args << "-jar" << "server.jar" << "nogui";
    }

    m_state = Starting;
    m_startTime = QDateTime::currentSecsSinceEpoch();
    m_uptimeTimer->start(1000);
    m_watchdog->start(60);  // 60s timeout
    emit stateChanged(Starting);

    m_process->start(cfg.javaPath, args);
    if (!m_process->waitForStarted(15000)) {
        m_state = Stopped;
        emit error("Failed to start Java process: " + m_process->errorString());
        emit stateChanged(Stopped);
        return;
    }
    m_state = Running;
    emit stateChanged(Running);
}

void ServerProcess::stop() {
    if (!m_process || m_state != Running) return;
    m_state = Stopping;
    emit stateChanged(Stopping);
    m_process->write("stop\n");
}

void ServerProcess::kill() {
    if (!m_process || m_process->state() == QProcess::NotRunning) return;
    m_process->kill();
}

void ServerProcess::sendCommand(const QString& cmd) {
    if (!m_process || m_state != Running) return;
    m_process->write((cmd + "\n").toUtf8());
}

qint64 ServerProcess::uptimeSecs() const {
    if (m_state != Running || m_startTime == 0) return m_uptime;
    return QDateTime::currentSecsSinceEpoch() - m_startTime;
}

void ServerProcess::onReadyRead() {
    while (m_process->canReadLine()) {
        QByteArray line = m_process->readLine();
        QString text = QString::fromUtf8(line).trimmed();
        emit consoleOutput(text);

        // Watchdog: detect "Done" → server ready
        if (m_watchdog->isArmed() && text.contains("Done")) {
            m_watchdog->feed();
        }
    }
}

void ServerProcess::onFinished(int exitCode, QProcess::ExitStatus status) {
    m_uptimeTimer->stop();
    m_state = Stopped;
    emit stateChanged(Stopped);
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
}

void ServerProcess::tickUptime() {
    if (m_state == Running) {
        m_uptime = QDateTime::currentSecsSinceEpoch() - m_startTime;
    }
}

int ServerProcess::watchdogRemaining() const {
    return m_watchdog ? m_watchdog->remainingSecs() : 0;
}

bool ServerProcess::isWatchdogArmed() const {
    return m_watchdog ? m_watchdog->isArmed() : false;
}
