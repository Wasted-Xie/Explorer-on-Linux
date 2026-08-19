#include "SystemTray.h"
#include <QApplication>
#include <QDebug>
#include <libexplorer-core/Config.h>

namespace explorer::components {

SystemTray::SystemTray(QWidget* parent)
    : explorer::ui::BaseWidget(parent), trayIcon(new QSystemTrayIcon(this)),
      trayMenu(new QMenu(this)) {
    setupUI();
    // Hide the widget itself; we only need the system tray icon
    setVisible(false);
}

SystemTray::~SystemTray() = default;

void SystemTray::setupUI() {
    // Set icon from theme
    QIcon icon = QIcon::fromTheme("applications-system");
    if (icon.isNull()) {
        icon = QIcon::fromTheme("application-x-executable");
    }
    trayIcon->setIcon(icon);
    trayIcon->setToolTip("Explorer Linux");

    // Create a simple context menu
    QAction* action = trayMenu->addAction("打开开始菜单");
    connect(action, &QAction::triggered, []() {
        QMessageBox::information(nullptr, "开始菜单", "这是一个占位的开始菜单。");
    });
    trayMenu->addSeparator();
    QAction* quitAction = trayMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    trayIcon->setContextMenu(trayMenu);

    // Connect tray icon activation to our signal
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        QString reasonStr;
        switch (reason) {
            case QSystemTrayIcon::Trigger: reasonStr = "Trigger"; break;
            case QSystemTrayIcon::DoubleClick: reasonStr = "DoubleClick"; break;
            case QSystemTrayIcon::MiddleClick: reasonStr = "MiddleClick"; break;
            case QSystemTrayIcon::Context: reasonStr = "Context"; break;
            default: reasonStr = "Unknown"; break;
        }
        emit iconActivated(reasonStr);
    });

    trayIcon->show();
}

} // namespace explorer::components

#include "SystemTray.moc"