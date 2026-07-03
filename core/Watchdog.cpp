#include "Watchdog.h"

Watchdog::Watchdog(QObject* parent) : QObject(parent) {
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        m_remaining--;
        emit tick(m_remaining);
        if (m_remaining <= 0) {
            m_armed = false;
            m_timer->stop();
            emit timeout();
        }
    });
}

void Watchdog::start(int timeoutSecs) {
    m_timeoutSecs = timeoutSecs;
    m_remaining   = timeoutSecs;
    m_armed       = true;
    m_timer->start();
}

void Watchdog::feed() {
    m_armed = false;
    m_timer->stop();
}

void Watchdog::stop() {
    m_armed = false;
    m_timer->stop();
}
