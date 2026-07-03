#include "JavaPanel.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QApplication>
#include <QClipboard>

static const struct { const char* mcVersion; int requiredJava; } kVersionMap[] = {
    {"1.8", 8}, {"1.12", 8}, {"1.16", 8},
    {"1.17", 17}, {"1.18", 17}, {"1.20", 17},
    {"1.20.5", 21}, {"1.21", 21},
};

JavaPanel::JavaPanel(QWidget* parent) : QWidget(parent), m_javaManager(new JavaManager(this)) {
    connect(m_javaManager, &JavaManager::downloadProgress, this, &JavaPanel::onDownloadProgress);
    connect(m_javaManager, &JavaManager::downloadFinished, this, &JavaPanel::onDownloadFinished);
    connect(m_javaManager, &JavaManager::downloadError, this, &JavaPanel::onDownloadError);

    ui.setupUi(this);

    // ── Pointer aliases ──
    m_scanBtn        = ui.scanBtn;
    m_selectedLabel  = ui.selectedLabel;
    m_systemJavaTable = ui.systemJavaTable;
    m_versionMapTable = ui.versionMapTable;
    m_versionCombo    = ui.versionCombo;
    m_installDirEdit  = ui.installDirEdit;
    m_downloadBtn     = ui.downloadBtn;
    m_progressBar     = ui.progressBar;
    m_downloadStatusLabel = ui.downloadStatusLabel;

    // ── Connects ──
    connect(m_scanBtn, &QPushButton::clicked, this, &JavaPanel::onScanSystem);
    connect(ui.copyBtn, &QPushButton::clicked, this, &JavaPanel::onCopyPath);
    connect(ui.browseBtn, &QPushButton::clicked, this, &JavaPanel::onBrowseInstallDir);
    connect(m_downloadBtn, &QPushButton::clicked, this, &JavaPanel::onDownloadJava);

    // ── Setup tables ──
    m_systemJavaTable->setHorizontalHeaderLabels({TR("panel.java.colVersion"), TR("panel.java.colVendor"), TR("panel.java.colPath"), ""});
    m_systemJavaTable->horizontalHeader()->setStretchLastSection(true);
    m_systemJavaTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_systemJavaTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_systemJavaTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    m_versionMapTable->setHorizontalHeaderLabels({TR("panel.java.colMCVersion"), TR("panel.java.colRequiredJava"), TR("panel.java.colCompatible")});
    m_versionMapTable->horizontalHeader()->setStretchLastSection(true);

    refreshVersionMapping();
}

void JavaPanel::refresh() { onScanSystem(); }

void JavaPanel::onScanSystem() {
    m_foundJavas = m_javaManager->findSystemJavas();
    m_systemJavaTable->setRowCount(0);
    for (int i = 0; i < m_foundJavas.size(); i++) {
        m_systemJavaTable->insertRow(i);
        m_systemJavaTable->setItem(i, 0, new QTableWidgetItem("Java " + QString::number(m_foundJavas[i].majorVersion)));
        m_systemJavaTable->setItem(i, 1, new QTableWidgetItem(m_foundJavas[i].vendor));
        m_systemJavaTable->setItem(i, 2, new QTableWidgetItem(m_foundJavas[i].path));
        auto* selBtn = new QPushButton(TR("panel.java.select"));
        selBtn->setFixedSize(60, 26);
        selBtn->setStyleSheet("QPushButton { background: rgba(96,165,250,0.15); border: 1px solid rgba(96,165,250,0.3); border-radius: 4px; color: #60a5fa; font-size: 11px; } QPushButton:hover { background: rgba(96,165,250,0.3); }");
        connect(selBtn, &QPushButton::clicked, this, &JavaPanel::onSelectJava);
        m_systemJavaTable->setCellWidget(i, 3, selBtn);
        m_systemJavaTable->setRowHeight(i, 34);
    }
    refreshVersionMapping();
    m_selectedLabel->setText(m_foundJavas.isEmpty() ? TR("panel.java.noJavaFound") : TR("panel.java.foundCount").arg(QString::number(m_foundJavas.size())));
    m_selectedLabel->setStyleSheet(m_foundJavas.isEmpty() ? "color: #ff4757; font-size: 12px;" : "color: #00d2a0; font-size: 12px;");
}

