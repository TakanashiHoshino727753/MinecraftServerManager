#include "ModManagerPanel.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QScrollBar>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QUrlQuery>

ModManagerPanel::ModManagerPanel(ConfigStore* store, QWidget* parent)
    : QWidget(parent), m_store(store)
{
    m_modManager = new ModManager(this);
    m_network = new QNetworkAccessManager(this);
    ui.setupUi(this);

    // ── Pointer aliases ──
    m_modTable        = ui.modTable;
    m_statusLabel     = ui.statusLabel;
    m_marketGroup     = ui.marketGroup;
    m_installFileBtn  = ui.installFileBtn;
    m_conflictCheckBtn= ui.conflictCheckBtn;
    m_scanBtn         = ui.scanBtn;

    // Modrinth widgets
    m_mrSearchInput   = ui.mrSearchInput;
    m_mrLoaderCombo   = ui.mrLoaderCombo;
    m_mrSearchBtn     = ui.mrSearchBtn;
    m_mrResultScroll  = ui.mrResultScroll;
    m_mrResultContent = ui.mrResultContent;
    m_mrPrevBtn       = ui.mrPrevBtn;
    m_mrNextBtn       = ui.mrNextBtn;
    m_mrPageLabel     = ui.mrPageLabel;

    // Setup result content layout
    m_mrResultLayout = new QVBoxLayout(m_mrResultContent);
    m_mrResultLayout->setSpacing(4);
    m_mrResultLayout->setContentsMargins(0, 0, 0, 0);
    m_mrResultLayout->addStretch();

    // Hide pagination initially
    m_mrPrevBtn->setVisible(false);
    m_mrNextBtn->setVisible(false);
    m_mrPageLabel->setVisible(false);

    // ── Connects ──
    connect(m_modManager, &ModManager::modsChanged, this, [this]() { updateModTable(); emit modsChanged(); });
    connect(m_modManager, &ModManager::error, this, [this](const QString& msg) {
        m_statusLabel->setText(msg); m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px; padding: 4px 0;");
    });
    connect(m_modManager, &ModManager::conflictDetected, this, [this](const QString& a, const QString& b) {
        m_statusLabel->setText(TR("panel.mod.conflict").arg(a, b));
        m_statusLabel->setStyleSheet("color: #f59e0b; font-size: 12px; padding: 4px 0;");
    });
    connect(m_installFileBtn, &QPushButton::clicked, this, &ModManagerPanel::onInstallFromFile);
    connect(m_conflictCheckBtn, &QPushButton::clicked, this, &ModManagerPanel::onConflictCheck);
    connect(m_scanBtn, &QPushButton::clicked, this, &ModManagerPanel::onScanMods);

    // Modrinth connects
    connect(m_mrSearchBtn, &QPushButton::clicked, this, &ModManagerPanel::onSearchModrinth);
    connect(m_mrSearchInput, &QLineEdit::returnPressed, this, &ModManagerPanel::onSearchModrinth);
    connect(m_mrPrevBtn, &QPushButton::clicked, this, &ModManagerPanel::onSearchPrev);
    connect(m_mrNextBtn, &QPushButton::clicked, this, &ModManagerPanel::onSearchNext);

    // ── Setup mod table ──
    m_modTable->setHorizontalHeaderLabels({TR("panel.mod.colName"), TR("panel.mod.colVersion"), TR("panel.mod.colLoader"), TR("panel.mod.colEnabled"), TR("panel.mod.colActions")});
    m_modTable->horizontalHeader()->setStretchLastSection(true);
    m_modTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_modTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_modTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_modTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    // ── i18n texts ──
    ui.titleLabel->setText(TR("panel.mod.title"));
    ui.installedGroup->setTitle(TR("panel.mod.installed"));
    m_marketGroup->setTitle(TR("panel.mod.market"));
    m_mrSearchBtn->setText(TR("panel.mod.search"));
    m_installFileBtn->setText(TR("panel.mod.installFile"));
    m_conflictCheckBtn->setText(TR("panel.mod.checkConflicts"));
    m_scanBtn->setText(TR("panel.mod.refresh"));

    // Show initial hint in results area
    QLabel* hint = new QLabel(TR("panel.mod.searchHint"));
    hint->setStyleSheet("color: #8888aa; font-size: 12px; padding: 30px;");
    hint->setAlignment(Qt::AlignCenter);
    hint->setObjectName("mrHintLabel");
    // Insert at beginning of layout (before stretch)
    m_mrResultLayout->insertWidget(0, hint);
}

