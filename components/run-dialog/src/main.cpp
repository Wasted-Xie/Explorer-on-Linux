#include "RunDialog.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDebug>
#include <QTimer>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("explorer-run-dialog");
    app.setApplicationDisplayName("Explorer Linux Run Dialog");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("explorer-linux");
    app.setQuitOnLastWindowClosed(false); // 保持运行以接收 DBus 信号

    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("Explorer Linux Run Dialog - Press Win+R to show");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption showOption("show", "Show the run dialog immediately");
    parser.addOption(showOption);

    QCommandLineOption hideOption("hide", "Hide the run dialog");
    parser.addOption(hideOption);

    QCommandLineOption toggleOption("toggle", "Toggle the run dialog visibility");
    parser.addOption(toggleOption);

    QCommandLineOption daemonOption("daemon", "Run as daemon (listen for DBus signals)");
    parser.addOption(daemonOption);

    parser.process(app);

    // 创建运行对话框实例
    explorer::components::RunDialog dialog;

    // 处理命令行动作
    if (parser.isSet(showOption)) {
        dialog.showDialog();
    } else if (parser.isSet(hideOption)) {
        dialog.hideDialog();
    } else if (parser.isSet(toggleOption)) {
        if (dialog.isDialogVisible()) {
            dialog.hideDialog();
        } else {
            dialog.showDialog();
        }
    } else if (parser.isSet(daemonOption)) {
        // 守护模式：不自动显示，等待 DBus 信号
        qDebug() << "RunDialog running in daemon mode, waiting for DBus signals...";
    } else {
        // 默认行为：显示对话框（用于测试）
        dialog.showDialog();
    }

    // 如果是守护模式且没有显示，添加一个定时器防止立即退出
    if (parser.isSet(daemonOption) && !dialog.isDialogVisible()) {
        QTimer::singleShot(0, &app, []() {
            // 应用继续运行，等待 DBus 信号
        });
    }

    return app.exec();
}