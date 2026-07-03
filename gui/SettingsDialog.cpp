#include "SettingsDialog.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QApplication>
#include <QClipboard>
#include <QUuid>
#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>

SettingsDialog::SettingsDialog(AppSettings* settings, QWidget* parent)
    : QDialog(parent), m_settings(settings)
{
    ui.setupUi(this);

    // ── Pointer aliases ──
    m_autoStartCheck    = ui.autoStartCheck;
    m_webApiEnableCheck = ui.webApiEnableCheck;
    m_webApiPortSpin    = ui.webApiPortSpin;
    m_webApiTokenEdit   = ui.webApiTokenEdit;
    m_webApiUrlLabel    = ui.webApiUrlLabel;
    m_webApiStatusLabel = ui.webApiStatusLabel;
    m_langCombo         = ui.langCombo;
    m_webUIFollowRadio  = ui.webUIFollowRadio;
    m_webUIManualRadio  = ui.webUIManualRadio;
    m_webUILangCombo    = ui.webUILangCombo;
    m_accentCombo       = ui.accentCombo;
    m_bgStyleCombo      = ui.bgStyleCombo;
    m_customBgBtn       = ui.customBgBtn;
    m_bgPreviewLabel    = ui.bgPreviewLabel;

    // ── Connects ──
    connect(ui.cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui.saveBtn,   &QPushButton::clicked, this, &SettingsDialog::onSave);

    connect(m_webApiEnableCheck, &QCheckBox::toggled, this, &SettingsDialog::onToggleWebApi);
    connect(ui.genBtn,  &QPushButton::clicked, this, &SettingsDialog::onGenerateToken);
    connect(ui.copyUrlBtn, &QPushButton::clicked, this, &SettingsDialog::onCopyUrl);

    connect(m_webUIFollowRadio, &QRadioButton::toggled, this, &SettingsDialog::onWebUIModeChanged);
    connect(m_webUIManualRadio, &QRadioButton::toggled, this, &SettingsDialog::onWebUIModeChanged);

    auto* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(m_webUIFollowRadio);
    modeGroup->addButton(m_webUIManualRadio);

    connect(m_webApiPortSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int port) {
        m_webApiUrlLabel->setText(QString("http://localhost:%1").arg(port));
    });

    connect(m_accentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        QString color = m_accentCombo->currentData().toString();
        ui.colorPreview->setStyleSheet(
            QString("background: %1; border-radius: 14px; border: 2px solid rgba(255,255,255,0.2);").arg(color));
    });

    connect(m_bgStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        bool isCustom = (m_bgStyleCombo->currentData().toString() == "custom");
        m_customBgBtn->setVisible(isCustom);
        m_bgPreviewLabel->setVisible(isCustom);
        if (!isCustom) {
            m_bgPreviewLabel->setText(TR("settings.noBgSelected"));
            m_bgPreviewLabel->setStyleSheet("color: #8888aa; font-size: 12px;");
        } else if (!m_settings->data.customBgPath.isEmpty()) {
            m_bgPreviewLabel->setText(m_settings->data.customBgPath);
            m_bgPreviewLabel->setStyleSheet("color: #00d2a0; font-size: 12px;");
        }
    });

    connect(m_customBgBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, TR("settings.chooseBg"),
            QString(), "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
        if (!path.isEmpty()) {
            m_settings->data.customBgPath = path;
            m_bgPreviewLabel->setText(path);
            m_bgPreviewLabel->setStyleSheet("color: #00d2a0; font-size: 12px;");
        }
    });

    // ── Populate combos ──
    {
        const auto codes = LanguageManager::allLanguageCodes();
        for (const auto& code : codes)
            m_langCombo->addItem(LanguageManager::languageName(code), code);
    }
    {
        const auto codes = LanguageManager::webUILanguageCodes();
        for (const auto& code : codes)
            m_webUILangCombo->addItem(LanguageManager::languageName(code), code);
    }
    {
        struct { QString name; QString color; } accents[] = {
            {"settings.accentPurple",  "#6c5ce7"},
            {"settings.accentBlue",    "#5a8dee"},
            {"settings.accentCyan",    "#22d3ee"},
            {"settings.accentGreen",   "#00d2a0"},
            {"settings.accentOrange",  "#f59e0b"},
            {"settings.accentRed",     "#ef4444"},
            {"settings.accentPink",    "#ec4899"},
            {"settings.accentMono",    "#94a3b8"},
        };
        for (const auto& a : accents)
            m_accentCombo->addItem(TR(a.name), a.color);
    }
    m_bgStyleCombo->addItem(TR("settings.bgNone"),  "none");
    m_bgStyleCombo->addItem(TR("settings.bgUseImage"), "custom");

    // ── Load current values ──
    m_autoStartCheck->setChecked(m_settings->data.autoStart);
    m_webApiEnableCheck->setChecked(m_settings->data.webApiEnabled);
    m_webApiPortSpin->setValue(m_settings->data.webApiPort);
    m_webApiTokenEdit->setText(m_settings->data.webApiToken);
    onToggleWebApi(m_settings->data.webApiEnabled);

    m_originalLanguage = m_settings->data.language;
    m_originalWebUILang = m_settings->data.webUILang;
    {
        int langIdx = m_langCombo->findData(m_settings->data.language);
        if (langIdx >= 0) m_langCombo->setCurrentIndex(langIdx);
    }
    bool manual = (m_settings->data.webUILang != "auto");
    m_webUIFollowRadio->setChecked(!manual);
    m_webUIManualRadio->setChecked(manual);
    {
        int webIdx = m_webUILangCombo->findData(m_settings->data.webUILang == "auto" ? "en" : m_settings->data.webUILang);
        if (webIdx >= 0) m_webUILangCombo->setCurrentIndex(webIdx);
    }
    m_webUILangCombo->setEnabled(manual);

    {
        int accentIdx = m_accentCombo->findData(m_settings->data.themeColor);
        if (accentIdx >= 0) m_accentCombo->setCurrentIndex(accentIdx);
    }
    {
        int bgIdx = m_bgStyleCombo->findData(m_settings->data.bgStyle);
        if (bgIdx >= 0) m_bgStyleCombo->setCurrentIndex(bgIdx);
    }
    m_customBgBtn->setVisible(m_settings->data.bgStyle == "custom");
    m_bgPreviewLabel->setVisible(m_settings->data.bgStyle == "custom");
    if (!m_settings->data.customBgPath.isEmpty()) {
        m_bgPreviewLabel->setText(m_settings->data.customBgPath);
        m_bgPreviewLabel->setStyleSheet("color: #00d2a0; font-size: 12px;");
    }

    applyLanguage();
}

