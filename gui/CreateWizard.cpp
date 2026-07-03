#include "CreateWizard.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QScrollArea>
#include <QFrame>
#include <QUuid>
#include <QDir>
#include <QMessageBox>
#include <QStyle>

// ── Custom QStackedWidget: sizeHint returns current page height, not max of all pages ──
class FittedStackedWidget : public QStackedWidget {
public:
    using QStackedWidget::QStackedWidget;
    QSize sizeHint() const override {
        QWidget* w = currentWidget();
        return w ? w->sizeHint() : QStackedWidget::sizeHint();
    }
    QSize minimumSizeHint() const override {
        QWidget* w = currentWidget();
        return w ? w->minimumSizeHint() : QStackedWidget::minimumSizeHint();
    }
};

CreateWizard::CreateWizard(ConfigStore* store, DownloadManager* dl, QWidget* parent)
    : QWidget(parent), m_store(store), m_dl(dl)
{
    ui.setupUi(this);

    // Replace the default QStackedWidget with our auto-sizing variant
    {
        QStackedWidget* oldStack = ui.stepStack;
        auto* newStack = new FittedStackedWidget;
        // Transfer all pages from old to new (removeWidget removes without deleting)
        QVector<QWidget*> pages;
        while (oldStack->count() > 0) {
            QWidget* w = oldStack->widget(0);
            oldStack->removeWidget(w);
            pages.append(w);
        }
        for (auto* w : pages) newStack->addWidget(w);
        newStack->setCurrentIndex(oldStack->currentIndex());
        // Swap in the scroll area
        ui.scrollArea->takeWidget();
        ui.stepStack = newStack;
        ui.scrollArea->setWidget(newStack);
        delete oldStack;
    }

    // Apply language-aware texts (override .ui defaults)
    auto* lm = LanguageManager::instance();
    ui.title->setText(lm->t("wizard.title"));
    ui.subtitle->setText(lm->t("wizard.subtitle"));

    // Style titles
    ui.title->setStyleSheet("font-size:24px; font-weight:bold; color:#a29bfe;");
    ui.subtitle->setStyleSheet("color:#8888aa; font-size:13px; margin-top:4px;");

    // Error label
    ui.errorLabel->setStyleSheet("color:#ff4757; font-size:12px; padding:4px 0;");
    ui.errorLabel->hide();

    // Progress area
    ui.progressArea->hide();

    // Step indicator
    ui.stepIndicatorLayout->addWidget(createStepIndicator());
    ui.scrollArea->setWidget(ui.stepStack);
    setupSteps();

    // Navigation buttons
    ui.cancelBtn->setText(lm->t("wizard.cancelBtn"));
    ui.cancelBtn->setObjectName("ghostBtn");
    ui.prevBtn->setText(lm->t("wizard.prevBtn"));
    ui.prevBtn->setObjectName("ghostBtn");
    ui.nextBtn->setObjectName("primaryBtn");

    connect(ui.cancelBtn, &QPushButton::clicked, this, [this]() { emit created(); });
    connect(ui.prevBtn, &QPushButton::clicked, this, &CreateWizard::prevStep);
    connect(ui.nextBtn, &QPushButton::clicked, this, &CreateWizard::nextStep);

    // Download signals
    connect(m_dl, &DownloadManager::versionsReady, this, &CreateWizard::onVersionsReady);
    connect(m_dl, &DownloadManager::buildsReady, this, &CreateWizard::onPaperBuildsReady);
    connect(m_dl, &DownloadManager::downloadProgress, this, [this](const QString& s, int p) {
        ui.progressLabel->setText(s);
        ui.progress->setValue(p);
    });

    updateNavButtons();
    updateStepIndicator(0);
}

