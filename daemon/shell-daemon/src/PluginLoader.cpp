#include "PluginLoader.h"
#include <QPluginLoader>
#include <QDir>
#include <QDebug>

namespace explorer::daemon {

class PluginLoader::Impl {
public:
    Impl(PluginLoader* owner) : q(owner) {}

    bool init() {
        return true;
    }

    void shutdown() {
        // 卸载所有插件
        for (auto it = m_loadedPlugins.begin(); it != m_loadedPlugins.end(); ++it) {
            it.value()->unload();
        }
        m_loadedPlugins.clear();
        m_pluginInstances.clear();
    }

    bool loadPlugin(const QString& filePath) {
        QFileInfo info(filePath);
        if (!info.exists() || !info.isFile()) {
            qWarning() << "Plugin file does not exist:" << filePath;
            return false;
        }

        QPluginLoader loader(filePath);
        QObject* plugin = loader.instance();
        if (!plugin) {
            qWarning() << "Failed to load plugin" << filePath << ":" << loader.errorString();
            return false;
        }

        // 获取插件元数据
        QJsonObject meta = loader.metaData();
        QString pluginName = meta.value("Name").toString();
        if (pluginName.isEmpty()) {
            pluginName = info.baseName();
        }

        // 检查是否已经加载
        if (m_loadedPlugins.contains(pluginName)) {
            qWarning() << "Plugin already loaded:" << pluginName;
            return false;
        }

        m_loadedPlugins[pluginName] = std::make_unique<QPluginLoader>(loader);
        m_pluginInstances[pluginName] = plugin;

        emit q->pluginLoaded(pluginName);
        return true;
    }

    bool unloadPlugin(const QString& pluginName) {
        auto it = m_loadedPlugins.find(pluginName);
        if (it == m_loadedPlugins.end()) return false;

        it->value()->unload();
        m_loadedPlugins.erase(it);
        m_pluginInstances.erase(pluginName);

        emit q->pluginUnloaded(pluginName);
        return true;
    }

    QObject* pluginInstance(const QString& pluginName) const {
        auto it = m_pluginInstances.find(pluginName);
        return it != m_pluginInstances.end() ? it.value() : nullptr;
    }

    QStringList loadedPlugins() const {
        QStringList result;
        for (auto it = m_loadedPlugins.begin(); it != m_loadedPlugins.end(); ++it) {
            result << it.key();
        }
        return result;
    }

    PluginLoader* q;
    QMap<QString, std::unique_ptr<QPluginLoader>> m_loadedPlugins;
    QMap<QString, QObject*> m_pluginInstances;
};

PluginLoader::PluginLoader(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
PluginLoader::~PluginLoader() {
    shutdown();
}

bool PluginLoader::init() {
    return d->init();
}

bool PluginLoader::loadPlugin(const QString& filePath) {
    return d->loadPlugin(filePath);
}

bool PluginLoader::unloadPlugin(const QString& pluginName) {
    return d->unloadPlugin(pluginName);
}

QObject* PluginLoader::pluginInstance(const QString& pluginName) const {
    return d->pluginInstance(pluginName);
}

QStringList PluginLoader::loadedPlugins() const {
    return d->loadedPlugins();
}

} // namespace explorer::daemon

#include "PluginLoader.moc"