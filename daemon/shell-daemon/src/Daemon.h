#pragma once

#include <QObject>
#include <QDBusConnection>
#include <QString>
#include <QMap>
#include <memory>
#include <functional>

#include <libexplorer-core/ExplorerModel.h>
#include <libexplorer-core/SignalDispatcher.h>
#include <libexplorer-ipc/DBusService.h>
#include <libexplorer-ipc/MessageBus.h>
#include <libexplorer-layer/LayerShell.h>

namespace explorer::daemon {

// 前向声明
class WindowManager;

// 服务管理器 - 管理后台服务和插件
class ServiceManager : public QObject {
    Q_OBJECT
public:
    explicit ServiceManager(QObject* parent = nullptr);
    ~ServiceManager() override;

    bool init();
    void shutdown();

    // 服务注册/发现
    bool registerService(const QString& name, QObject* service);
    QObject* service(const QString& name) const;
    bool unregisterService(const QString& name);

    // 插件管理
    bool loadPlugin(const QString& path);
    bool unloadPlugin(const QString& name);

signals:
    void serviceRegistered(const QString& name);
    void serviceUnregistered(const QString& name);
    void pluginLoaded(const QString& name);
    void pluginUnloaded(const QString& name);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

// 插件加载器
class PluginLoader : public QObject {
    Q_OBJECT
public:
    explicit PluginLoader(QObject* parent = nullptr);
    ~PluginLoader() override;

    bool loadPlugin(const QString& filePath);
    bool unloadPlugin(const QString& pluginName);
    QObject* pluginInstance(const QString& pluginName) const;

    QStringList loadedPlugins() const;

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

// 主守护进程
class Daemon : public QObject {
    Q_OBJECT
public:
    explicit Daemon(QObject* parent = nullptr);
    ~Daemon() override;

    bool init(int& argc, char** argv);
    int run();
    void shutdown();

    // 访问器
    ServiceManager& serviceManager();
    PluginLoader& pluginLoader();
    explorer::layer::LayerShellManager& layerShellManager();
    explorer::core::ExplorerModel& explorerModel();
    explorer::core::SignalDispatcher& signalDispatcher();
    WindowManager& windowManager();

signals:
    void initialized();
    void startingUp();
    void shuttingDown();

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::daemon

#include "Daemon.moc"