QWidget* CreateWizard::createStepIndicator() {
    QWidget* indicator = new QWidget;
    QHBoxLayout* il = new QHBoxLayout(indicator);
    il->setContentsMargins(0, 0, 0, 0);
    il->setSpacing(0);

    auto* lm = LanguageManager::instance();
    const QString stepNames[] = {lm->t("wizard.step1Label"), lm->t("wizard.step2Label"), lm->t("wizard.step3Label"), lm->t("wizard.step4Label")};
    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            m_stepLines[i-1] = new QLabel;
            m_stepLines[i-1]->setObjectName("stepLine");
            m_stepLines[i-1]->setFixedHeight(2);
            m_stepLines[i-1]->setMinimumWidth(60);
            il->addWidget(m_stepLines[i-1], 0, Qt::AlignVCenter);
        }
        QWidget* step = new QWidget;
        step->setFixedSize(80, 60);
        QVBoxLayout* sl = new QVBoxLayout(step);
        sl->setContentsMargins(0, 0, 0, 0);
        sl->setSpacing(6);
        m_stepCircles[i] = new QLabel(QString::number(i + 1));
        m_stepCircles[i]->setObjectName("stepCirclePending");
        m_stepCircles[i]->setFixedSize(28, 28);
        m_stepCircles[i]->setAlignment(Qt::AlignCenter);
        m_stepLabels[i] = new QLabel(stepNames[i]);
        m_stepLabels[i]->setObjectName("stepLabelPending");
        m_stepLabels[i]->setAlignment(Qt::AlignCenter);
        sl->addWidget(m_stepCircles[i], 0, Qt::AlignCenter);
        sl->addWidget(m_stepLabels[i], 0, Qt::AlignCenter);
        il->addWidget(step, 0, Qt::AlignCenter);
    }
    il->addStretch();
    return indicator;
}

void CreateWizard::updateStepIndicator(int step) {
    for (int i = 0; i < 4; i++) {
        if (i < step) {
            m_stepCircles[i]->setText(QString::fromUtf8("\u2713"));
            m_stepCircles[i]->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #00d2a0, stop:1 #00e5b0); color: white; font-weight: bold; border-radius: 14px; min-width:28px; max-width:28px; min-height:28px; max-height:28px;");
            m_stepLabels[i]->setStyleSheet("color:#00d2a0; font-size:12px; font-weight:bold; qproperty-alignment:AlignCenter;");
        } else if (i == step) {
            m_stepCircles[i]->setText(QString::number(i + 1));
            m_stepCircles[i]->setStyleSheet("background: #6c5ce7; color: white; font-weight: bold; border-radius: 14px; min-width:28px; max-width:28px; min-height:28px; max-height:28px; box-shadow: 0 0 12px rgba(108,92,231,0.4);");
            m_stepLabels[i]->setStyleSheet("color:#a29bfe; font-size:12px; font-weight:bold; qproperty-alignment:AlignCenter;");
        } else {
            m_stepCircles[i]->setText(QString::number(i + 1));
            m_stepCircles[i]->setStyleSheet("background: #1a1a2e; color: #555577; border: 1px solid #2a2a4a; border-radius: 14px; min-width:28px; max-width:28px; min-height:28px; max-height:28px;");
            m_stepLabels[i]->setStyleSheet("color:#555577; font-size:12px; qproperty-alignment:AlignCenter;");
        }
        if (i < 3) {
            m_stepLines[i]->setStyleSheet(i < step
                ? "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #00d2a0, stop:1 #6c5ce7); min-height:2px; max-height:2px;"
                : "background: #2a2a4a; min-height:2px; max-height:2px;");
        }
    }
}

void CreateWizard::reset() {
    m_step = 0; m_type = "paper"; m_version.clear(); m_build.clear();
    m_serverName.clear(); m_serverPath.clear(); m_javaPath = "java";
    m_minRam = 1024; m_maxRam = 4096; m_eulaAccepted = false;
    ui.errorLabel->hide(); ui.progressArea->hide();
    m_nameEdit->clear(); m_pathEdit->clear(); m_javaEdit->setText(TR("detail.javaDefault"));
    m_eulaCheck->setChecked(false);
    ui.stepStack->setCurrentIndex(0);
    fitStackToCurrent();
    updateStep1Selection();
    updateNavButtons();
    updateStepIndicator(0);
}

void CreateWizard::updateStep1Selection() {
    if (m_typeCards.isEmpty()) return;
    for (auto* c : m_typeCards) {
        bool sel = c->property("cardType").toString() == m_type;
        QString accent = c->property("cardAccent").toString();
        c->setStyleSheet(QString(
            "QPushButton { background: #12121e; border: %1; border-radius: 16px; text-align: left; padding: 16px; }"
            "QPushButton:hover { border-color: %2; }"
        ).arg(sel ? "2px solid " + accent : "1px solid rgba(42,42,74,0.5)", accent));
    }
}

