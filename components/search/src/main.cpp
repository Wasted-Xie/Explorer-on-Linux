#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>

#include <explorer/core/Config.h>
#include <explorer/core/SignalDispatcher.h>

#include "SearchWindow.h"

int main(int argc, char* argv[]) {
    // 启用高 DPI 支持
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("explorer-search");
    app.setApplicationDisplayName("Explorer Linux Search");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Explorer Linux");
    app.setOrganizationDomain("explorer-linux.org");

    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("Explorer Linux Global Search Component");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption showOption("show", "Show the search window");
    parser.addOption(showOption);

    QCommandLineOption hideOption("hide", "Hide the search window");
    parser.addOption(hideOption);

    QCommandLineOption toggleOption("toggle", "Toggle the search window visibility");
    parser.addOption(toggleOption);

    QCommandLineOption daemonOption("daemon", "Run as daemon (register DBus service)");
    parser.addOption(daemonOption);

    QCommandLineOption testOption("test", "Run in test mode (show window and exit on close)");
    parser.addOption(testOption);

    parser.process(app);

    // 初始化配置系统
    auto& config = explorer::core::Config::instance();

    // 创建搜索窗口
    explorer::search::SearchWindow window;

    // 处理命令行参数
    if (parser.isSet(showOption)) {
        window.showWindow();
    } else if (parser.isSet(hideOption)) {
        window.hideWindow();
        return 0;
    } else if (parser.isSet(toggleOption)) {
        window.toggleWindow();
    } else if (parser.isSet(testOption)) {
        // 测试模式：显示窗口，关闭时退出
        window.showWindow();
        QObject::connect(&window, &explorer::search::SearchWindow::visibilityChanged,
                         [&app](bool visible) {
                             if (!visible) {
                                 app.quit();
                             }
                         });
    } else if (parser.isSet(daemonOption)) {
        // 守护模式：只注册 DBus 服务，不显示窗口
        qDebug() << "Running in daemon mode, search service registered";
    } else {
        // 默认行为：显示窗口（用于独立测试）
        window.showWindow();
    }

    // 设置应用程序退出时的清理
    QObject::connect(&app, &QApplication::aboutToQuit, [&window]() {
        window.saveSettings();
    });

    return app.exec();
}