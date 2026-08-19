#include "Settings.h"
#include "Config.h"
#include <QSize>
#include <QPoint>
#include <QColor>

namespace explorer::core {

Settings::Settings(const QString& group) : m_group(group) {}

template<typename T>
T Settings::get(const QString& key, const T& defaultValue) const {
    return Config::instance().value(m_group, key, QVariant::fromValue(defaultValue)).value<T>();
}

template<typename T>
void Settings::set(const QString& key, const T& value) {
    Config::instance().setValue(m_group, key, QVariant::fromValue(value));
}

bool Settings::contains(const QString& key) const {
    return Config::instance().contains(m_group, key);
}

void Settings::remove(const QString& key) {
    Config::instance().remove(m_group, key);
}

void Settings::clear() {
    Config::instance().clearGroup(m_group);
}

QStringList Settings::allKeys() const {
    return Config::instance().allKeys(m_group);
}

QStringList Settings::childGroups() const {
    return Config::instance().childGroups(m_group);
}

// 显式实例化常用类型
template QString Settings::get<QString>(const QString&, const QString&) const;
template int Settings::get<int>(const QString&, int) const;
template bool Settings::get<bool>(const QString&, bool) const;
template double Settings::get<double>(const QString&, double) const;
template QSize Settings::get<QSize>(const QString&, const QSize&) const;
template QPoint Settings::get<QPoint>(const QString&, const QPoint&) const;
template QColor Settings::get<QColor>(const QString&, const QColor&) const;
template QStringList Settings::get<QStringList>(const QString&, const QStringList&) const;

template void Settings::set<QString>(const QString&, const QString&);
template void Settings::set<int>(const QString&, int);
template void Settings::set<bool>(const QString&, bool);
template void Settings::set<double>(const QString&, double);
template void Settings::set<QSize>(const QString&, const QSize&);
template void Settings::set<QPoint>(const QString&, const QPoint&);
template void Settings::set<QColor>(const QString&, const QColor&);
template void Settings::set<QStringList>(const QString&, const QStringList&);

// 便捷方法
QString Settings::getString(const QString& key, const QString& defaultValue) const {
    return get<QString>(key, defaultValue);
}

int Settings::getInt(const QString& key, int defaultValue) const {
    return get<int>(key, defaultValue);
}

bool Settings::getBool(const QString& key, bool defaultValue) const {
    return get<bool>(key, defaultValue);
}

double Settings::getDouble(const QString& key, double defaultValue) const {
    return get<double>(key, defaultValue);
}

QSize Settings::getSize(const QString& key, const QSize& defaultValue) const {
    return get<QSize>(key, defaultValue);
}

QPoint Settings::getPoint(const QString& key, const QPoint& defaultValue) const {
    return get<QPoint>(key, defaultValue);
}

QColor Settings::getColor(const QString& key, const QColor& defaultValue) const {
    return get<QColor>(key, defaultValue);
}

QStringList Settings::getStringList(const QString& key, const QStringList& defaultValue) const {
    return get<QStringList>(key, defaultValue);
}

void Settings::setString(const QString& key, const QString& value) {
    set<QString>(key, value);
}

void Settings::setInt(const QString& key, int value) {
    set<int>(key, value);
}

void Settings::setBool(const QString& key, bool value) {
    set<bool>(key, value);
}

void Settings::setDouble(const QString& key, double value) {
    set<double>(key, value);
}

void Settings::setSize(const QString& key, const QSize& value) {
    set<QSize>(key, value);
}

void Settings::setPoint(const QString& key, const QPoint& value) {
    set<QPoint>(key, value);
}

void Settings::setColor(const QString& key, const QColor& value) {
    set<QColor>(key, value);
}

void Settings::setStringList(const QString& key, const QStringList& value) {
    set<QStringList>(key, value);
}

Settings Settings::childGroup(const QString& name) const {
    QString newGroup = m_group.isEmpty() ? name : (m_group + "/" + name);
    return Settings(newGroup);
}

void Settings::sync() {
    Config::instance().sync();
}

} // namespace explorer::core