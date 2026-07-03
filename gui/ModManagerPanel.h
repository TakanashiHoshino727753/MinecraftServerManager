#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "core/ConfigStore.h"
#include "core/ModManager.h"
#include "ui_ModManagerPanel.h"

class ModManagerPanel : public QWidget {
    Q_OBJECT
public:
    explicit ModManagerPanel(ConfigStore* store, QWidget* parent = nullptr);
    void loadServer(const QString& serverId);
    void refresh();

signals:
    void modsChanged();

private slots:
    void onScanMods();
    void onInstallFromFile();
    void onConflictCheck();
    void onToggleMod(int row);
    void onRemoveMod(int row);
    void onInstallMarketMod(int index);

    // Modrinth search
    void onSearchModrinth();
    void onSearchPrev();
    void onSearchNext();
    void onSelectMod(const QString& projectId, const QString& modName, const QString& iconUrl, const QString& desc);
    void onInstallVersion(const QString& versionId, const QString& versionName);

private:
    void updateModTable();
    void updateModrinthResults(const QJsonArray& results, int total, int offset, int limit);

    ModInfo parseModJarForPath(const QString& jarPath) const;

    Ui::ModManagerPanel ui;
    ConfigStore*  m_store;
    ModManager*   m_modManager;
    QString       m_serverId;
    QString       m_serverPath;
    QString       m_serverLoader;

    QTableWidget* m_modTable;
    QGroupBox*    m_marketGroup;
    QPushButton*  m_installFileBtn;
    QPushButton*  m_conflictCheckBtn;
    QPushButton*  m_scanBtn;
    QLabel*       m_statusLabel;
    bool          m_hasServer = false;

    // Modrinth search UI widgets
    QLineEdit*    m_mrSearchInput;
    QComboBox*    m_mrLoaderCombo;
    QPushButton*  m_mrSearchBtn;
    QScrollArea*  m_mrResultScroll;
    QWidget*      m_mrResultContent;
    QVBoxLayout*  m_mrResultLayout;
    QPushButton*  m_mrPrevBtn;
    QPushButton*  m_mrNextBtn;
    QLabel*       m_mrPageLabel;

    // Modrinth state
    QNetworkAccessManager* m_network;
    int     m_mrOffset = 0;
    int     m_mrTotal = 0;
    QString m_mrQuery;
    QString m_mrLoader;

    // Version picker dialog
    QDialog*     m_versionDialog = nullptr;
    QListWidget* m_versionList = nullptr;
    QLabel*      m_versionTitle = nullptr;
    QPushButton* m_versionCloseBtn = nullptr;
    QLabel*      m_versionLoading = nullptr;
    QString      m_mrProjectId;
    QString      m_mrModName;
};
