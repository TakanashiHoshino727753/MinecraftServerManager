#include <QApplication>
#include <QIcon>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include "gui/MainWindow.h"

static const char* INSTANCE_KEY = "MCServerManager_SingleInstance_v1";

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MCServerManager");
    app.setOrganizationName("MCSM");

    // ── Single-instance enforcement ──
    // 当关闭窗口后程序仍在托盘运行，再次双击 exe 应激活已有窗口而非启动新进程

    QSharedMemory sharedMem{QLatin1String(INSTANCE_KEY)};

    if (!sharedMem.create(1)) {
        if (sharedMem.error() == QSharedMemory::AlreadyExists) {
            // 已有实例运行 → 发送激活信号并退出
            QLocalSocket sock;
            sock.connectToServer(QLatin1String(INSTANCE_KEY));
            if (sock.waitForConnected(2000)) {
                sock.write("SHOW", 4);
                sock.waitForBytesWritten(2000);
            }
            return 0;
        }
        // 其他错误（如权限不足）→ 降级运行，不阻挡启动
    }

    // ── 启动本地服务器，接收"显示窗口"信号 ──
    QLocalServer::removeServer(QLatin1String(INSTANCE_KEY));
    QLocalServer wakeServer;

    // 窗口图标（从 .qrc 加载，替换 icons/app.svg 即可更换）
    app.setWindowIcon(QIcon(":/icons/app.svg"));

    MainWindow window;

    QObject::connect(&wakeServer, &QLocalServer::newConnection, &wakeServer, [&]() {
        QLocalSocket* client = wakeServer.nextPendingConnection();
        if (client) {
            client->waitForReadyRead(500);
            client->deleteLater();
            window.showNormal();
            window.raise();
            window.activateWindow();
        }
    });

    wakeServer.listen(QLatin1String(INSTANCE_KEY));

    window.show();

    return app.exec();
}
