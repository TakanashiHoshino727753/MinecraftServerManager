#pragma once
#include <QWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>
#include "core/ConfigStore.h"
#include "core/DownloadManager.h"
#include "ui_CreateWizard.h"

class CreateWizard : public QWidget {
    Q_OBJECT
public:
    explicit CreateWizard(ConfigStore* store, DownloadManager* dl, QWidget* parent = nullptr);
    void reset();

signals:
    void created();

private slots:
    void nextStep();
    void prevStep();
    void createServer();
    void selectPath();
    void selectJava();
    void onVersionsReady(const QStringList& versions);
    void onPaperBuildsReady(const QStringList& builds);

private:
    void setupSteps();
    QWidget* createStepIndicator();
    QWidget* createStep1();
    QWidget* createStep2();
    QWidget* createStep3();
    QWidget* createStep4();
    void refreshStep4();
    void updateStep1Selection();
    void updateNavButtons();
    void updateStepIndicator(int step);
    void fitStackToCurrent();

    Ui::CreateWizard  ui;

    ConfigStore*      m_store;
    DownloadManager*  m_dl;

    int               m_step = 0;

    // Step indicator widgets
    QLabel* m_stepCircles[4];
    QLabel* m_stepLabels[4];
    QLabel* m_stepLines[3];

    // State
    QString m_type = "paper";
    QString m_version;
    QString m_build;
    QString m_serverName;
    QString m_serverPath;
    QString m_javaPath = "java";
    int     m_minRam = 1024;
    int     m_maxRam = 4096;
    bool    m_eulaAccepted = false;

    // Widgets
    QComboBox* m_versionCombo;
    QComboBox* m_buildCombo;
    QLabel*    m_buildLabel;
    QLineEdit* m_nameEdit;
    QLineEdit* m_pathEdit;
    QLineEdit* m_javaEdit;
    QComboBox* m_minRamCombo;
    QComboBox* m_maxRamCombo;
    QCheckBox* m_eulaCheck;
    QFrame*    m_step4Card = nullptr;
    QVBoxLayout* m_step4CardLayout = nullptr;
    QVector<QPushButton*> m_typeCards;
};
