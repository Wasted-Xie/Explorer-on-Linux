#include "StartMenuManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <QTextStream>
#include "libs/libexplorer-core/src/config/Config.h"
#include "libs/libexplorer-core/src/signals/SignalDispatcher.h"

namespace explorer::startmenu {

class StartMenuManager::Impl {
public:
    std::unique_ptr<StartMenuWindow> window;
    QList<DesktopEntry> applications;
    
    // DBus
    std::unique_ptr<explorer::ipc::MessageBus> messageBus;
    std::unique_ptr<explorer::ipc::ServiceWatcher> serviceWatcher;
    bool daemonConnected = false;
    
    // 配置
    QString applicationsDir = "/usr/share/applications";
    QStringList additionalDirs;
};

StartMenuManager::StartMenuManager(QObject* parent)
    : QObject(parent), d(std::make_unique<Impl>()) {
    // 从配置读取应用目录
    auto& config = explorer::core::Config::instance();
    d->applicationsDir = config.value("startmenu/applicationsDir", "/usr/share/applications").toString();
    d->additionalDirs = config.value("startmenu/additionalDirs", QStringList() << "/usr/local/share/applications" << QDir::homePath() + "/.local/share/applications").toStringList();
}

StartMenuManager::~StartMenuManager() {
    shutdown();
}

bool StartMenuManager::init() {
    // 创建窗口
    d->window = std::make_unique<StartMenuWindow>();
    connect(d->window.get(), &StartMenuWindow::applicationLaunched, this, &StartMenuManager::onApplicationLaunched);
    connect(d->window.get(), &StartMenuWindow::closed, this, [this]() {
        emit visibilityChanged(false);
    });
    
    // 初始化消息总线
    d->messageBus = std::make_unique<explorer::ipc::MessageBus>(this);
    if (!d->messageBus->init()) {
        qWarning() << "Failed to initialize MessageBus";
    }
    
    // 监视守护进程
    d->serviceWatcher = std::make_unique<explorer::ipc::ServiceWatcher>(this);
    connect(d->serviceWatcher.get(), &explorer::ipc::ServiceWatcher::serviceAppeared, this, &StartMenuManager::onDaemonAppeared);
    connect(d->serviceWatcher.get(), &explorer::ipc::ServiceWatcher::serviceVanished, this, &StartMenuManager::onDaemonVanished);
    d->serviceWatcher->watchService("org.explorer.Daemon");
    
    // 设置 DBus 信号监听
    setupDBusSignals();
    
    // 加载应用列表
    reloadApplications();
    
    // 注册到守护进程
    registerWithDaemon();
    
    qInfo() << "StartMenuManager initialized";
    return true;
}

void StartMenuManager::shutdown() {
    unregisterFromDaemon();
    
    if (d->window) {
        d->window->hide();
        d->window.reset();
    }
    
    if (d->serviceWatcher) {
        d->serviceWatcher->unwatchAll();
        d->serviceWatcher.reset();
    }
    
    if (d->messageBus) {
        d->messageBus.reset();
    }
    
    qInfo() << "StartMenuManager shutdown";
}

void StartMenuManager::showAt(const QPoint& globalPos) {
    if (d->window) {
        d->window->showAt(globalPos);
        emit visibilityChanged(true);
    }
}

void StartMenuManager::hide() {
    if (d->window) {
        d->window->hide();
        emit visibilityChanged(false);
    }
}

void StartMenuManager::toggle(const QPoint& globalPos) {
    if (d->window && d->window->isVisible()) {
        hide();
    } else {
        showAt(globalPos);
    }
}

void StartMenuManager::reloadApplications() {
    d->applications.clear();
    
    // 主目录
    auto mainApps = parseDesktopFiles(d->applicationsDir);
    d->applications.append(mainApps);
    
    // 额外目录
    for (const QString& dir : d->additionalDirs) {
        if (QDir(dir).exists()) {
            auto apps = parseDesktopFiles(dir);
            d->applications.append(apps);
        }
    }
    
    // 按名称排序
    std::sort(d->applications.begin(), d->applications.end(),
              [](const DesktopEntry& a, const DesktopEntry& b) {
                  return a.name.toLower() < b.name.toLower();
              });
    
    // 更新窗口
    if (d->window) {
        QList<QPair<QString, QString>> apps;
        for (const auto& entry : d->applications) {
            if (!entry.noDisplay) {
                apps.append(qMakePair(entry.name, entry.exec));
            }
        }
        d->window->setApplications(apps);
    }
    
    qInfo() << "Loaded" << d->applications.size() << "applications";
}

bool StartMenuManager::isVisible() const {
    return d->window && d->window->isVisible();
}

void StartMenuManager::onShowRequested(const QPoint& pos) {
    showAt(pos);
}

void StartMenuManager::onHideRequested() {
    hide();
}

void StartMenuManager::onToggleRequested(const QPoint& pos) {
    toggle(pos);
}

void StartMenuManager::onDaemonAppeared() {
    qInfo() << "Daemon appeared, registering";
    d->daemonConnected = true;
    registerWithDaemon();
}

void StartMenuManager::onDaemonVanished() {
    qWarning() << "Daemon vanished";
    d->daemonConnected = false;
}

void StartMenuManager::onApplicationLaunched(const QString& exec) {
    // 使用 ProcessUtils 启动应用
    auto result = explorer::utils::ProcessUtils::execute(exec);
    if (result.exitCode != 0) {
        qWarning() << "Failed to launch:" << exec << "exit code:" << result.exitCode << "stderr:" << result.stderr;
    }
    hide();
    emit applicationLaunched(exec);
}

void StartMenuManager::registerWithDaemon() {
    if (!d->daemonConnected) return;
    
    // 通过 MessageBus 发布注册消息
    if (d->messageBus) {
        QVariantMap data;
        data["service"] = "startmenu";
        data["pid"] = QCoreApplication::applicationPid();
        d->messageBus->publish("explorer.daemon.register", data);
    }
    
    // 也可以直接调用 DBus 方法
    explorer::ipc::DBusInterface daemonInterface("org.explorer.Daemon", "/org/explorer/Daemon", "org.explorer.Daemon");
    if (daemonInterface.isValid()) {
        daemonInterface.callNoReply("RegisterComponent", "startmenu", QCoreApplication::applicationPid());
    }
}

void StartMenuManager::unregisterFromDaemon() {
    if (!d->daemonConnected) return;
    
    if (d->messageBus) {
        QVariantMap data;
        data["service"] = "startmenu";
        d->messageBus->publish("explorer.daemon.unregister", data);
    }
    
    explorer::ipc::DBusInterface daemonInterface("org.explorer.Daemon", "/org/explorer/Daemon", "org.explorer.Daemon");
    if (daemonInterface.isValid()) {
        daemonInterface.callNoReply("UnregisterComponent", "startmenu");
    }
    
    d->daemonConnected = false;
}

QList<StartMenuManager::DesktopEntry> StartMenuManager::parseDesktopFiles(const QString& dir) {
    QList<DesktopEntry> entries;
    QStringList files = explorer::utils::FileUtils::listFiles(dir, QStringList() << "*.desktop", false);
    
    for (const QString& file : files) {
        DesktopEntry entry = parseDesktopFile(file);
        if (!entry.name.isEmpty() && !entry.exec.isEmpty()) {
            entries.append(entry);
        }
    }
    
    return entries;
}

StartMenuManager::DesktopEntry StartMenuManager::parseDesktopFile(const QString& filePath) {
    DesktopEntry entry;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return entry;
    }
    
