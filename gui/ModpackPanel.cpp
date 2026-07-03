#include "ModpackPanel.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QUuid>
#include <QDir>
#include <QTimer>

ModpackPanel::ModpackPanel(ConfigStore* store, QWidget* parent)
    : QWidget(parent), m_store(store), m_importer(new ModpackImporter(this))
{
    ui.setupUi(this);

    // ── Pointer aliases ──
    m_scroll          = ui.scroll;
    m_page            = ui.page;
    m_sourceEdit      = ui.sourceEdit;
    m_sourceBrowseBtn = ui.sourceBrowseBtn;
    m_analyzeBtn      = ui.analyzeBtn;
    m_analysisGroup   = ui.analysisGroup;
    m_analysisName    = ui.analysisName;
    m_analysisVersion = ui.analysisVersion;
    m_analysisLoader  = ui.analysisLoader;
    m_analysisModCount= ui.analysisModCount;
    m_targetEdit      = ui.targetEdit;
    m_targetBrowseBtn = ui.targetBrowseBtn;
    m_importBtn       = ui.importBtn;
    m_progress        = ui.progressBar;
    m_statusLabel     = ui.statusLabel;

    // ── Connects ──
    connect(m_importer, &ModpackImporter::progress, this, &ModpackPanel::onImporterProgress);
    connect(m_importer, &ModpackImporter::error, this, &ModpackPanel::onImporterError);
    connect(m_importer, &ModpackImporter::finished, this, [this](const ModpackImporter::ModpackInfo& info) {
        m_info = info; m_analyzed = true;
        m_analysisName->setText(info.name);
        m_analysisVersion->setText(info.mcVersion.isEmpty() ? TR("panel.modpack.unknown") : info.mcVersion);
        m_analysisLoader->setText(info.loader);
        m_analysisModCount->setText(QString::number(info.modFiles.size()) + " " + TR("panel.modpack.mods"));
        m_analysisGroup->setVisible(true);
        m_importBtn->setEnabled(true);
        m_statusLabel->setText(TR("panel.modpack.ready"));
        m_statusLabel->setStyleSheet("color: #00d2a0; font-size: 12px; padding: 4px 0;");
    });
    connect(m_sourceBrowseBtn, &QPushButton::clicked, this, &ModpackPanel::onBrowseSource);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &ModpackPanel::onAnalyze);
    connect(m_targetBrowseBtn, &QPushButton::clicked, this, &ModpackPanel::onBrowseTarget);
    connect(m_importBtn, &QPushButton::clicked, this, &ModpackPanel::onImport);

    // ── i18n texts ──
    ui.titleLabel->setText(TR("panel.modpack.title"));
    m_statusLabel->setText(TR("panel.modpack.selectHint"));
    ui.selGroup->setTitle(TR("panel.modpack.selectSource"));
    m_sourceEdit->setPlaceholderText(TR("panel.modpack.sourcePlaceholder"));
    m_sourceBrowseBtn->setText(TR("panel.modpack.browse"));
    m_analyzeBtn->setText(TR("panel.modpack.analyze"));
    ui.analysisGroup->setTitle(TR("panel.modpack.analysisResult"));
    ui.analysisNameLabel->setText(TR("panel.modpack.nameLabel"));
    ui.analysisVersionLabel->setText(TR("panel.modpack.versionLabel"));
    ui.analysisLoaderLabel->setText(TR("panel.modpack.loaderLabel"));
    ui.analysisModCountLabel->setText(TR("panel.modpack.modCountLabel"));
    ui.tgtGroup->setTitle(TR("panel.modpack.targetDir"));
    m_targetEdit->setPlaceholderText(TR("panel.modpack.targetPlaceholder"));
    m_targetBrowseBtn->setText(TR("panel.modpack.browse"));
    m_importBtn->setText(TR("panel.modpack.importBtn"));
}

void ModpackPanel::onBrowseSource() {
    QString dir = QFileDialog::getExistingDirectory(this, TR("panel.modpack.selectSourceDir"));
    if (!dir.isEmpty()) m_sourceEdit->setText(dir);
}
void ModpackPanel::onBrowseTarget() {
    QString dir = QFileDialog::getExistingDirectory(this, TR("panel.modpack.selectTargetDir"));
    if (!dir.isEmpty()) m_targetEdit->setText(dir);
}
void ModpackPanel::onAnalyze() {
    QString path = m_sourceEdit->text().trimmed();
    if (path.isEmpty()) { m_statusLabel->setText(TR("panel.modpack.noPath")); return; }
    m_statusLabel->setText(TR("panel.modpack.analyzing"));
    m_statusLabel->setStyleSheet("color: #facc15; font-size: 12px;");
    m_analyzed = false;
    m_analysisGroup->setVisible(false);
    m_importBtn->setEnabled(false);
    m_importer->analyze(path);
}
void ModpackPanel::onImport() {
    if (!m_analyzed || !m_info.valid) return;
    QString targetDir = m_targetEdit->text().trimmed();
    if (targetDir.isEmpty()) { m_statusLabel->setText(TR("panel.modpack.noTarget")); return; }
    m_importBtn->setEnabled(false);
    m_progress->setVisible(true);
    m_progress->setValue(0);
    m_statusLabel->setText(TR("panel.modpack.installing"));
    m_statusLabel->setStyleSheet("color: #facc15; font-size: 12px;");
    m_importer->install(m_info, targetDir);
    ServerConfig cfg = m_importer->generateConfig(m_info, targetDir);
    cfg.name = m_info.name.isEmpty() ? QFileInfo(targetDir).fileName() : m_info.name;
    m_store->saveServer(cfg);
    m_progress->setValue(100);
    m_statusLabel->setText(TR("panel.modpack.importDone"));
    m_statusLabel->setStyleSheet("color: #00d2a0; font-size: 14px; font-weight: bold;");
    QTimer::singleShot(1500, this, [this]() { emit importCompleted(); });
}
void ModpackPanel::onImporterProgress(const QString& step) {
    m_statusLabel->setText(step);
    m_statusLabel->setStyleSheet("color: #8888aa; font-size: 12px;");
}
void ModpackPanel::onImporterError(const QString& msg) {
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px;");
}
void ModpackPanel::clearAnalysis() {
    m_analyzed = false;
    m_info = {};
    m_analysisGroup->setVisible(false);
    m_importBtn->setEnabled(false);
}
