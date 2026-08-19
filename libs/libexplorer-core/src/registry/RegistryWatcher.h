#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <functional>

namespace explorer::core {

// 文件系统监视器 - 监视配置文件/注册表文件变化
class RegistryWatcher : public QObject {
    Q_OBJECT
public:
    explicit RegistryWatcher(QObject* parent = nullptr);
    ~RegistryWatcher() override;

    // 添加监视路径
    bool addPath(const QString& path);
    void removePath(const QString& path);
    void removeAllPaths();

    // 设置回调
    using ChangeCallback = std::function<void(const QString& path, const QString& key, const QVariant& value)>;
    void setChangeCallback(ChangeCallback callback);

signals:
    void fileChanged(const QString& path);
    void keyChanged(const QString& path, const QString& key, const QVariant& value);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::core