// ── Slots ──

void SettingsDialog::applyLanguage() {
    auto* lm = LanguageManager::instance();

    setWindowTitle(lm->t("settings.title"));
    ui.titleLabel->setText(lm->t("settings.titleLabel"));

    ui.tabWidget->setTabText(0, lm->t("settings.general"));
    ui.tabWidget->setTabText(1, lm->t("settings.language"));
    ui.tabWidget->setTabText(2, lm->t("settings.theme"));
    ui.tabWidget->setTabText(3, lm->t("settings.webApi"));
    ui.tabWidget->setTabText(4, lm->t("settings.botPlugin"));

    // General
    ui.startupGroup->setTitle(lm->t("settings.startup"));
    ui.autoStartCheck->setText(lm->t("settings.autoStart"));
    ui.autoHint->setText(lm->t("settings.autoStartHint"));

    // Language
    ui.appLangGroup->setTitle(lm->t("settings.appLang"));
    ui.langLabel->setText(lm->t("settings.langLabel"));
    ui.langHint->setText(lm->t("settings.langHint"));
    ui.webUIGroup->setTitle(lm->t("settings.webUILang"));
    ui.webUIFollowRadio->setText(lm->t("settings.followLauncher"));
    ui.webUIManualRadio->setText(lm->t("settings.setManually"));
    ui.manualLabel->setText(lm->t("settings.langLabel"));

    // Theme
    ui.accentGroup->setTitle(lm->t("settings.accentColor"));
    ui.accentLabel->setText(lm->t("settings.accentLabel"));
    ui.bgGroup->setTitle(lm->t("settings.bgImage"));
    ui.customBgBtn->setText(lm->t("settings.chooseBg"));
    ui.themeHint->setText(lm->t("settings.themeHint"));

    // Web API
    ui.apiGroup->setTitle(lm->t("settings.webApiGroup"));
    ui.webApiEnableCheck->setText(lm->t("settings.enableWeb"));
    ui.portLabel->setText(lm->t("settings.port"));
    ui.tokenLabel->setText(lm->t("settings.apiToken"));
    ui.genBtn->setText(lm->t("settings.generate"));
    ui.urlLabel->setText(lm->t("settings.webApiUrl"));
    ui.copyUrlBtn->setText(lm->t("settings.copy"));
    ui.apiDesc->setText(lm->t("settings.apiDesc"));
    ui.webApiTokenEdit->setPlaceholderText(lm->t("settings.tokenHint"));

    // Bot Plugin
    ui.botTitle->setText(lm->t("settings.botTitle"));
    ui.botDesc->setText(lm->t("settings.botDesc"));
    ui.cmdGroup->setTitle(lm->t("settings.cmdEndpoint"));
    ui.napcatGroup->setTitle(lm->t("settings.napcatGroup"));
    ui.nbGroup->setTitle(lm->t("settings.nonebotGroup"));

    // Buttons
    ui.cancelBtn->setText(lm->t("settings.cancel"));
    ui.saveBtn->setText(lm->t("settings.saveBtn"));

    // Update dynamic text
    onToggleWebApi(m_webApiEnableCheck->isChecked());

    bool isCustom = (m_bgStyleCombo->currentData().toString() == "custom");
    if (!isCustom) {
        m_bgPreviewLabel->setText(TR("settings.noBgSelected"));
    } else if (m_settings->data.customBgPath.isEmpty()) {
        m_bgPreviewLabel->setText(TR("settings.noBgSelected"));
    }
}