void CreateWizard::setupSteps() {
    // Step pages use layout from .ui — we populate them here
    QWidget* p1 = ui.stepStack->widget(0);
    QVBoxLayout* l1 = new QVBoxLayout(p1); l1->setContentsMargins(0,8,0,8); l1->setSpacing(12);
    l1->addWidget(createStep1());
    l1->addStretch();

    QWidget* p2 = ui.stepStack->widget(1);
    QVBoxLayout* l2 = new QVBoxLayout(p2); l2->setContentsMargins(0,8,0,8); l2->setSpacing(12);
    l2->addWidget(createStep2());
    l2->addStretch();

    QWidget* p3 = ui.stepStack->widget(2);
    QVBoxLayout* l3 = new QVBoxLayout(p3); l3->setContentsMargins(0,8,0,8); l3->setSpacing(14);
    l3->addWidget(createStep3());

    QWidget* p4 = ui.stepStack->widget(3);
    QVBoxLayout* l4 = new QVBoxLayout(p4); l4->setContentsMargins(0,8,0,8); l4->setSpacing(12);
    l4->addWidget(createStep4());
}

// Step 1: Server Type
QWidget* CreateWizard::createStep1() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* lm = LanguageManager::instance();
    QLabel* label = new QLabel(lm->t("wizard.step1Title"));
    label->setStyleSheet("font-size:17px; font-weight:bold; color:#e0e0f0;");
    layout->addWidget(label);
    QLabel* hint = new QLabel(lm->t("wizard.step1Hint"));
    hint->setStyleSheet("color:#8888aa; font-size:12px;");
    layout->addWidget(hint);
    layout->addSpacing(8);

    auto* cardRow = new QHBoxLayout; cardRow->setSpacing(0);
    cardRow->setContentsMargins(10, 0, 10, 0);

    struct CardInfo { QString id; QString name; QString desc; QString features; QString color; };
    QVector<CardInfo> types = {
        {"vanilla", lm->t("wizard.vanillaName"), lm->t("wizard.vanillaDesc"), lm->t("wizard.vanillaFeat"), "#facc15"},
        {"paper",   lm->t("wizard.paperName"),   lm->t("wizard.paperDesc"),   lm->t("wizard.paperFeat"),   "#60a5fa"},
        {"fabric",  lm->t("wizard.fabricName"),  lm->t("wizard.fabricDesc"),  lm->t("wizard.fabricFeat"),  "#22d3ee"},
    };

    QVector<QPushButton*> allCards;
    for (const auto& t : types) {
        auto* card = new QPushButton;
        card->setCursor(Qt::PointingHandCursor);
        card->setMinimumWidth(180); card->setMinimumHeight(150); card->setMaximumWidth(220);
        bool selected = (t.id == m_type);
        card->setStyleSheet(QString(
            "QPushButton { background: #12121e; border: %1; border-radius: 16px; text-align: left; padding: 16px; }"
            "QPushButton:hover { border-color: %2; }"
        ).arg(selected ? "2px solid " + t.color : "1px solid rgba(42,42,74,0.5)", t.color));

        auto* inner = new QWidget;
        inner->setStyleSheet("background:transparent;");
        auto* il = new QVBoxLayout(inner);
        il->setContentsMargins(0,0,0,0); il->setSpacing(4);
        QLabel* typeName = new QLabel(t.name);
        typeName->setStyleSheet(QString("font-size:16px; font-weight:bold; color:%1; background:transparent;").arg(t.color));
        QLabel* typeDesc = new QLabel(t.desc);
        typeDesc->setStyleSheet("font-size:13px; color:#8888aa; background:transparent;");
        QLabel* typeFeat = new QLabel(t.features);
        typeFeat->setStyleSheet("font-size:11px; color:#5a5a7a; background:transparent;");
        il->addWidget(typeName); il->addWidget(typeDesc); il->addWidget(typeFeat); il->addStretch();

        auto* ol = new QVBoxLayout(card); ol->setContentsMargins(10,10,12,10); ol->addWidget(inner);
        card->setProperty("cardType", t.id);
        card->setProperty("cardAccent", t.color);
        allCards.append(card);
    }

    for (int i = 0; i < allCards.size(); i++) {
        if (i == 0) cardRow->insertStretch(0);
        cardRow->addWidget(allCards[i]);
        if (i < allCards.size() - 1) cardRow->addSpacing(5);
    }
    cardRow->addStretch();

    for (auto* card : allCards) {
        QString typeId = card->property("cardType").toString();
        connect(card, &QPushButton::clicked, this, [this, allCards, typeId]() {
            m_type = typeId;
            for (auto* c : allCards) {
                bool sel = c->property("cardType").toString() == typeId;
                QString accent = c->property("cardAccent").toString();
                c->setStyleSheet(QString(
                    "QPushButton { background: #12121e; border: %1; border-radius: 16px; text-align: left; padding: 16px; }"
                    "QPushButton:hover { border-color: %2; }"
                ).arg(sel ? "2px solid " + accent : "1px solid rgba(42,42,74,0.5)", accent));
            }
        });
    }
    m_typeCards = allCards;
    layout->addLayout(cardRow);
    return page;
}

