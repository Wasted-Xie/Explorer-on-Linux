#include "ServiceManager.h"
#include <QPluginLoader>
#include <QDebug>

namespace explorer::daemon {

class ServiceManager::Impl {
public:
    Impl(ServiceManager* owner) : q(owner) {}

    bool init() {
        return true;
    }

    void shutdown() {
        // 清理所有服务
        m_services.clear();
    }

    bool registerService(const QString& name, QObject* service) {
        if (!service) return false;
        if (m_services.contains(name)) {
            qWarning() << "Service already registered:" << name;
            return false;
        }
        m_services[name] = service;
        emit q->serviceRegistered(name);
        return true;
    }

    QObject* service(const QString& name) const {
        auto it = m_services.find(name);
        return it != m_services.end() ? it.value() : nullptr;
    }

    bool unregisterService(const QString& name) {
        auto it = m_services.find(name);
        if (it == m_services.end()) return false;
        m_services.erase(it);
        emit q->serviceUnregistered(name);
        return true;
    }

    bool loadPlugin(const QString& path) {
        QPluginLoader loader(path);
        QObject* plugin = loader.instance();
        if (!plugin) {
            qWarning() << "Failed to load plugin" << path << ":" << loader.errorString();
            return false;
        }

        QStringList interfaces = plugin->pluginMetaData().value("IIDs").toArray().toVariantList();
        // 这里可以根据接口类型进行不同的处理
        // 简化处理：我们假设所有插件都提供一个服务接口

        // 生成插件名称
        QString pluginName = QFileInfo(path).baseName();
        if (registerService(pluginName, plugin)) {
            m_pluginLoaders[pluginName] = std::make_unique<QPluginLoader>(loader);
            emit q->pluginLoaded(pluginName);
            return true;
        }

        return false;
    }

    bool unloadPlugin(const QString& name) {
        auto it = m_pluginLoaders.find(name);
        if (it == m_pluginLoaders.end()) return false;

        // 先取消注册服务
        unregisterService(name);

        // 然后卸载插件
        it->second->unload();
        m_pluginLoaders.erase(it);

        emit q->pluginUnloaded(name);
        return true;
    }

    ServiceManager* q;
    QMap<QString, QObject*> m_services;
    QMap<QString, std::unique_ptr<QPluginLoader>> m_pluginLoaders;
};

ServiceManager::ServiceManager(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
ServiceManager::~ServiceManager() {
    shutdown();
}

bool ServiceManager::init() {
    return d->init();
}

void ServiceManager::shutdown() {
    d->shutdown();
}

bool ServiceManager::registerService(const QString& name, QObject* service) {
    return d->registerService(name, service);
}

QObject* ServiceManager::service(const QString& name) const {
    return d->service(name);
}

bool ServiceManager::unregisterService(const QString& name) {
    return d->unregisterService(name);
}

bool ServiceManager::loadPlugin(const QString& path) {
    return d->loadPlugin(path);
}

bool ServiceManager::unloadPlugin(const QString& name) {
    return d->unloadPlugin(name);
}

} // namespace explorer::daemon

#include "ServiceManager.moc"