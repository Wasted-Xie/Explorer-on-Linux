#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include "libs/libexplorer-core/src/config/Config.h"
#include "libs/libexplorer-core/src/signals/SignalDispatcher.h"
#include "libs/libexplorer-layer/src/LayerShell.h"
#include "StartMenuManager.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("explorer-startmenu");
    app.setApplicationDisplayName("Explorer Start Menu");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Explorer Linux");
    app.setOrganizationDomain("org.explorer");
    
    // 设置高 DPI 支持
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("Explorer Linux Start Menu");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption showOption({"s", "show"}, "Show menu at position (x,y)", "position", "0,0");
    parser.addOption(showOption);
    
    QCommandLineOption hideOption({"hide"}, "Hide menu if visible");
    parser.addOption(hideOption);
    
    QCommandLineOption toggleOption({"t", "toggle"}, "Toggle menu at position (x,y)", "position", "0,0");
    parser.addOption(toggleOption);
    
    QCommandLineOption daemonOption({"d", "daemon"}, "Run as daemon (default)");
    parser.addOption(daemonOption);
    
    QCommandLineOption configOption({"c", "config"}, "Config file path", "path");
    parser.addOption(configOption);
    
    parser.process(app);
    
    // 初始化配置
    auto& config = explorer::core::Config::instance();
    if (parser.isSet(configOption)) {
        // TODO: 支持自定义配置文件路径
    }
    
    // 初始化 LayerShell
    auto& layerManager = explorer::layer::LayerShellManager::instance();
    if (!layerManager.init()) {
        qWarning() << "Failed to initialize LayerShell, some features may not work";
    }
    
    // 初始化主题
    auto& themeManager = explorer::ui::ThemeManager::instance();
    themeManager.init();
    themeManager.setTheme(explorer::ui::ThemeManager::ThemeType::Dark);
    
    // 创建管理器
    explorer::startmenu::StartMenuManager manager;
    if (!manager.init()) {
        qCritical() << "Failed to initialize StartMenuManager";
        return 1;
    }
    
    // 处理命令行动作
    if (parser.isSet(showOption)) {
        QString pos = parser.value(showOption);
        QStringList coords = pos.split(',');
        if (coords.size() == 2) {
            int x = coords[0].toInt();
            int y = coords[1].toInt();
            manager.showAt(QPoint(x, y));
        }
    } else if (parser.isSet(hideOption)) {
        manager.hide();
    } else if (parser.isSet(toggleOption)) {
        QString pos = parser.value(toggleOption);
        QStringList coords = pos.split(',');
        if (coords.size() == 2) {
            int x = coords[0].toInt();
            int y = coords[1].toInt();
            manager.toggle(QPoint(x, y));
        }
    }
    
    // 如果没有指定动作，作为守护进程运行
    if (!parser.isSet(showOption) && !parser.isSet(hideOption) && !parser.isSet(toggleOption)) {
        qInfo() << "Start Menu running as daemon";
        return app.exec();
    }
    
    // 对于显示/隐藏/切换操作，运行事件循环一小段时间以处理窗口显示
    QTimer::singleShot(5000, &app, &QGuiApplication::quit);
    return app.exec();
}