void ModManagerPanel::loadServer(const QString& serverId) {
    m_serverId = serverId;
    m_hasServer = !serverId.isEmpty();
    if (m_hasServer) {
        ServerConfig* cfg = m_store->findServer(serverId);
        if (cfg) {
            m_serverPath = cfg->path;
            m_serverLoader = (cfg->type == "fabric") ? "fabric" : (cfg->type == "forge") ? "forge" : "vanilla";
            m_modManager->scan(m_serverPath, m_serverLoader);
        }
    }
    updateModTable();
    m_installFileBtn->setEnabled(m_hasServer);
    m_conflictCheckBtn->setEnabled(m_hasServer);
    m_scanBtn->setEnabled(m_hasServer);
}

void ModManagerPanel::refresh() {
    if (m_hasServer) m_modManager->scan(m_serverPath, m_serverLoader);
}

void ModManagerPanel::updateModTable() {
    m_modTable->setRowCount(0);
    if (!m_hasServer) return;
    auto mods = m_modManager->mods();
    for (int i = 0; i < mods.size(); i++) {
        m_modTable->insertRow(i);
        m_modTable->setItem(i, 0, new QTableWidgetItem(mods[i].name));
        m_modTable->setItem(i, 1, new QTableWidgetItem(mods[i].version));
        m_modTable->setItem(i, 2, new QTableWidgetItem(mods[i].loader));

        auto* cb = new QCheckBox;
        cb->setChecked(mods[i].enabled);
        cb->setStyleSheet("margin-left: 10px;");
        connect(cb, &QCheckBox::toggled, this, [this, i](bool checked) { onToggleMod(i); });
        m_modTable->setCellWidget(i, 3, cb);

        auto* btnWidget = new QWidget;
        auto* btnLayout = new QHBoxLayout(btnWidget);
        btnLayout->setContentsMargins(4, 2, 4, 2);
        btnLayout->setSpacing(6);
        auto* delBtn = new QPushButton(TR("panel.mod.remove"));
        delBtn->setFixedSize(60, 26);
        delBtn->setStyleSheet("QPushButton { background: rgba(255,71,87,0.15); border: 1px solid rgba(255,71,87,0.3); border-radius: 4px; color: #ff4757; font-size: 11px; } QPushButton:hover { background: rgba(255,71,87,0.3); }");
        connect(delBtn, &QPushButton::clicked, this, [this, i]() { onRemoveMod(i); });
        btnLayout->addWidget(delBtn);
        btnLayout->addStretch();
        m_modTable->setCellWidget(i, 4, btnWidget);
        m_modTable->setRowHeight(i, 36);
    }
}

