#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include "models/ServerConfig.h"

class Watchdog;

class ServerProcess : public QObject {
    Q_OBJECT
public:
    enum State { Stopped, Starting, Running, Stopping };
    Q_ENUM(State)

    explicit ServerProcess(QObject* parent = nullptr);
    ~ServerProcess();

    void start(const ServerConfig& cfg);
    void stop();
    void kill();
    void sendCommand(const QString& cmd);
    State state() const { return m_state; }
    qint64 uptimeSecs() const;
    QString serverId() const { return m_serverId; }

    // Watchdog: 启动后 60s 内未检测到 "Done" 则自动杀进程
    int  watchdogRemaining() const;
    bool isWatchdogArmed() const;

signals:
    void stateChanged(State newState);
    void consoleOutput(const QString& line);
    void error(const QString& msg);
    void watchdogTick(int remainingSecs);

private slots:
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void tickUptime();

private:
    QProcess*  m_process = nullptr;
    QTimer*    m_uptimeTimer = nullptr;
    Watchdog*  m_watchdog = nullptr;
    State      m_state = Stopped;
    qint64     m_startTime = 0;
    qint64     m_uptime = 0;
    QString    m_serverId;
};
