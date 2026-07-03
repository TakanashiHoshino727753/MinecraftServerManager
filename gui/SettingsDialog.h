#pragma once
#include <QDialog>
#include <QButtonGroup>
#include "core/AppSettings.h"
#include "ui_SettingsDialog.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(AppSettings* settings, QWidget* parent = nullptr);

signals:
    void languageChanged();

private slots:
    void onSave();
    void onGenerateToken();
    void onCopyUrl();
    void onToggleWebApi(bool enabled);
    void onWebUIModeChanged();

private:
    void applyLanguage();
    Ui::SettingsDialog ui;
    AppSettings* m_settings;

    // General
    QCheckBox*    m_autoStartCheck;

    // Web API
    QCheckBox*    m_webApiEnableCheck;
    QSpinBox*     m_webApiPortSpin;
    QLineEdit*    m_webApiTokenEdit;
    QLabel*       m_webApiUrlLabel;
    QLabel*       m_webApiStatusLabel;

    // Language
    QComboBox*    m_langCombo;
    QRadioButton* m_webUIFollowRadio;
    QRadioButton* m_webUIManualRadio;
    QComboBox*    m_webUILangCombo;

    // Theme
    QComboBox*    m_accentCombo;
    QComboBox*    m_bgStyleCombo;
    QPushButton*  m_customBgBtn;
    QLabel*       m_bgPreviewLabel;

    QString       m_originalLanguage;
    QString       m_originalWebUILang;
};
