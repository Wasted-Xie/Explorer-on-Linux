#include "Config.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QCoreApplication>

namespace explorer::core {

class Config::Impl {
public:
    Impl() {
        // 确保配置目录存在
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        QString appDir = configDir + "/explorer-linux";
        QDir().mkpath(appDir);

        settings = std::make_unique<QSettings>(appDir + "/config.ini", QSettings::IniFormat);
    }

    std::unique_ptr<QSettings> settings;
    mutable QMutex mutex;
};

Config& Config::instance() {
    static Config inst;
    return inst;
}

Config::Config() : d(std::make_unique<Impl>()) {}
Config::~Config() = default;

void Config::setValue(const QString& key, const QVariant& value) {
    setValue("", key, value);
}

void Config::setValue(const QString& group, const QString& key, const QVariant& value) {
    QMutexLocker lock(&d->mutex);
    if (!group.isEmpty()) {
        d->settings->beginGroup(group);
    }
    d->settings->setValue(key, value);
    if (!group.isEmpty()) {
        d->settings->endGroup();
    }
}

QVariant Config::value(const QString& key, const QVariant& defaultValue) const {
    return value("", key, defaultValue);
}

QVariant Config::value(const QString& group, const QString& key, const QVariant& defaultValue) const {
    QMutexLocker lock(&d->mutex);
    if (!group.isEmpty()) {
        d->settings->beginGroup(group);
    }
    QVariant result = d->settings->value(key, defaultValue);
    if (!group.isEmpty()) {
        d->settings->endGroup();
    }
    return result;
}

bool Config::contains(const QString& key) const {
    return contains("", key);
}

bool Config::contains(const QString& group, const QString& key) const {
    QMutexLocker lock(&d->mutex);
    if (!group.isEmpty()) {
        d->settings->beginGroup(group);
    }
    bool result = d->settings->contains(key);
    if (!group.isEmpty()) {
        d->settings->endGroup();
    }
    return result;
}

void Config::remove(const QString& key) {
    remove("", key);
}

void Config::remove(const QString& group, const QString& key) {
    QMutexLocker lock(&d->mutex);
    if (!group.isEmpty()) {
        d->settings->beginGroup(group);
    }
    d->settings->remove(key);
    if (!group.isEmpty()) {
        d->settings->endGroup();
    }
}

void Config::clearGroup(const QString& group) {
    QMutexLocker lock(&d->mutex);
    d->settings->beginGroup(group);
    d->settings->remove("");
    d->settings->endGroup();
}

void Config::sync() {
    QMutexLocker lock(&d->mutex);
    d->settings->sync();
}

void Config::reload() {
    QMutexLocker lock(&d->mutex);
    // QSettings 自动重新加载，这里只需同步
    d->settings->sync();
}

QStringList Config::allKeys(const QString& group) const {
    QMutexLocker lock(&d->mutex);
    if (!group.isEmpty()) {
        d->settings->beginGroup(group);
    }
    QStringList keys = d->settings->allKeys();
    if (!group.isEmpty()) {
        d->settings->endGroup();
    }
    return keys;
}

QStringList Config::childGroups(const QString& group) const {
    QMutexLocker lock(&d->mutex);
    if (!group.isEmpty()) {
        d->settings->beginGroup(group);
    }
    QStringList groups = d->settings->childGroups();
    if (!group.isEmpty()) {
        d->settings->endGroup();
    }
    return groups;
}

} // namespace explorer::core