void SettingsDialog::onWebUIModeChanged() {
    bool manual = m_webUIManualRadio->isChecked();
    m_webUILangCombo->setEnabled(manual);
}

void SettingsDialog::onToggleWebApi(bool enabled) {
    m_webApiPortSpin->setEnabled(enabled);
    m_webApiTokenEdit->setEnabled(enabled);
    if (enabled) {
        m_webApiStatusLabel->setText(TR("settings.readyPort") + QString::number(m_webApiPortSpin->value()));
        m_webApiStatusLabel->setStyleSheet("color: #00d2a0; font-size: 12px; margin-top: 4px;");
    } else {
        m_webApiStatusLabel->setText(TR("settings.disabled"));
        m_webApiStatusLabel->setStyleSheet("color: #555577; font-size: 12px; margin-top: 4px;");
    }
}

void SettingsDialog::onGenerateToken() {
    QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_webApiTokenEdit->setText(token);
}

void SettingsDialog::onCopyUrl() {
    QApplication::clipboard()->setText(m_webApiUrlLabel->text());
}

void SettingsDialog::onSave() {
    m_settings->data.webApiEnabled = m_webApiEnableCheck->isChecked();
    m_settings->data.webApiPort    = m_webApiPortSpin->value();
    m_settings->data.webApiToken   = m_webApiTokenEdit->text();

    if (m_autoStartCheck->isChecked() != m_settings->data.autoStart)
        m_settings->setAutoStart(m_autoStartCheck->isChecked());
    else
        m_settings->data.autoStart = m_autoStartCheck->isChecked();

    QString newLang = m_langCombo->currentData().toString();
    if (newLang.isEmpty()) newLang = "en";
    QString newWebUILang = m_webUIFollowRadio->isChecked() ? "auto"
                           : m_webUILangCombo->currentData().toString();
    if (newWebUILang.isEmpty()) newWebUILang = "en";

    bool langChanged = (newLang != m_originalLanguage || newWebUILang != m_originalWebUILang);

    m_settings->data.language  = newLang;
    m_settings->data.webUILang = newWebUILang;

    m_settings->data.themeColor   = m_accentCombo->currentData().toString();
    m_settings->data.bgStyle      = m_bgStyleCombo->currentData().toString();

    m_settings->save();

    auto* lm = LanguageManager::instance();
    bool appLangChanged = (newLang != m_originalLanguage);
    if (appLangChanged) {
        lm->setLanguageCode(newLang);
    }
    lm->setWebUILangCode(newWebUILang);

    if (langChanged) {
        emit languageChanged();
        QMessageBox::information(this, TR("settings.langChanged"),
            TR("settings.langChangedMsg"));
    }

    accept();
}