// Step 2: Version
QWidget* CreateWizard::createStep2() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* lm = LanguageManager::instance();
    QLabel* label = new QLabel(lm->t("wizard.step2Title"));
    label->setStyleSheet("font-size:17px; font-weight:bold; color:#e0e0f0;");
    layout->addWidget(label);
    QLabel* hint = new QLabel(lm->t("wizard.step2Hint"));
    hint->setStyleSheet("color:#8888aa; font-size:12px;");
    layout->addWidget(hint); layout->addSpacing(12);

    QLabel* vLabel = new QLabel(lm->t("wizard.versionLabel"));
    vLabel->setStyleSheet("color:#8888aa; font-size:13px; font-weight:500;");
    layout->addWidget(vLabel);
    m_versionCombo = new QComboBox;
    m_versionCombo->setMinimumHeight(40);
    m_versionCombo->setStyleSheet("font-size:14px;");
    layout->addWidget(m_versionCombo);

    m_buildLabel = new QLabel(lm->t("wizard.buildLabel"));
    m_buildLabel->setStyleSheet("color:#8888aa; font-size:13px; font-weight:500;");
    layout->addWidget(m_buildLabel);
    m_buildCombo = new QComboBox;
    m_buildCombo->setMinimumHeight(40);
    m_buildCombo->setStyleSheet("font-size:14px;");
    layout->addWidget(m_buildCombo);

    m_buildLabel->setVisible(false);
    m_buildCombo->setVisible(false);

    connect(m_versionCombo, &QComboBox::currentTextChanged, this, [this](const QString& v) {
        m_version = v;
        if (m_type == "paper") m_dl->fetchPaperBuilds(v);
    });
    connect(m_buildCombo, &QComboBox::currentTextChanged, this, [this](const QString& b) { m_build = b; });
    return page;
}

