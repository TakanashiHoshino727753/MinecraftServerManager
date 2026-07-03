#include "ServerListPanel.h"
#include "core/LanguageManager.h"
#define TR(key) LanguageManager::instance()->t(key)
#include <QLabel>
#include <QHBoxLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QEvent>
#include <QStyle>
#include <QIcon>
#include "core/LanguageManager.h"

ServerListPanel::ServerListPanel(ConfigStore* store, QWidget* parent)
    : QWidget(parent), m_store(store)
{
    ui.setupUi(this);

    // Apply i18n to .ui strings
    ui.title->setText(TR("list.title"));
    ui.createBtn->setText(TR("list.createBtn"));

    // Style titles
    ui.title->setStyleSheet("font-size:24px; font-weight:bold; color:#a29bfe;");
    ui.countLabel->setStyleSheet("color:#8888aa; font-size:12px;");

    // Button styles
    ui.refreshBtn->setObjectName("ghostBtn");
    ui.refreshBtn->setFixedSize(40, 40);                                   // 1:1 正方形
    ui.refreshBtn->setText(QString::fromUtf8("\u21BB"));                   // ↻
    ui.refreshBtn->setToolTip(TR("list.refresh"));
    ui.refreshBtn->setStyleSheet(
        "QPushButton#ghostBtn { background: transparent; border: 1px solid rgba(42,42,74,0.5);"
        " color: #8888aa; border-radius: 8px; padding: 0; font-size: 18px; }"
        "QPushButton#ghostBtn:hover { background: rgba(255,255,255,0.05); color: #e0e0f0; }");
    ui.createBtn->setObjectName("primaryBtn");
    ui.createBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);   // 宽度跟随文字

    connect(ui.refreshBtn, &QPushButton::clicked, this, &ServerListPanel::refresh);
    connect(ui.createBtn, &QPushButton::clicked, this, &ServerListPanel::createRequested);

    ui.gridLayout->addStretch();
}

QString ServerListPanel::typeBorderColor(const QString& type) const {
    if (type == "vanilla") return "rgba(250,204,21,0.3)";
    if (type == "paper")   return "rgba(96,165,250,0.3)";
    if (type == "fabric")  return "rgba(34,211,238,0.3)";
    if (type == "spigot")  return "rgba(251,146,60,0.3)";
    if (type == "forge")   return "rgba(192,132,252,0.3)";
    return "rgba(42,42,74,0.5)";
}

QString ServerListPanel::typeLabelColor(const QString& type) const {
    if (type == "vanilla") return "#facc15";
    if (type == "paper")   return "#60a5fa";
    if (type == "fabric")  return "#22d3ee";
    if (type == "spigot")  return "#fb923c";
    if (type == "forge")   return "#c084fc";
    return "#8888aa";
}

