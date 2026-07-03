#pragma once
#include <QObject>
#include <QTimer>

// ── Watchdog ────────────────────────────────────────────────────────
// 服务器启动后 60s 倒计时，一旦控制台输出 "Done" 则重置并停用。
// 超时未就绪则自动 kill 进程，防止卡死。

class Watchdog : public QObject {
    Q_OBJECT
public:
    explicit Watchdog(QObject* parent = nullptr);

    void start(int timeoutSecs = 60);
    void feed();        // 收到 "Done" 时调用，重置并停用
    void stop();

    bool isArmed() const { return m_armed; }
    int  remainingSecs() const { return m_remaining; }

signals:
    void timeout();     // 超时 → 外部 kill 进程
    void tick(int remaining);

private:
    QTimer* m_timer = nullptr;
    int     m_remaining = 60;
    int     m_timeoutSecs = 60;
    bool    m_armed = false;   // 仅在启动→就绪期间为 true
};