void ModManagerPanel::onScanMods() { refresh(); }
void ModManagerPanel::onInstallFromFile() {
    if (!m_hasServer) return;
    QString path = QFileDialog::getOpenFileName(this, TR("panel.mod.selectJar"), QString(), "JAR files (*.jar)");
    if (!path.isEmpty()) { m_modManager->installMod(path); }
}
void ModManagerPanel::onConflictCheck() {
    auto conflicts = m_modManager->checkConflicts();
    if (conflicts.isEmpty()) {
        m_statusLabel->setText(TR("panel.mod.noConflicts"));
        m_statusLabel->setStyleSheet("color: #00d2a0; font-size: 12px; padding: 4px 0;");
    } else {
        QString msg = TR("panel.mod.conflictsFound") + "\n";
        for (auto& p : conflicts) msg += p.first.name + " \u2194 " + p.second.name + "\n";
        QMessageBox::warning(this, TR("panel.mod.conflictTitle"), msg);
    }
}
void ModManagerPanel::onToggleMod(int row) {
    auto cb = qobject_cast<QCheckBox*>(m_modTable->cellWidget(row, 3));
    if (!cb) return;
    auto mods = m_modManager->mods();
    if (row < mods.size()) m_modManager->setModEnabled(mods[row].fileName, cb->isChecked());
}
void ModManagerPanel::onRemoveMod(int row) {
    auto mods = m_modManager->mods();
    if (row >= mods.size()) return;
    if (QMessageBox::question(this, TR("panel.mod.confirmRemove"), TR("panel.mod.removeMsg").arg(mods[row].name)) == QMessageBox::Yes) {
        m_modManager->removeMod(mods[row].fileName);
    }
}
void ModManagerPanel::onInstallMarketMod(int) {
    m_statusLabel->setText(TR("panel.mod.marketNote"));
    m_statusLabel->setStyleSheet("color: #8888aa; font-size: 12px; padding: 4px 0;");
}

// ═══════════════════════════════════════════════════════════════════════
//  Modrinth Search Implementation
// ═══════════════════════════════════════════════════════════════════════

void ModManagerPanel::onSearchModrinth() {
    m_mrQuery = m_mrSearchInput->text().trimmed();
    if (m_mrQuery.isEmpty()) return;

    int idx = m_mrLoaderCombo->currentIndex();
    QStringList loaderMap = {"", "fabric", "forge", "quilt", "neoforge"};
    m_mrLoader = (idx >= 0 && idx < loaderMap.size()) ? loaderMap[idx] : "";
    m_mrOffset = 0;

    // Build search URL with facets
    QStringList facets;
    facets << "[\"project_type:mod\"]";

    if (!m_mrLoader.isEmpty()) {
        QString cat = m_mrLoader.toLower();
        if (cat == "fabric") facets << "[\"categories:fabric\"]";
        else if (cat == "forge") facets << "[\"categories:forge\"]";
        else if (cat == "neoforge") facets << "[\"categories:neoforge\"]";
        else if (cat == "quilt") facets << "[\"categories:quilt\"]";
    }

    QString facetsStr = "[" + facets.join(",") + "]";

    QString url = "https://api.modrinth.com/v2/search?query="
        + QUrl::toPercentEncoding(m_mrQuery)
        + "&facets=" + QUrl::toPercentEncoding(facetsStr)
        + "&offset=" + QString::number(m_mrOffset)
        + "&limit=20";

    // Show loading
    QLayoutItem* item;
    while ((item = m_mrResultLayout->takeAt(0))) {
        if (item->widget()) {
            if (item->widget()->objectName() != "mrHintLabel") delete item->widget();
        }
        delete item;
    }
    m_mrResultLayout->addStretch();

    QLabel* loading = new QLabel(TR("panel.mod.searching"));
    loading->setStyleSheet("color: #8888aa; font-size: 12px; padding: 30px;");
    loading->setAlignment(Qt::AlignCenter);
    loading->setObjectName("mrLoading");
    m_mrResultLayout->insertWidget(0, loading);

    m_statusLabel->setText(TR("panel.mod.searching"));
    m_statusLabel->setStyleSheet("color: #a29bfe; font-size: 12px; padding: 4px 0;");

    // Make API request
    QNetworkRequest req{QUrl(url)};
    req.setRawHeader("User-Agent", "MCServerManager/2.0 (mod-search)");
    req.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_statusLabel->setText(TR("panel.mod.networkError") + ": " + reply->errorString());
            m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px; padding: 4px 0;");
            return;
        }
        QByteArray data = reply->readAll();
        QJsonObject resp = QJsonDocument::fromJson(data).object();
        QJsonArray hits = resp["hits"].toArray();
        int total = resp["total_hits"].toInt();

        QJsonArray results;
        for (const auto& hit : hits) {
            QJsonObject h = hit.toObject();
            QJsonObject mod;
            mod["projectId"]  = h["project_id"];
            mod["name"]       = h["title"];
            mod["description"]= h["description"];
            mod["iconUrl"]    = h["icon_url"];
            mod["downloads"]  = h["downloads"];
            mod["categories"] = h["categories"];
            mod["versions"]   = h["versions"];
            mod["author"]     = h["author"];
            results.append(mod);
        }
        updateModrinthResults(results, total, m_mrOffset, 20);
    });
}

