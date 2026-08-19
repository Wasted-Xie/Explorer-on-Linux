#include "Daemon.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QSysInfo>
#include <QTimer>

#include <libexplorer-core/Config.h>
#include <libexplorer-core/Settings.h>
#include <libexplorer-utils/StringUtils.h>
#include <libexplorer-utils/ProcessUtils.h>

#include "WindowManager.h"

namespace explorer::daemon {

class Daemon::Impl {
public:
    Impl(Daemon* owner) : q(owner) {}

    bool parseCommandLine(int& argc, char** argv) {
        QCoreApplication::setApplicationName("explorer-linux-daemon");
        QCoreApplication::setApplicationVersion("0.1.0");
        QCoreApplication::setOrganizationName("Explorer Linux");
        QCoreApplication::setOrganizationDomain("explorer-linux.org");

        QCommandLineParser parser;
        parser.setApplicationDescription("Explorer Linux Shell Daemon");
        parser.addHelpOption();
        parser.addVersionOption();

        QCommandLineOption debugOption(QStringList() << "d" << "debug",
                                       "Enable debug output");
        parser.addOption(debugOption);

        QCommandLineOption configOption(QStringList() << "c" << "config",
                                        "Specify configuration file",
                                        "file");
        parser.addOption(configOption);

        parser.process(argc, argv);

        if (parser.isSet(debugOption)) {
            qSetMessagePattern("%{file}:%{line} [%{type}] %{message}");
        }

        if (parser.isSet(configOption)) {
            QString configFile = parser.value(configOption);
            // TODO: 加载自定义配置文件
            qDebug() << "Using config file:" << configFile;
        }

        return true;
    }

    bool initializeServices() {
        // 初始化核心模型
        if (!m_explorerModel.init()) {
            qWarning() << "Failed to initialize explorer model";
            return false;
        }

        // 初始化信号分发器
        if (!m_signalDispatcher.init()) {
            qWarning() << "Failed to initialize signal dispatcher";
            return false;
        }

        // 初始化服务管理器
        if (!m_serviceManager.init()) {
            qWarning() << "Failed to initialize service manager";
            return false;
        }

        // 初始化插件加载器
        if (!m_pluginLoader.init()) {
            qWarning() << "Failed to initialize plugin loader";
            return false;
        }

        // 初始化 layer-shell 管理器
        if (!m_layerShellManager.init()) {
            qWarning() << "Failed to initialize layer-shell manager";
            // 不是致命错误，继续运行
        }

        // 初始化 DBus 服务
        if (!m_dbusService.init()) {
            qWarning() << "Failed to initialize DBus service";
            return false;
        }

        // 初始化消息总线
        if (!m_messageBus.init()) {
            qWarning() << "Failed to initialize message bus";
            return false;
        }

        // 初始化 WindowManager
        m_windowManager = std::make_unique<WindowManager>();
        if (!m_windowManager->init(m_dbusService.connection())) {
            qWarning() << "Failed to initialize WindowManager";
            return false;
        }

        // 注册 WindowManager 到服务管理器
        m_serviceManager.registerService("WindowManager", m_windowManager.get());

        return true;
    }

    void setupDBusService() {
        // 注册核心服务到 DBus
        m_dbusService.registerObject("/org/explorer/Core", &m_explorerModel);
        m_dbusService.registerObject("/org/explorer/Signals", &m_signalDispatcher);
        m_dbusService.registerObject("/org/explorer/Services", &m_serviceManager);
        m_dbusService.registerObject("/org/explorer/Plugins", &m_pluginLoader);
        
        // 注册 WindowManager
        if (m_windowManager) {
            m_dbusService.registerObject("/org/explorer/WindowManager", m_windowManager.get());
        }
    }

    Daemon* q;
    explorer::core::ExplorerModel m_explorerModel;
    explorer::core::SignalDispatcher m_signalDispatcher;
    ServiceManager m_serviceManager;
    PluginLoader m_pluginLoader;
    explorer::layer::LayerShellManager m_layerShellManager;
    explorer::ipc::DBusService m_dbusService{"org.explorer.Daemon"};
    explorer::ipc::MessageBus m_messageBus;
    std::unique_ptr<WindowManager> m_windowManager;
};

Daemon::Daemon(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
Daemon::~Daemon() {
    shutdown();
}

bool Daemon::init(int& argc, char** argv) {
    emit startingUp();

    // 解析命令行参数
    if (!d->parseCommandLine(argc, argv)) {
        return false;
    }

    // 初始化 Qt 应用程序（如果还没有的话）
    if (!QCoreApplication::instance()) {
        // 实际上，Daemon 通常不需要 GUI，所以我们不创建 QApplication
        // 但我们需要 QCoreApplication 来处理事件等
        static QCoreApplication app(argc, argv);
    }

    // 初始化所有服务
    if (!d->initializeServices()) {
        return false;
    }

    // 设置 DBus 服务
    d->setupDBusService();

    // 发送初始化完成信号
    QTimer::singleShot(0, this, [this]() {
        emit initialized();
    });

    return true;
}

int Daemon::run() {
    // 进入事件循环
    return QCoreApplication::exec();
}

void Daemon::shutdown() {
    emit shuttingDown();

    // 按相反顺序关闭服务
    if (d->windowManager) {
        d->windowManager->shutdown();
        d->windowManager.reset();
    }
    d->messageBus.shutdown();
    d->dbusService.stop();
    d->layerShellManager.shutdown();
    d->pluginLoader.shutdown();
    d->serviceManager.shutdown();
    d->signalDispatcher.shutdown();
    d->explorerModel.shutdown();
}

ServiceManager& Daemon::serviceManager() {
    return d->serviceManager;
}

PluginLoader& Daemon::pluginLoader() {
    return d->pluginLoader;
}

explorer::layer::LayerShellManager& Daemon::layerShellManager() {
    return d->layerShellManager;
}

explorer::core::ExplorerModel& Daemon::explorerModel() {
    return d->explorerModel;
}

explorer::core::SignalDispatcher& Daemon::signalDispatcher() {
    return d->signalDispatcher;
}

WindowManager& Daemon::windowManager() {
    if (!d->windowManager) {
        // 这不应该发生，因为 WindowManager 在 init() 中创建
        static WindowManager dummy;
        return dummy;
    }
    return *d->windowManager;
}

} // namespace explorer::daemon

#include "Daemon.moc"