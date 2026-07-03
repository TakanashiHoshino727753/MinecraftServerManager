#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QTabBar>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include "core/ConfigStore.h"
#include "core/ServerProcess.h"
#include "core/DownloadManager.h"
#include "core/AppSettings.h"
#include "core/WebApiServer.h"
#include "ui_MainWindow.h"

class ServerListPanel;
class CreateWizard;
class ServerDetailPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private slots:
    void switchToServers();
    void switchToCreate();
    void onServerSelected(const QString& id);
    void onServerCreated();
    void onBackToList();
    void onServerControlRequested(const QString& id);
    void onServerConfigRequested(const QString& id);
    void onTabClicked(int index);
    void onTabCloseRequested(int index);
    void onProcessStateChanged(ServerProcess::State state);
    void openSettingsDialog();
    void onLanguageChanged();

private:
    void initUi();
    void refreshServerCounts();
    void openServerTab(const QString& id);
    void closeServerTabByIndex(int index);
    void closeServerTab(const QString& id);
    void cleanupTabs();
    void applyLanguage();
    void createTrayIcon();
    void createButtonIcons();
    void resetAllNavButtons();
    void hideTabBar();

    Ui::MainWindow    ui;

    ConfigStore*      m_store;
    ServerProcess*    m_process;
    DownloadManager*  m_downloader;
    AppSettings       m_appSettings;
    WebApiServer*     m_webApi = nullptr;

    // Content panels (created in code)
    ServerListPanel*  m_listPanel;
    CreateWizard*     m_createPanel;
    ServerDetailPanel*m_detailPanel;

    // Sidebar buttons
    QPushButton*      m_btnSettings = nullptr;

    // System tray
    QSystemTrayIcon*  m_trayIcon = nullptr;
    QMenu*            m_trayMenu = nullptr;

    // Open server tabs
    QVector<QString>  m_openServerIds;
    int m_listIndex   = 0;
    int m_createIndex = 1;
    int m_detailIndex = 2;
};