void ModManagerPanel::onSearchPrev() {
    if (m_mrOffset > 0) {
        m_mrOffset = qMax(0, m_mrOffset - 20);
        onSearchModrinth();
    }
}

void ModManagerPanel::onSearchNext() {
    if (m_mrOffset + 20 < m_mrTotal) {
        m_mrOffset += 20;
        onSearchModrinth();
    }
}

void ModManagerPanel::updateModrinthResults(const QJsonArray& results, int total, int offset, int limit) {
    // Clear existing results
    QLayoutItem* item;
    while ((item = m_mrResultLayout->takeAt(0))) {
        if (item->widget()) {
            if (item->widget()->objectName() != "mrHintLabel") delete item->widget();
        }
        delete item;
    }
    m_mrResultLayout->addStretch();

    if (results.isEmpty()) {
        QLabel* nope = new QLabel(TR("panel.mod.noResults"));
        nope->setStyleSheet("color: #8888aa; font-size: 12px; padding: 30px;");
        nope->setAlignment(Qt::AlignCenter);
        m_mrResultLayout->insertWidget(0, nope);
        m_mrPrevBtn->setVisible(false);
        m_mrNextBtn->setVisible(false);
        m_mrPageLabel->setVisible(false);
        m_statusLabel->setText(TR("panel.mod.noResults"));
        m_statusLabel->setStyleSheet("color: #8888aa; font-size: 12px; padding: 4px 0;");
        return;
    }

    m_mrTotal = total;

    for (int i = 0; i < results.size(); i++) {
        QJsonObject mod = results[i].toObject();
        QString projectId = mod["projectId"].toString();
        QString name = mod["name"].toString();
        QString desc = mod["description"].toString();
        QString iconUrl = mod["iconUrl"].toString();
        QString author = mod["author"].toString();
        QJsonArray cats = mod["categories"].toArray();
        int downloads = mod["downloads"].toInt();

        // Truncate description
        if (desc.length() > 120) desc = desc.left(117) + "...";

        // Build category string
        QStringList catNames;
        for (const auto& c : cats) catNames << c.toString();
        QString catStr = catNames.join(", ");

        // Build card
        auto* card = new QWidget;
        card->setStyleSheet("QWidget { background: rgba(18,18,30,0.8); border: 1px solid #2a2a4a; border-radius: 8px; } QWidget:hover { border-color: #a29bfe; }");
        card->setCursor(Qt::PointingHandCursor);
        auto* crd = new QHBoxLayout(card);
        crd->setContentsMargins(10, 8, 10, 8);
        crd->setSpacing(10);

        // Icon placeholder
        auto* iconLbl = new QLabel;
        iconLbl->setFixedSize(40, 40);
        iconLbl->setAlignment(Qt::AlignCenter);
        if (!iconUrl.isEmpty()) {
            iconLbl->setText("\xF0\x9F\x93\xA6"); // package emoji as placeholder
        } else {
            iconLbl->setText("\xF0\x9F\x93\xA6");
        }
        iconLbl->setStyleSheet("font-size: 20px; background: rgba(162,155,254,0.15); border-radius: 8px; border: none;");
        crd->addWidget(iconLbl);

        // Info
        auto* infoLayout = new QVBoxLayout;
        infoLayout->setSpacing(2);
        auto* nameLabel = new QLabel(name);
        nameLabel->setStyleSheet("font-weight: 600; color: #e0e0f0; font-size: 13px; border: none;");
        auto* descLabel = new QLabel(desc);
        descLabel->setStyleSheet("color: #8888aa; font-size: 11px; border: none;");
        descLabel->setWordWrap(true);
        auto* metaLabel = new QLabel((author.isEmpty() ? "" : author + " \u00B7 ") + QString::number(downloads) + " DL" + (catStr.isEmpty() ? "" : " \u00B7 " + catStr));
        metaLabel->setStyleSheet("color: #666688; font-size: 10px; border: none;");
        infoLayout->addWidget(nameLabel);
        infoLayout->addWidget(descLabel);
        infoLayout->addWidget(metaLabel);
        crd->addLayout(infoLayout, 1);

        // Install button
        auto* installBtn = new QPushButton(TR("panel.mod.install"));
        installBtn->setFixedSize(70, 28);
        installBtn->setStyleSheet("QPushButton { background: rgba(0,210,160,0.15); border: 1px solid rgba(0,210,160,0.3); border-radius: 6px; color: #00d2a0; font-size: 11px; font-weight: bold; } QPushButton:hover { background: rgba(0,210,160,0.3); }");
        if (!m_hasServer) installBtn->setEnabled(false);
        connect(installBtn, &QPushButton::clicked, this, [this, projectId, name, iconUrl, desc]() {
            onSelectMod(projectId, name, iconUrl, desc);
        });
        crd->addWidget(installBtn);

        // Insert before stretch
        m_mrResultLayout->insertWidget(m_mrResultLayout->count() - 1, card);
    }

    // Update pagination
    int totalPages = (total + limit - 1) / limit;
    int curPage = offset / limit + 1;
    m_mrPageLabel->setText(TR("panel.mod.page").arg(curPage).arg(totalPages).arg(total));
    m_mrPrevBtn->setEnabled(offset > 0);
    m_mrNextBtn->setEnabled(offset + limit < total);
    m_mrPrevBtn->setVisible(true);
    m_mrNextBtn->setVisible(true);
    m_mrPageLabel->setVisible(true);

    m_statusLabel->setText(TR("panel.mod.searchResults").arg(total));
    m_statusLabel->setStyleSheet("color: #8888aa; font-size: 12px; padding: 4px 0;");
}

