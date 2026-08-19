#pragma once

#include <QWidget>
#include <libexplorer-ui/BaseWidget.h>
#include <QSystemTrayIcon>
#include <QMenu>

namespace explorer::components {

class SystemTray : public explorer::ui::BaseWidget {
    Q_OBJECT
public:
    explicit SystemTray(QWidget* parent = nullptr);
    ~SystemTray() override;

signals:
    void iconActivated(const QString& reason);

private:
    QSystemTrayIcon* trayIcon;
    QMenu* trayMenu;
    void setupUI();
};

} // namespace explorer::components

#include "SystemTray.moc"