void JavaPanel::onSelectJava() {
    int row = m_systemJavaTable->currentRow();
    if (row < 0 || row >= m_foundJavas.size()) return;
    m_selectedJavaPath = m_foundJavas[row].path;
    m_selectedLabel->setText(m_selectedJavaPath);
    m_selectedLabel->setStyleSheet("color: #00d2a0; font-size: 12px; font-weight: bold;");
}

void JavaPanel::onCopyPath() {
    if (!m_selectedJavaPath.isEmpty()) QApplication::clipboard()->setText(m_selectedJavaPath);
}

void JavaPanel::refreshVersionMapping() {
    m_versionMapTable->setRowCount(0);
    int num = sizeof(kVersionMap) / sizeof(kVersionMap[0]);
    for (int i = 0; i < num; i++) {
        m_versionMapTable->insertRow(i);
        m_versionMapTable->setItem(i, 0, new QTableWidgetItem(kVersionMap[i].mcVersion));
        m_versionMapTable->setItem(i, 1, new QTableWidgetItem("Java " + QString::number(kVersionMap[i].requiredJava)));

        bool found = false;
        for (auto& j : m_foundJavas) {
            if (j.majorVersion >= kVersionMap[i].requiredJava) { found = true; break; }
        }
        auto* compatItem = new QTableWidgetItem(found ? TR("panel.java.yes") : TR("panel.java.no"));
        compatItem->setForeground(found ? QColor("#00d2a0") : QColor("#ff4757"));
        m_versionMapTable->setItem(i, 2, compatItem);
        m_versionMapTable->setRowHeight(i, 30);
    }
}

void JavaPanel::onDownloadJava() {
    QString verStr = m_versionCombo->currentText();
    int javaVer = 21;
    if (verStr.contains("8")) javaVer = 8;
    else if (verStr.contains("11")) javaVer = 11;
    else if (verStr.contains("17")) javaVer = 17;

    QString destDir = m_installDirEdit->text().trimmed();
    if (destDir.isEmpty()) { m_downloadStatusLabel->setText(TR("panel.java.noInstallDir")); m_downloadStatusLabel->setStyleSheet("color: #ff4757;"); return; }

    m_downloadBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_downloadStatusLabel->setText(TR("panel.java.downloading"));
    m_downloadStatusLabel->setStyleSheet("color: #facc15;");
    m_javaManager->downloadJava(javaVer, destDir);
}

void JavaPanel::onBrowseInstallDir() {
    QString dir = QFileDialog::getExistingDirectory(this, TR("panel.java.selectInstallDir"));
    if (!dir.isEmpty()) m_installDirEdit->setText(dir);
}

void JavaPanel::onDownloadProgress(int percent) {
    m_progressBar->setValue(percent);
    m_downloadStatusLabel->setText(TR("panel.java.downloadingProgress").arg(QString::number(percent)));
}

void JavaPanel::onDownloadFinished(const QString& javaPath) {
    m_progressBar->setValue(100);
    m_downloadStatusLabel->setText(TR("panel.java.downloadDone").arg(javaPath));
    m_downloadStatusLabel->setStyleSheet("color: #00d2a0; font-size: 12px; font-weight: bold;");
    m_downloadBtn->setEnabled(true);
    m_selectedJavaPath = javaPath;
    m_selectedLabel->setText(javaPath);
    m_selectedLabel->setStyleSheet("color: #00d2a0; font-size: 12px; font-weight: bold;");
    onScanSystem();
}

void JavaPanel::onDownloadError(const QString& err) {
    m_downloadStatusLabel->setText(err);
    m_downloadStatusLabel->setStyleSheet("color: #ff4757; font-size: 12px;");
    m_downloadBtn->setEnabled(true);
    m_progressBar->setVisible(false);
}
