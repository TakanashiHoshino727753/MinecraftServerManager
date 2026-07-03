#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QGroupBox>
#include "core/ConfigStore.h"
#include "core/ModpackImporter.h"
#include "ui_ModpackPanel.h"

class ModpackPanel : public QWidget {
    Q_OBJECT
public:
    explicit ModpackPanel(ConfigStore* store, QWidget* parent = nullptr);

signals:
    void importCompleted();

private slots:
    void onBrowseSource();
    void onBrowseTarget();
    void onAnalyze();
    void onImport();
    void onImporterProgress(const QString& step);
    void onImporterError(const QString& message);

private:
    void clearAnalysis();

    Ui::ModpackPanel ui;
    ConfigStore*      m_store;
    ModpackImporter*  m_importer;
    ModpackImporter::ModpackInfo m_info;
    bool              m_analyzed = false;

    QLineEdit*   m_sourceEdit;
    QPushButton* m_sourceBrowseBtn;
    QPushButton* m_analyzeBtn;

    QGroupBox*   m_analysisGroup;
    QLabel*      m_analysisName;
    QLabel*      m_analysisVersion;
    QLabel*      m_analysisLoader;
    QLabel*      m_analysisModCount;

    QLineEdit*   m_targetEdit;
    QPushButton* m_targetBrowseBtn;

    QPushButton*  m_importBtn;
    QProgressBar* m_progress;
    QLabel*       m_statusLabel;
    QScrollArea*  m_scroll = nullptr;
    QWidget*      m_page = nullptr;
};
