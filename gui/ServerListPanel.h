#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include "core/ConfigStore.h"
#include "ui_ServerListPanel.h"

class ServerListPanel : public QWidget {
    Q_OBJECT
public:
    explicit ServerListPanel(ConfigStore* store, QWidget* parent = nullptr);
    void setRunningServerId(const QString& id) { m_runningServerId = id; }

public slots:
    void refresh();

signals:
    void serverSelected(const QString& id);
    void serverDoubleClicked(const QString& id);
    void createRequested();
    void serverControlRequested(const QString& id);
    void serverConfigRequested(const QString& id);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void showEmptyState();
    void showServerGrid();
    QWidget* createServerCard(const ServerConfig& srv);
    QString typeBorderColor(const QString& type) const;
    QString typeLabelColor(const QString& type) const;

    Ui::ServerListPanel ui;
    ConfigStore* m_store;
    QMap<QString, QWidget*> m_cards;
    QString   m_runningServerId;
};
