#include "Registry.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace explorer::core {

Registry& Registry::instance() {
    static Registry inst;
    return inst;
}

Registry::Registry() : QObject(nullptr), m_root(std::make_unique<Node>()) {}
Registry::~Registry() = default;

Registry::Node* Registry::findNode(const QString& path, bool create) const {
    QMutexLocker lock(&m_mutex);
    Node* current = m_root.get();

    if (path.isEmpty() || path == "/") return current;

    QStringList parts = splitPath(path);
    for (const QString& part : parts) {
        auto it = current->children.find(part);
        if (it == current->children.end()) {
            if (!create) return nullptr;
            auto newNode = std::make_unique<Node>();
            Node* newNodePtr = newNode.get();
            current->children[part] = std::move(newNode);
            current = newNodePtr;
        } else {
            current = it->get();
        }
    }
    return current;
}

QStringList Registry::splitPath(const QString& path) const {
    QString normalized = path;
    if (normalized.startsWith('/')) normalized = normalized.mid(1);
    if (normalized.endsWith('/')) normalized.chop(1);
    if (normalized.isEmpty()) return {};
    return normalized.split('/', Qt::SkipEmptyParts);
}

void Registry::notifyWatchers(Node* node, const QString& fullPath, const QString& valueName, const QVariant& value) {
    for (auto& watcher : node->watchers) {
        try {
            watcher.second(fullPath, valueName, value);
        } catch (const std::exception& e) {
            qWarning() << "Registry watcher exception:" << e.what();
        }
    }
    emit valueChanged(fullPath, valueName, value);
}

void Registry::setValue(const QString& path, const QVariant& value) {
    Node* node = findNode(path, true);
    if (!node) return;

    QStringList parts = splitPath(path);
    QString valueName = parts.isEmpty() ? "default" : parts.last();
    QString parentPath = parts.size() <= 1 ? "/" : "/" + parts.mid(0, parts.size() - 1).join("/");

    QMutexLocker lock(&m_mutex);
    node->values[valueName] = value;
    notifyWatchers(node, parentPath + "/" + valueName, valueName, value);
}

QVariant Registry::value(const QString& path, const QVariant& defaultValue) const {
    QStringList parts = splitPath(path);
    if (parts.isEmpty()) return defaultValue;

    QString valueName = parts.last();
    QString parentPath = parts.size() <= 1 ? "/" : "/" + parts.mid(0, parts.size() - 1).join("/");

    Node* node = findNode(parentPath, false);
    if (!node) return defaultValue;

    QMutexLocker lock(&m_mutex);
    auto it = node->values.find(valueName);
    return it != node->values.end() ? *it : defaultValue;
}

bool Registry::contains(const QString& path) const {
    QStringList parts = splitPath(path);
    if (parts.isEmpty()) return true; // 根节点总是存在

    QString valueName = parts.last();
    QString parentPath = parts.size() <= 1 ? "/" : "/" + parts.mid(0, parts.size() - 1).join("/");

    Node* node = findNode(parentPath, false);
    if (!node) return false;

    QMutexLocker lock(&m_mutex);
    return node->values.contains(valueName) || node->children.contains(valueName);
}

bool Registry::hasSubKeys(const QString& path) const {
    Node* node = findNode(path, false);
    if (!node) return false;

    QMutexLocker lock(&m_mutex);
    return !node->children.isEmpty();
}

QStringList Registry::subKeys(const QString& path) const {
    Node* node = findNode(path, false);
    if (!node) return {};

    QMutexLocker lock(&m_mutex);
    return node->children.keys();
}

QStringList Registry::valueNames(const QString& path) const {
    Node* node = findNode(path, false);
    if (!node) return {};

    QMutexLocker lock(&m_mutex);
    return node->values.keys();
}

void Registry::remove(const QString& path) {
    QStringList parts = splitPath(path);
    if (parts.isEmpty()) {
        // 清空根节点
        QMutexLocker lock(&m_mutex);
        m_root->children.clear();
        m_root->values.clear();
        return;
    }

    QString valueName = parts.last();
    QString parentPath = parts.size() <= 1 ? "/" : "/" + parts.mid(0, parts.size() - 1).join("/");

    Node* parent = findNode(parentPath, false);
    if (!parent) return;

    QMutexLocker lock(&m_mutex);
    parent->children.remove(valueName);
    parent->values.remove(valueName);
}

void Registry::removeValue(const QString& path, const QString& valueName) {
    Node* node = findNode(path, false);
    if (!node) return;

    QMutexLocker lock(&m_mutex);
    node->values.remove(valueName);
    notifyWatchers(node, path, valueName, QVariant());
}

int Registry::watch(const QString& path, ChangeCallback callback) {
    Node* node = findNode(path, true);
    if (!node) return -1;

    QMutexLocker lock(&m_mutex);
    int id = node->nextWatcherId++;
    node->watchers.append({id, std::move(callback)});
    return id;
}

void Registry::unwatch(int watchId) {
    // 需要遍历所有节点查找 watcher
    // 简化实现：标记为无效，实际清理在下次触发时
    // 完整实现需要维护全局 watchId -> node 映射
    Q_UNUSED(watchId);
}

bool Registry::exportToFile(const QString& filePath) const {
    QMutexLocker lock(&m_mutex);

    auto nodeToJson = [](const Node* node, auto&& self) -> QJsonObject {
        QJsonObject obj;
        // 值
        for (auto it = node->values.constBegin(); it != node->values.constEnd(); ++it) {
            obj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        // 子节点
        QJsonObject childrenObj;
        for (auto it = node->children.constBegin(); it != node->children.constEnd(); ++it) {
            childrenObj[it.key()] = self(it->get(), self);
        }
        if (!childrenObj.isEmpty()) {
            obj["__children__"] = childrenObj;
        }
        return obj;
    };

    QJsonDocument doc(nodeToJson(m_root.get(), nodeToJson));
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool Registry::importFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) return false;

    auto jsonToNode = [](const QJsonObject& obj, Node* node, auto&& self) {
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (it.key() == "__children__") {
                if (it.value().isObject()) {
                    for (auto cit = it.value().toObject().constBegin(); cit != it.value().toObject().constEnd(); ++cit) {
                        auto child = std::make_unique<Node>();
                        self(cit.value().toObject(), child.get(), self);
                        node->children[cit.key()] = std::move(child);
                    }
                }
            } else {
                node->values[it.key()] = it.value().toVariant();
            }
        }
    };

    QMutexLocker lock(&m_mutex);
    m_root = std::make_unique<Node>();
    jsonToNode(doc.object(), m_root.get(), jsonToNode);
    return true;
}

void Registry::clear() {
    QMutexLocker lock(&m_mutex);
    m_root = std::make_unique<Node>();
}

} // namespace explorer::core