    QTextStream in(&file);
    bool inDesktopEntry = false;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        if (line == "[Desktop Entry]") {
            inDesktopEntry = true;
            continue;
        }
        
        if (line.startsWith('[') && line != "[Desktop Entry]") {
            inDesktopEntry = false;
            continue;
        }
        
        if (!inDesktopEntry) continue;
        
        if (line.startsWith("Name=")) {
            entry.name = line.mid(5);
        } else if (line.startsWith("Exec=")) {
            entry.exec = line.mid(5);
            // 移除字段代码如 %f, %u 等
            entry.exec = entry.exec.remove(QRegularExpression("%[fFuUdDnNickvm]"));
            entry.exec = entry.exec.trimmed();
        } else if (line.startsWith("Icon=")) {
            entry.icon = line.mid(5);
        } else if (line.startsWith("Comment=")) {
            entry.comment = line.mid(8);
        } else if (line.startsWith("Categories=")) {
            entry.categories = line.mid(11).split(';', Qt::SkipEmptyParts);
        } else if (line.startsWith("NoDisplay=")) {
            entry.noDisplay = line.mid(10).toLower() == "true";
        } else if (line.startsWith("Terminal=")) {
            entry.terminal = line.mid(9).toLower() == "true";
        }
    }
    
    file.close();
    return entry;
}

QIcon StartMenuManager::loadIcon(const QString& iconName) {
    // 尝试从主题加载
    QIcon icon = QIcon::fromTheme(iconName);
    if (!icon.isNull()) {
        return icon;
    }
    
    // 尝试作为文件路径加载
    if (QFile::exists(iconName)) {
        return QIcon(iconName);
    }
    
    // 尝试标准图标目录
    QStringList iconDirs = {
        "/usr/share/icons",
        "/usr/local/share/icons",
        QDir::homePath() + "/.local/share/icons"
    };
    
    QStringList extensions = {".png", ".svg", ".xpm"};
    
    for (const QString& dir : iconDirs) {
        QDir d(dir);
        if (!d.exists()) continue;
        
        QStringList subdirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& subdir : subdirs) {
            for (const QString& ext : extensions) {
                QString path = dir + "/" + subdir + "/" + iconName + ext;
                if (QFile::exists(path)) {
                    return QIcon(path);
                }
            }
        }
    }
    
    return QIcon();
}

void StartMenuManager::setupDBusSignals() {
    if (!d->messageBus) return;
    
    // 监听显示请求
    d->messageBus->subscribe("explorer.startmenu.show", [this](const QString&, const QVariant& msg) {
        QVariantMap data = msg.toMap();
        int x = data["x"].toInt();
        int y = data["y"].toInt();
        onShowRequested(QPoint(x, y));
    });
    
    // 监听隐藏请求
    d->messageBus->subscribe("explorer.startmenu.hide", [this](const QString&, const QVariant&) {
        onHideRequested();
    });
    
    // 监听切换请求
    d->messageBus->subscribe("explorer.startmenu.toggle", [this](const QString&, const QVariant& msg) {
        QVariantMap data = msg.toMap();
        int x = data["x"].toInt();
        int y = data["y"].toInt();
        onToggleRequested(QPoint(x, y));
    });
    
    // 监听重新加载请求
    d->messageBus->subscribe("explorer.startmenu.reload", [this](const QString&, const QVariant&) {
        reloadApplications();
    });
}

} // namespace explorer::startmenu