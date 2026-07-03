#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QScrollArea>
#include <QGroupBox>
#include "core/JavaManager.h"
#include "ui_JavaPanel.h"

class JavaPanel : public QWidget {
    Q_OBJECT
public:
    explicit JavaPanel(QWidget* parent = nullptr);
    JavaManager* javaManager() const { return m_javaManager; }
    QString selectedJavaPath() const { return m_selectedJavaPath; }
    void refresh();

private slots:
    void onScanSystem();
    void onSelectJava();
    void onCopyPath();
    void onDownloadJava();
    void onBrowseInstallDir();
    void onDownloadProgress(int percent);
    void onDownloadFinished(const QString& javaPath);
    void onDownloadError(const QString& err);

private:
    void refreshVersionMapping();

    Ui::JavaPanel   ui;
    JavaManager*    m_javaManager;
    QTableWidget*   m_systemJavaTable;
    QPushButton*    m_scanBtn;
    QLabel*         m_selectedLabel;
    QString         m_selectedJavaPath;
    QList<JavaInfo> m_foundJavas;

    QTableWidget*   m_versionMapTable;
    QComboBox*      m_versionCombo;
    QLineEdit*      m_installDirEdit;
    QPushButton*    m_downloadBtn;
    QProgressBar*   m_progressBar;
    QLabel*         m_downloadStatusLabel;
};