// Step 3: Configuration
QWidget* CreateWizard::createStep3() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto* lm = LanguageManager::instance();
    QLabel* label = new QLabel(lm->t("wizard.step3Title"));
    label->setStyleSheet("font-size:17px; font-weight:bold; color:#e0e0f0;");
    layout->addWidget(label);

    auto addF = [&](const QString& lbl) {
        QLabel* l = new QLabel(lbl); l->setStyleSheet("color:#8888aa; font-size:13px; font-weight:500;");
        layout->addWidget(l);
    };

    addF(lm->t("wizard.name"));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText(lm->t("wizard.namePlaceholder"));
    m_nameEdit->setMinimumHeight(40); m_nameEdit->setStyleSheet("font-size:14px;");
    layout->addWidget(m_nameEdit);

    addF(lm->t("wizard.path"));
    auto* pathRow = new QHBoxLayout; pathRow->setSpacing(8);
    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText(lm->t("wizard.pathPlaceholder"));
    m_pathEdit->setReadOnly(true); m_pathEdit->setMinimumHeight(40); m_pathEdit->setStyleSheet("font-size:14px;");
    QPushButton* browsePath = new QPushButton(lm->t("wizard.browse"));
    browsePath->setFixedWidth(80); browsePath->setMinimumHeight(40); browsePath->setStyleSheet("font-size:13px;");
    connect(browsePath, &QPushButton::clicked, this, &CreateWizard::selectPath);
    pathRow->addWidget(m_pathEdit, 1); pathRow->addWidget(browsePath);
    layout->addLayout(pathRow);

    QGroupBox* memGroup = new QGroupBox(lm->t("wizard.memoryGroup"));
    memGroup->setStyleSheet("QGroupBox { border: 1px solid #2a2a4a; border-radius: 10px; margin-top: 12px; padding-top: 20px; font-weight: bold; color: #a29bfe; } QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 6px; }");
    auto* memLayout = new QHBoxLayout(memGroup); memLayout->setSpacing(12);
    memLayout->addWidget(new QLabel(lm->t("wizard.minLabel")));
    m_minRamCombo = new QComboBox;
    for (int mb : {512, 1024, 2048, 4096, 6144, 8192, 12288, 16384}) {
        m_minRamCombo->addItem(lm->t("wizard.memUnit").arg(mb), mb);
        if (mb == 1024) m_minRamCombo->setCurrentIndex(m_minRamCombo->count() - 1);
    }
    memLayout->addWidget(m_minRamCombo, 1);
    memLayout->addWidget(new QLabel(lm->t("wizard.maxLabel")));
    m_maxRamCombo = new QComboBox;
    for (int mb : {1024, 2048, 4096, 6144, 8192, 12288, 16384}) {
        m_maxRamCombo->addItem(lm->t("wizard.memUnit").arg(mb), mb);
        if (mb == 4096) m_maxRamCombo->setCurrentIndex(m_maxRamCombo->count() - 1);
    }
    memLayout->addWidget(m_maxRamCombo, 1);
    layout->addWidget(memGroup);

    addF(lm->t("wizard.javaLabel"));
    auto* javaRow = new QHBoxLayout; javaRow->setSpacing(8);
    m_javaEdit = new QLineEdit(TR("detail.javaDefault"));
    m_javaEdit->setMinimumHeight(40); m_javaEdit->setStyleSheet("font-size:14px;");
    QPushButton* browseJava = new QPushButton(lm->t("wizard.browse"));
    browseJava->setFixedWidth(80); browseJava->setMinimumHeight(40); browseJava->setStyleSheet("font-size:13px;");
    connect(browseJava, &QPushButton::clicked, this, &CreateWizard::selectJava);
    javaRow->addWidget(m_javaEdit, 1); javaRow->addWidget(browseJava);
    layout->addLayout(javaRow);

    QLabel* javaHint = new QLabel(lm->t("wizard.javaHint"));
    javaHint->setStyleSheet("color:#555577; font-size:11px;");
    layout->addWidget(javaHint);

    layout->addSpacing(8);
    m_eulaCheck = new QCheckBox(lm->t("wizard.eula"));
    m_eulaCheck->setStyleSheet("QCheckBox { color: #8888aa; font-size: 13px; } QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid #2a2a4a; border-radius: 4px; background: #12121e; } QCheckBox::indicator:checked { background: #6c5ce7; border-color: #6c5ce7; } QCheckBox::indicator:hover { border-color: #6c5ce7; }");
    layout->addWidget(m_eulaCheck);
    return page;
}

// Step 4: Confirm
QWidget* CreateWizard::createStep4() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* lm = LanguageManager::instance();
    QLabel* label = new QLabel(lm->t("wizard.step4Title"));
    label->setStyleSheet("font-size:17px; font-weight:bold; color:#e0e0f0;");
    layout->addWidget(label);
    QLabel* hint = new QLabel(lm->t("wizard.step4Hint"));
    hint->setStyleSheet("color:#8888aa; font-size:12px;");
    layout->addWidget(hint); layout->addSpacing(12);

    QFrame* card = new QFrame;
    card->setStyleSheet("QFrame { background: #12121e; border: 1px solid #2a2a4a; border-radius: 16px; padding: 20px; }");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(14);
    m_step4Card = card;
    m_step4CardLayout = cardLayout;
    layout->addWidget(card);
    return page;
}

