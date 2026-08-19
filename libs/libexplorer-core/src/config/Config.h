#pragma once

#include <QString>
#include <QVariant>
#include <QMap>
#include <QMutex>
#include <memory>

namespace explorer::core {

class Config final {
public:
    static Config& instance();

    // 非可复制、可移动
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    // 设置值
    void setValue(const QString& key, const QVariant& value);
    void setValue(const QString& group, const QString& key, const QVariant& value);

    // 获取值
    QVariant value(const QString& key, const QVariant& defaultValue = {}) const;
    QVariant value(const QString& group, const QString& key, const QVariant& defaultValue = {}) const;

    // 检查键是否存在
    bool contains(const QString& key) const;
    bool contains(const QString& group, const QString& key) const;

    // 移除键
    void remove(const QString& key);
    void remove(const QString& group, const QString& key);

    // 清空组
    void clearGroup(const QString& group);

    // 同步到磁盘
    void sync();

    // 重新加载
    void reload();

    // 获取所有键
    QStringList allKeys(const QString& group = {}) const;

    // 获取子组
    QStringList childGroups(const QString& group = {}) const;

private:
    Config();
    ~Config();

    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::core