bool ServerListPanel::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonDblClick) {
        QWidget* w = qobject_cast<QWidget*>(obj);
        if (w) {
            QString serverId = w->property("serverId").toString();
            if (!serverId.isEmpty()) {
                emit serverDoubleClicked(serverId);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

QWidget* ServerListPanel::createServerCard(const ServerConfig& srv) {
    QFrame* card = new QFrame;
    card->setCursor(Qt::PointingHandCursor);
    card->setMinimumWidth(240);
    card->setMaximumWidth(380);

    QString borderColor = typeBorderColor(srv.type);
    QString capType = srv.type;
    capType[0] = capType[0].toUpper();

    card->setStyleSheet(QString(
        "QFrame#card_%1 { background: #12121e; border: 1px solid %2; border-radius: 12px; padding: 16px; }"
        "QFrame#card_%1:hover { border-color: rgba(108,92,231,0.4); }"
    ).arg(srv.id, borderColor));
    card->setObjectName(QString("card_%1").arg(srv.id));
    card->setProperty("serverId", srv.id);
    card->installEventFilter(this);

    QVBoxLayout* cl = new QVBoxLayout(card);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(8);

    // Row 1: Status + Type badge
    QHBoxLayout* row1 = new QHBoxLayout;
    row1->setSpacing(6);
    QHBoxLayout* statusBox = new QHBoxLayout;
    statusBox->setSpacing(6);
    bool isRunning = (srv.id == m_runningServerId);
    QLabel* dot = new QLabel;
    dot->setFixedSize(10, 10);
    dot->setStyleSheet(isRunning ? "background: #00d2a0; border-radius: 5px;" : "background: #555566; border-radius: 5px;");
    statusBox->addWidget(dot);
    QLabel* statusText = new QLabel(isRunning ? TR("list.running") : TR("list.stopped"));
    statusText->setStyleSheet(isRunning ? "font-size:12px; color:#00d2a0; font-weight:500;" : "font-size:12px; color:#8888aa;");
    statusBox->addWidget(statusText);
    row1->addLayout(statusBox);
    row1->addStretch();
    QLabel* typeBadge = new QLabel(capType);
    typeBadge->setStyleSheet(QString("font-size:12px; font-weight:500; color:%1;").arg(typeLabelColor(srv.type)));
    row1->addWidget(typeBadge);
    cl->addLayout(row1);

    // Row 2: Name
    QLabel* nameLabel = new QLabel(srv.name);
    nameLabel->setStyleSheet("font-size:15px; font-weight:600; color:#e0e0f0;");
    nameLabel->setWordWrap(true);
    cl->addWidget(nameLabel);

    // Row 3: Version
    QLabel* versionLabel = new QLabel(TR("list.version").arg(srv.version));
    versionLabel->setStyleSheet("font-size:12px; color:#8888aa;");
    cl->addWidget(versionLabel);

    // Row 4: Info
    QHBoxLayout* infoRow = new QHBoxLayout;
    infoRow->setSpacing(16);
    QLabel* memInfo = new QLabel(TR("list.memory").arg(srv.minRamMB).arg(srv.maxRamMB));
    memInfo->setStyleSheet("font-size:11px; color:#8888aa;");
    infoRow->addWidget(memInfo);
    QLabel* uptimeInfo = new QLabel(TR("list.uptime"));
    uptimeInfo->setStyleSheet("font-size:11px; color:#8888aa;");
    infoRow->addWidget(uptimeInfo);
    infoRow->addStretch();
    cl->addLayout(infoRow);

    // Row 5: Path
    QLabel* pathLabel = new QLabel(TR("list.path").arg(srv.path));
    pathLabel->setStyleSheet("font-size:10px; color:rgba(136,136,170,0.6);");
    pathLabel->setWordWrap(false);
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    cl->addWidget(pathLabel);

    // Row 6: Action buttons
    QHBoxLayout* actionRow = new QHBoxLayout;
    actionRow->setSpacing(8);

    QPushButton* startBtn = new QPushButton(isRunning ? TR("list.stop") : TR("list.start"));
    startBtn->setStyleSheet(isRunning
        ? "QPushButton { background: rgba(255,71,87,0.1); border: 1px solid rgba(255,71,87,0.2); color: #ff4757; border-radius: 8px; padding: 6px 14px; font-size: 12px; font-weight: 500; } QPushButton:hover { background: rgba(255,71,87,0.2); }"
        : "QPushButton { background: rgba(0,210,160,0.1); border: 1px solid rgba(0,210,160,0.2); color: #00d2a0; border-radius: 8px; padding: 6px 14px; font-size: 12px; font-weight: 500; } QPushButton:hover { background: rgba(0,210,160,0.2); }");
    connect(startBtn, &QPushButton::clicked, this, [this, id = srv.id]() { emit serverControlRequested(id); });
    actionRow->addWidget(startBtn, 1);

    QPushButton* settingsBtn = new QPushButton;
    settingsBtn->setFixedSize(34, 34);
    settingsBtn->setIcon(QIcon(":/icons/gear.svg"));
    settingsBtn->setIconSize(QSize(18, 18));
    settingsBtn->setToolTip(TR("list.editTooltip"));
    settingsBtn->setStyleSheet("QPushButton { background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3); color: #8888aa; border-radius: 8px; font-size: 16px; } QPushButton:hover { background: rgba(108,92,231,0.15); color: #a29bfe; }");
    connect(settingsBtn, &QPushButton::clicked, this, [this, id = srv.id]() { emit serverConfigRequested(id); });
    actionRow->addWidget(settingsBtn);

    QPushButton* detailBtn = new QPushButton;
    detailBtn->setFixedSize(34, 34);
    detailBtn->setIcon(QIcon(":/icons/chevron_right.svg"));
    detailBtn->setIconSize(QSize(18, 18));
    detailBtn->setToolTip(TR("list.detailTooltip"));
    detailBtn->setStyleSheet("QPushButton { background: rgba(26,26,46,0.5); border: 1px solid rgba(42,42,74,0.3); color: #8888aa; border-radius: 8px; font-size: 16px; } QPushButton:hover { background: rgba(255,255,255,0.05); color: #e0e0f0; }");
    connect(detailBtn, &QPushButton::clicked, this, [this, id = srv.id]() { emit serverSelected(id); });
    actionRow->addWidget(detailBtn);

    cl->addLayout(actionRow);
    m_cards[srv.id] = card;
    return card;
}

void ServerListPanel::showEmptyState() {
    QVBoxLayout* emptyLayout = new QVBoxLayout;
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->addStretch();

    QWidget* iconWidget = new QWidget;
    iconWidget->setFixedSize(64, 64);
    iconWidget->setStyleSheet("background: rgba(108,92,231,0.1); border: 1px solid rgba(108,92,231,0.2); border-radius: 16px;");
    QVBoxLayout* iconL = new QVBoxLayout(iconWidget);
    QLabel* iconLabel = new QLabel("SV");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("color: rgba(108,92,231,0.4); font-size: 24px; font-weight: bold; background: transparent;");
    iconL->addWidget(iconLabel);
    emptyLayout->addWidget(iconWidget, 0, Qt::AlignCenter);
    emptyLayout->addSpacing(16);

    QLabel* eTitle = new QLabel(TR("list.emptyTitle"));
    eTitle->setStyleSheet("color:#8888aa; font-size:16px; font-weight:600;");
    emptyLayout->addWidget(eTitle, 0, Qt::AlignCenter);
    emptyLayout->addSpacing(8);

    QLabel* eDesc = new QLabel(TR("list.emptyDesc"));
    eDesc->setStyleSheet("color:rgba(136,136,170,0.6); font-size:13px;");
    eDesc->setWordWrap(true);
    eDesc->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(eDesc, 0, Qt::AlignCenter);
    emptyLayout->addSpacing(24);

    QPushButton* createBtn = new QPushButton(TR("list.createFirst"));
    createBtn->setObjectName("primaryBtn");
    createBtn->setMinimumHeight(44);
    createBtn->setFixedWidth(280);
    connect(createBtn, &QPushButton::clicked, this, &ServerListPanel::createRequested);
    emptyLayout->addWidget(createBtn, 0, Qt::AlignCenter);
    emptyLayout->addStretch();

    ui.gridLayout->addLayout(emptyLayout);
}

void ServerListPanel::showServerGrid() {
    QLayoutItem* item;
    while ((item = ui.gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        if (item->layout()) {
            QLayout* subLayout = item->layout();
            QLayoutItem* si;
            while ((si = subLayout->takeAt(0)) != nullptr) {
                if (si->widget()) si->widget()->deleteLater();
                delete si;
            }
        }
        delete item;
    }
    m_cards.clear();

    auto servers = m_store->servers();
    if (servers.isEmpty()) return;

    const int cols = 3;
    for (int i = 0; i < servers.size(); i += cols) {
        QHBoxLayout* row = new QHBoxLayout;
        row->setSpacing(12);
        for (int j = 0; j < cols && (i + j) < servers.size(); j++) {
            QWidget* card = createServerCard(servers[i + j]);
            row->addWidget(card, 1);
        }
        for (int j = servers.size() - i; j < cols; j++)
            row->addStretch(1);
        ui.gridLayout->addLayout(row);
    }
    ui.gridLayout->addStretch();
}

void ServerListPanel::refresh() {
    auto servers = m_store->servers();
    if (servers.isEmpty()) {
        QLayoutItem* item;
        while ((item = ui.gridLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            if (item->layout()) {
                QLayout* subLayout = item->layout();
                QLayoutItem* si;
                while ((si = subLayout->takeAt(0)) != nullptr) {
                    if (si->widget()) si->widget()->deleteLater();
                    delete si;
                }
            }
            delete item;
        }
        m_cards.clear();
        showEmptyState();
    } else {
        showServerGrid();
    }
    ui.countLabel->setText(TR("list.serverCount").arg(servers.size()));
}