void CreateWizard::refreshStep4() {
    if (!m_step4CardLayout) return;
    QLayoutItem* item;
    while ((item = m_step4CardLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        if (item->layout()) { QLayout* sub = item->layout(); QLayoutItem* si; while ((si = sub->takeAt(0)) != nullptr) { if (si->widget()) si->widget()->deleteLater(); delete si; } }
        delete item;
    }
    auto addRow = [&](const QString& key, const QString& val, const QString& valColor = "#e0e0f0") {
        auto* row = new QHBoxLayout; row->setSpacing(12);
        QLabel* keyLabel = new QLabel(key); keyLabel->setStyleSheet("color:#8888aa; font-size:13px;");
        QLabel* valLabel = new QLabel(val); valLabel->setStyleSheet(QString("color:%1; font-weight:bold; font-size:13px;").arg(valColor)); valLabel->setWordWrap(true);
        row->addWidget(keyLabel); row->addStretch(); row->addWidget(valLabel);
        m_step4CardLayout->addLayout(row);
    };
    auto* lm = LanguageManager::instance();
    QString typeColor = m_type == "vanilla" ? "#facc15" : m_type == "fabric" ? "#22d3ee" : m_type == "spigot" ? "#fb923c" : m_type == "forge" ? "#c084fc" : "#60a5fa";
    QString capType = m_type; capType[0] = capType[0].toUpper();
    addRow(lm->t("wizard.keyType"), capType, typeColor);
    addRow(lm->t("wizard.keyVersion"), m_version.isEmpty() ? lm->t("wizard.notSelected") : m_version);
    if (m_type == "paper") addRow(lm->t("wizard.keyBuild"), m_build.isEmpty() ? lm->t("wizard.latest") : m_build);
    addRow(lm->t("wizard.keyName"), m_serverName.isEmpty() ? lm->t("wizard.notSet") : m_serverName);
    addRow(lm->t("wizard.keyPath"), m_serverPath.isEmpty() ? lm->t("wizard.notSelected") : m_serverPath);
    addRow(lm->t("wizard.keyMemory"), QString("%1 - %2 MB").arg(m_minRam).arg(m_maxRam));
    addRow(lm->t("wizard.keyJava"), m_javaPath);
    addRow(lm->t("wizard.keyEula"), m_eulaAccepted ? lm->t("wizard.agreed") : lm->t("wizard.notAgreed"), m_eulaAccepted ? "#00d2a0" : "#ff4757");
}

void CreateWizard::nextStep() {
    if (m_step == 3) { createServer(); return; }
    auto* lm = LanguageManager::instance();
    if (m_step == 1 && m_version.isEmpty()) { ui.errorLabel->setText(lm->t("wizard.errVersion")); ui.errorLabel->show(); return; }
    if (m_step == 2) {
        m_serverName = m_nameEdit->text().trimmed(); m_serverPath = m_pathEdit->text().trimmed();
        m_minRam = m_minRamCombo->currentData().toInt(); m_maxRam = m_maxRamCombo->currentData().toInt();
        m_javaPath = m_javaEdit->text().trimmed(); if (m_javaPath.isEmpty()) m_javaPath = "java";
        m_eulaAccepted = m_eulaCheck->isChecked();
        if (m_serverName.isEmpty()) { ui.errorLabel->setText(lm->t("wizard.errName")); ui.errorLabel->show(); return; }
        if (m_serverPath.isEmpty()) { ui.errorLabel->setText(lm->t("wizard.errPath")); ui.errorLabel->show(); return; }
        if (!m_eulaAccepted) { ui.errorLabel->setText(lm->t("wizard.errEula")); ui.errorLabel->show(); return; }
    }
    ui.errorLabel->hide();
    if (m_step == 0) m_dl->fetchVersions();
    if (m_step == 1) {
        m_version = m_versionCombo->currentText(); m_build = m_buildCombo->currentText();
        if (m_type == "paper" && m_build.isEmpty()) { ui.errorLabel->setText(lm->t("wizard.errBuild")); ui.errorLabel->show(); return; }
    }
    m_step++;
    ui.stepStack->setCurrentIndex(m_step);
    updateNavButtons();
    if (m_step == 1) { bool isPaper = (m_type == "paper"); m_buildLabel->setVisible(isPaper); m_buildCombo->setVisible(isPaper); if (isPaper && !m_version.isEmpty()) m_dl->fetchPaperBuilds(m_version); }
    if (m_step == 3) refreshStep4();
    if (m_step == 2) { m_nameEdit->setText(m_serverName); m_pathEdit->setText(m_serverPath); }
    updateStepIndicator(m_step);
    fitStackToCurrent();
}

void CreateWizard::prevStep() {
    if (m_step == 2) { m_serverName = m_nameEdit->text().trimmed(); m_serverPath = m_pathEdit->text().trimmed(); m_minRam = m_minRamCombo->currentData().toInt(); m_maxRam = m_maxRamCombo->currentData().toInt(); m_javaPath = m_javaEdit->text().trimmed(); if (m_javaPath.isEmpty()) m_javaPath = "java"; }
    if (m_step > 0) { m_step--; ui.stepStack->setCurrentIndex(m_step); }
    if (m_step == 1) { bool isPaper = (m_type == "paper"); m_buildLabel->setVisible(isPaper); m_buildCombo->setVisible(isPaper); if (isPaper && !m_version.isEmpty()) m_dl->fetchPaperBuilds(m_version); }
    updateNavButtons(); updateStepIndicator(m_step);
    fitStackToCurrent();
}

void CreateWizard::updateNavButtons() {
    auto* lm = LanguageManager::instance();
    ui.prevBtn->setVisible(m_step > 0);
    if (m_step == 3) {
        ui.nextBtn->setText(lm->t("wizard.createServer")); ui.nextBtn->setObjectName("greenBtn");
        ui.nextBtn->style()->unpolish(ui.nextBtn); ui.nextBtn->style()->polish(ui.nextBtn);
    } else {
        ui.nextBtn->setText(lm->t("wizard.nextStep")); ui.nextBtn->setObjectName("primaryBtn");
        ui.nextBtn->style()->unpolish(ui.nextBtn); ui.nextBtn->style()->polish(ui.nextBtn);
    }
}

void CreateWizard::fitStackToCurrent() {
    // Our FittedStackedWidget overrides sizeHint() to return current page height.
    // Just force a layout recalculation so QScrollArea picks up the new size.
    ui.stepStack->updateGeometry();
}

void CreateWizard::selectPath() { auto* lm = LanguageManager::instance(); QString dir = QFileDialog::getExistingDirectory(this, lm->t("wizard.selectDir")); if (!dir.isEmpty()) m_pathEdit->setText(dir); }
void CreateWizard::selectJava() { auto* lm = LanguageManager::instance(); QString file = QFileDialog::getOpenFileName(this, lm->t("wizard.selectJava"), QString(), lm->t("wizard.javaFilter")); if (!file.isEmpty()) m_javaEdit->setText(file); }
void CreateWizard::onVersionsReady(const QStringList& versions) { m_versionCombo->clear(); m_versionCombo->addItems(versions); if (!versions.isEmpty()) { m_version = versions.first(); m_versionCombo->setCurrentIndex(0); } }
void CreateWizard::onPaperBuildsReady(const QStringList& builds) { m_buildCombo->clear(); m_buildCombo->addItems(builds); if (!builds.isEmpty()) m_buildCombo->setCurrentIndex(builds.size() - 1); }

void CreateWizard::createServer() {
    m_serverName = m_nameEdit->text().trimmed(); m_serverPath = m_pathEdit->text().trimmed();
    m_minRam = m_minRamCombo->currentData().toInt(); m_maxRam = m_maxRamCombo->currentData().toInt();
    m_javaPath = m_javaEdit->text().trimmed(); if (m_javaPath.isEmpty()) m_javaPath = "java";
    m_eulaAccepted = m_eulaCheck->isChecked();
    auto* lm = LanguageManager::instance();
    if (m_serverName.isEmpty() || m_serverPath.isEmpty()) { ui.errorLabel->setText(lm->t("wizard.errFill")); ui.errorLabel->show(); return; }
    if (!m_eulaAccepted) { ui.errorLabel->setText(lm->t("wizard.errEulaAgree")); ui.errorLabel->show(); return; }

    ui.errorLabel->hide(); ui.progressArea->show();
    ui.prevBtn->setEnabled(false); ui.nextBtn->setEnabled(false); ui.nextBtn->setText(lm->t("wizard.creating"));

    ServerConfig cfg;
    cfg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    cfg.name = m_serverName; cfg.type = m_type; cfg.version = m_version; cfg.build = m_build;
    cfg.path = m_serverPath; cfg.javaPath = m_javaPath; cfg.minRamMB = m_minRam; cfg.maxRamMB = m_maxRam;

    QDir().mkpath(m_serverPath);
    QFile eula(m_serverPath + "/eula.txt");
    if (eula.open(QIODevice::WriteOnly)) { eula.write("eula=true\n"); eula.close(); }

    QObject* context = new QObject(this);
    connect(m_dl, &DownloadManager::downloadFinished, context, [this, cfg, context](bool ok, const QString& msg) {
        context->deleteLater(); ui.progressArea->hide();
        ui.prevBtn->setEnabled(true); ui.nextBtn->setEnabled(true);
        if (ok) { m_store->saveServer(cfg); emit created(); }
        else { ui.errorLabel->setText(TR("wizard.dlFailed") + msg); ui.errorLabel->show(); }
    });

    if (m_type == "vanilla") m_dl->downloadVanilla(m_version, m_serverPath);
    else if (m_type == "paper") m_dl->downloadPaper(m_version, m_build, m_serverPath);
    else if (m_type == "fabric") m_dl->downloadFabric(m_version, m_serverPath);
}
