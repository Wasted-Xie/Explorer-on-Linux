#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include "NotificationCenter.h"
#include <explorer/core/Config.h>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("explorer-notification-center");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Explorer Linux");
    app.setQuitOnLastWindowClosed(false); // 保持后台运行

    // 命令行参数
    QCommandLineParser parser;
    parser.setApplicationDescription("Explorer Linux Notification Center");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption daemonOption(QStringList() << "d" << "daemon",
                                    "Run as daemon (background service)");
    parser.addOption(daemonOption);
    
    QCommandLineOption showOption(QStringList() << "s" << "show",
                                  "Show notification center and exit");
    parser.addOption(showOption);
    
    QCommandLineOption hideOption(QStringList() << "hide",
                                  "Hide notification center and exit");
    parser.addOption(hideOption);
    
    QCommandLineOption toggleOption(QStringList() << "t" << "toggle",
                                    "Toggle notification center visibility and exit");
    parser.addOption(toggleOption);
    
    parser.process(app);

    // 检查 DBus 会话总线
    if (!QDBusConnection::sessionBus().isConnected()) {
        qCritical() << "Cannot connect to DBus session bus";
        return 1;
    }

    // 创建通知中心
    NotificationCenter center;
    if (!center.init()) {
        qCritical() << "Failed to initialize NotificationCenter";
        return 1;
    }

    // 处理一次性命令
    if (parser.isSet(showOption)) {
        center.Show();
        return 0;
    }
    if (parser.isSet(hideOption)) {
        center.Hide();
        return 0;
    }
    if (parser.isSet(toggleOption)) {
        center.toggleCenter();
        return 0;
    }

    // 守护进程模式或默认模式：进入事件循环
    qDebug() << "Notification Center started";
    qDebug() << "DBus service:" << "org.explorer.NotificationCenter";
    qDebug() << "Object path:" << "/org/explorer/NotificationCenter";

    int result = app.exec();
    
    center.shutdown();
    qDebug() << "Notification Center stopped";
    
    return result;
}