// ═══════════════════════════════════════════════════════════════════════
//  Modrinth Version Picker & Download
// ═══════════════════════════════════════════════════════════════════════

void ModManagerPanel::onSelectMod(const QString& projectId, const QString& modName, const QString& iconUrl, const QString& desc) {
    Q_UNUSED(iconUrl)
    Q_UNUSED(desc)

    if (!m_hasServer) {
        QMessageBox::information(this, TR("panel.mod.selectServer"), TR("panel.mod.selectServerMsg"));
        return;
    }

    m_mrProjectId = projectId;
    m_mrModName = modName;

    // Create version dialog
    if (!m_versionDialog) {
        m_versionDialog = new QDialog(this);
        m_versionDialog->setWindowTitle(TR("panel.mod.selectVersion"));
        m_versionDialog->setMinimumSize(480, 360);
        m_versionDialog->setStyleSheet("QDialog { background: #12121e; border: 1px solid #2a2a4a; border-radius: 12px; }");

        auto* dlgLayout = new QVBoxLayout(m_versionDialog);
        dlgLayout->setContentsMargins(16, 16, 16, 16);
        dlgLayout->setSpacing(10);

        // Title
        auto* titleRow = new QHBoxLayout;
        m_versionTitle = new QLabel;
        m_versionTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #a29bfe;");
        titleRow->addWidget(m_versionTitle, 1);

        m_versionCloseBtn = new QPushButton("\u2715");
        m_versionCloseBtn->setFixedSize(28, 28);
        m_versionCloseBtn->setStyleSheet("QPushButton { background: transparent; border: none; color: #8888aa; font-size: 14px; } QPushButton:hover { color: #ff4757; }");
        connect(m_versionCloseBtn, &QPushButton::clicked, m_versionDialog, &QDialog::reject);
        titleRow->addWidget(m_versionCloseBtn);
        dlgLayout->addLayout(titleRow);

        // Loading label
        m_versionLoading = new QLabel(TR("panel.mod.loadingVersions"));
        m_versionLoading->setStyleSheet("color: #8888aa; font-size: 12px; padding: 20px;");
        m_versionLoading->setAlignment(Qt::AlignCenter);
        dlgLayout->addWidget(m_versionLoading);

        // Version list
        m_versionList = new QListWidget;
        m_versionList->setStyleSheet(
            "QListWidget { background: #0a0a16; border: 1px solid #2a2a4a; border-radius: 8px; color: #e0e0f0; }"
            "QListWidget::item { padding: 8px 12px; border-bottom: 1px solid #1a1a2e; }"
            "QListWidget::item:hover { background: rgba(162,155,254,0.1); }"
            "QListWidget::item:selected { background: rgba(162,155,254,0.2); }"
        );
        dlgLayout->addWidget(m_versionList);
    }

    m_versionTitle->setText(modName);
    m_versionLoading->setVisible(true);
    m_versionList->clear();
    m_versionLoading->setText(TR("panel.mod.loadingVersions"));

    // Fetch versions from Modrinth API
    QString verUrl = "https://api.modrinth.com/v2/project/" + projectId + "/version";
    QNetworkRequest req{QUrl(verUrl)};
    req.setRawHeader("User-Agent", "MCServerManager/2.0 (mod-version)");
    req.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_versionLoading->setVisible(false);

        if (reply->error() != QNetworkReply::NoError) {
            QListWidgetItem* errItem = new QListWidgetItem(TR("panel.mod.networkError"));
            errItem->setForeground(QColor("#ff4757"));
            m_versionList->addItem(errItem);
            return;
        }

        QByteArray data = reply->readAll();
        QJsonArray versions = QJsonDocument::fromJson(data).array();

        if (versions.isEmpty()) {
            QListWidgetItem* noItem = new QListWidgetItem(TR("panel.mod.noVersions"));
            noItem->setForeground(QColor("#8888aa"));
            m_versionList->addItem(noItem);
            return;
        }

        for (const auto& v : versions) {
            QJsonObject vo = v.toObject();
            QString versionId = vo["id"].toString();
            QString verName = vo["name"].toString();
            QString verNum = vo["version_number"].toString();
            QJsonArray gameVersions = vo["game_versions"].toArray();
            QJsonArray loaders = vo["loaders"].toArray();
            QString verType = vo["version_type"].toString();

            QStringList mcVerList;
            for (const auto& gv : gameVersions) mcVerList << gv.toString();
            QStringList loaderList;
            for (const auto& l : loaders) loaderList << l.toString();

            QString displayName = verName.isEmpty() ? verNum : verName;
            QString details = "MC " + mcVerList.join(", ") + "  \u00B7  " + loaderList.join(", ");
            if (!verType.isEmpty()) details += "  \u00B7  " + verType;

            auto* item = new QListWidgetItem;
            item->setText(displayName + "\n" + details);
            item->setData(Qt::UserRole, versionId);
            item->setData(Qt::UserRole + 1, displayName);
            item->setToolTip(details);

            QFont f = item->font();
            f.setPixelSize(13);
            item->setFont(f);

            m_versionList->addItem(item);
        }

        m_versionList->setMinimumHeight(qMin(versions.size() * 54, 300));
    });

    m_versionList->setMinimumHeight(200);

    // Handle version selection
    if (m_versionDialog->exec() == QDialog::Accepted) {
        auto* selItem = m_versionList->currentItem();
        if (selItem) {
            QString versionId = selItem->data(Qt::UserRole).toString();
            QString versionName = selItem->data(Qt::UserRole + 1).toString();
            onInstallVersion(versionId, versionName);
        }
    }
}

