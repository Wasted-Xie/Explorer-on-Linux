#pragma once

#include <QString>
#include <QVariant>
#include <QMap>
#include <QMutex>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <functional>

namespace explorer::core {

// 类注册表 - 类似 Windows 注册表的键值存储，支持层级结构
class Registry : public QObject {
    Q_OBJECT
public:
    static Registry& instance();

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    // 设置值
    void setValue(const QString& path, const QVariant& value);
    QVariant value(const QString& path, const QVariant& defaultValue = {}) const;

    // 检查键/路径是否存在
    bool contains(const QString& path) const;
    bool hasSubKeys(const QString& path) const;

    // 获取子键/值名称
    QStringList subKeys(const QString& path) const;
    QStringList valueNames(const QString& path) const;

    // 删除
    void remove(const QString& path);
    void removeValue(const QString& path, const QString& valueName);

    // 监听变化
    using ChangeCallback = std::function<void(const QString& path, const QString& valueName, const QVariant& newValue)>;
    int watch(const QString& path, ChangeCallback callback);
    void unwatch(int watchId);

    // 导入/导出
    bool exportToFile(const QString& filePath) const;
    bool importFromFile(const QString& filePath);

    // 清空
    void clear();

signals:
    void valueChanged(const QString& path, const QString& valueName, const QVariant& newValue);

private:
    Registry();
    ~Registry() override;

    struct Node {
        QMap<QString, QVariant> values;
        QMap<QString, std::unique_ptr<Node>> children;
        QVector<std::pair<int, ChangeCallback>> watchers;
        int nextWatcherId = 1;
    };

    std::unique_ptr<Node> m_root;
    mutable QMutex m_mutex;

    Node* findNode(const QString& path, bool create = false) const;
    QStringList splitPath(const QString& path) const;
    void notifyWatchers(Node* node, const QString& fullPath, const QString& valueName, const QVariant& value);
};

} // namespace explorer::core