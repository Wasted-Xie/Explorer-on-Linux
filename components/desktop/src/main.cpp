#include "DesktopWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QScreen>
#include <QMessageBox>

#include <libexplorer-core/Config.h>
#include <libexplorer-core/SignalDispatcher.h>
#include <libexplorer-layer/LayerShell.h>

using namespace explorer::desktop;

int main(int argc, char* argv[]) {
    // 设置应用程序属性
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication app(argc, argv);
    app.setApplicationName("explorer-desktop");
    app.setApplicationDisplayName("Explorer Desktop");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Explorer Linux");
    app.setOrganizationDomain("explorer-linux.org");

    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("Explorer Linux Desktop Component");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption noLayerOption("no-layer", "Run without layer-shell (for testing)");
    parser.addOption(noLayerOption);

    QCommandLineOption testOption("test", "Run in test mode (regular window)");
    parser.addOption(testOption);

    QCommandLineOption configOption("config", "Configuration file path", "path");
    parser.addOption(configOption);

    parser.process(app);

    bool useLayerShell = !parser.isSet(noLayerOption) && !parser.isSet(testOption);

    qDebug() << "Starting Explorer Desktop";
    qDebug() << "LayerShell enabled:" << useLayerShell;

    // 初始化配置系统
    explorer::core::Config& config = explorer::core::Config::instance();
    if (parser.isSet(configOption)) {
        // TODO: 支持自定义配置文件路径
        qDebug() << "Config file:" << parser.value(configOption);
    }

    // 初始化 LayerShell 管理器
    if (useLayerShell) {
        if (!explorer::layer::LayerShellManager::instance().init()) {
            qWarning() << "Failed to initialize LayerShell, falling back to regular window";
            useLayerShell = false;
        } else {
            qDebug() << "LayerShell initialized successfully";
        }
    }

    // 创建桌面窗口
    DesktopWindow desktopWindow;
    desktopWindow.initialize();

    // 如果不使用 LayerShell，作为普通窗口显示（用于测试）
    if (!useLayerShell) {
        desktopWindow.setWindowTitle("Explorer Desktop (Test Mode)");
        desktopWindow.resize(1024, 768);
        desktopWindow.show();
    }

    // 连接应用程序退出信号
    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        qDebug() << "Application shutting down";
        explorer::layer::LayerShellManager::instance().shutdown();
    });

    return app.exec();
}