void ModManagerPanel::onInstallVersion(const QString& versionId, const QString& versionName) {
    if (!m_hasServer) return;

    m_statusLabel->setText(TR("panel.mod.downloadingMod").arg(m_mrModName));
    m_statusLabel->setStyleSheet("color: #a29bfe; font-size: 12px; padding: 4px 0;");

    // Step 1: Resolve version info from Modrinth API to get download URL
    QString verUrl = "https://api.modrinth.com/v2/version/" + versionId;
    QNetworkRequest req{QUrl(verUrl)};
    req.setRawHeader("User-Agent", "MCServerManager/2.0 (mod-download)");
    req.setRawHeader("Accept", "application/json");

    QNetworkReply* verReply = m_network->get(req);
    connect(verReply, &QNetworkReply::finished, this, [this, verReply, versionId, versionName]() {
        verReply->deleteLater();

        if (verReply->error() != QNetworkReply::NoError) {
            m_statusLabel->setText(TR("panel.mod.networkError") + ": " + verReply->errorString());
            m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px; padding: 4px 0;");
            return;
        }

        QByteArray data = verReply->readAll();
        QJsonObject verInfo = QJsonDocument::fromJson(data).object();
        QJsonArray files = verInfo["files"].toArray();

        if (files.isEmpty()) {
            m_statusLabel->setText(TR("panel.mod.noDownload"));
            m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px; padding: 4px 0;");
            return;
        }

        // Pick primary file or first available
        QString dlUrl;
        QString fileName;
        for (const auto& f : files) {
            QJsonObject fo = f.toObject();
            if (fo["primary"].toBool()) {
                dlUrl = fo["url"].toString();
                fileName = fo["filename"].toString();
                break;
            }
        }
        if (dlUrl.isEmpty()) {
            QJsonObject fo = files.first().toObject();
            dlUrl = fo["url"].toString();
            fileName = fo["filename"].toString();
        }

        if (dlUrl.isEmpty()) {
            m_statusLabel->setText(TR("panel.mod.noDownload"));
            m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px; padding: 4px 0;");
            return;
        }

        // Step 2: Download the actual mod file
        QNetworkRequest dlReq{QUrl(dlUrl)};
        dlReq.setRawHeader("User-Agent", "MCServerManager/2.0 (mod-download)");

        QNetworkReply* dlReply = m_network->get(dlReq);
        connect(dlReply, &QNetworkReply::finished, this, [this, dlReply, fileName, versionName]() {
            dlReply->deleteLater();

            if (dlReply->error() != QNetworkReply::NoError) {
                m_statusLabel->setText(TR("panel.mod.downloadFailed") + ": " + dlReply->errorString());
                m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px; padding: 4px 0;");
                return;
            }

            QByteArray modData = dlReply->readAll();
            if (modData.isEmpty()) {
                m_statusLabel->setText(TR("panel.mod.emptyFile"));
                m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px; padding: 4px 0;");
                return;
            }

            // Ensure mods folder exists
            QString modsPath = m_serverPath + "/mods";
            QDir().mkpath(modsPath);

            // Determine filename
            QString finalName = fileName;
            if (finalName.isEmpty()) {
                finalName = m_mrModName + "-" + versionName + ".jar";
            }

            QString targetPath = modsPath + "/" + finalName;
            QFile outFile(targetPath);
            if (!outFile.open(QIODevice::WriteOnly)) {
                m_statusLabel->setText(TR("panel.mod.writeFailed"));
                m_statusLabel->setStyleSheet("color: #ff4757; font-size: 12px; padding: 4px 0;");
                return;
            }
            outFile.write(modData);
            outFile.close();

            // Re-scan mods
            m_modManager->scan(m_serverPath, m_serverLoader);

            m_statusLabel->setText(TR("panel.mod.installedMsg").arg(m_mrModName));
            m_statusLabel->setStyleSheet("color: #00d2a0; font-size: 12px; padding: 4px 0;");

            emit modsChanged();
        });
    });
}
