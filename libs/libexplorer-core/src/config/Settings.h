#pragma once

#include "Config.h"
#include <QString>
#include <QVariant>

namespace explorer::core {

// 类型安全的设置访问器
class Settings {
public:
    explicit Settings(const QString& group = {});

    // 模板方法
    template<typename T>
    T get(const QString& key, const T& defaultValue = T{}) const;

    template<typename T>
    void set(const QString& key, const T& value);

    bool contains(const QString& key) const;
    void remove(const QString& key);
    void clear();

    QStringList allKeys() const;
    QStringList childGroups() const;

    // 常用类型的便捷方法
    QString getString(const QString& key, const QString& defaultValue = {}) const;
    int getInt(const QString& key, int defaultValue = 0) const;
    bool getBool(const QString& key, bool defaultValue = false) const;
    double getDouble(const QString& key, double defaultValue = 0.0) const;
    QSize getSize(const QString& key, const QSize& defaultValue = {}) const;
    QPoint getPoint(const QString& key, const QPoint& defaultValue = {}) const;
    QColor getColor(const QString& key, const QColor& defaultValue = {}) const;
    QStringList getStringList(const QString& key, const QStringList& defaultValue = {}) const;

    void setString(const QString& key, const QString& value);
    void setInt(const QString& key, int value);
    void setBool(const QString& key, bool value);
    void setDouble(const QString& key, double value);
    void setSize(const QString& key, const QSize& value);
    void setPoint(const QString& key, const QPoint& value);
    void setColor(const QString& key, const QColor& value);
    void setStringList(const QString& key, const QStringList& value);

    // 子组
    Settings childGroup(const QString& name) const;

    // 同步
    void sync();

private:
    QString m_group;
};

} // namespace explorer::core