#include "RegistryWatcher.h"
#include <QFileSystemWatcher>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDebug>

namespace explorer::core {

class RegistryWatcher::Impl {
public:
    Impl(RegistryWatcher* owner) : q(owner) {
        connect(&watcher, &QFileSystemWatcher::fileChanged, this, &Impl::onFileChanged);
    }

    void onFileChanged(const QString& path) {
        if (changeCallback) {
            // 简单实现：重新读取整个文件并通知所有键变化
            QFile file(path);
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                if (!doc.isNull() && doc.isObject()) {
                    processJsonObject(path, doc.object(), "");
                }
            }
        }
        emit q->fileChanged(path);
    }

    void processJsonObject(const QString& filePath, const QJsonObject& obj, const QString& prefix) {
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            QString fullKey = prefix.isEmpty() ? it.key() : prefix + "/" + it.key();
            if (it.value().isObject() && it.key() != "__children__") {
                processJsonObject(filePath, it.value().toObject(), fullKey);
            } else if (it.key() != "__children__") {
                changeCallback(filePath, fullKey, it.value().toVariant());
                emit q->keyChanged(filePath, fullKey, it.value().toVariant());
            }
        }
    }

    QFileSystemWatcher watcher;
    ChangeCallback changeCallback;
    RegistryWatcher* q;
};

RegistryWatcher::RegistryWatcher(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
RegistryWatcher::~RegistryWatcher() = default;

bool RegistryWatcher::addPath(const QString& path) {
    QFileInfo info(path);
    if (!info.exists()) {
        // 如果文件不存在，创建空文件
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write("{}");
        }
    }
    return d->watcher.addPath(path);
}

void RegistryWatcher::removePath(const QString& path) {
    d->watcher.removePath(path);
}

void RegistryWatcher::removeAllPaths() {
    d->watcher.removePaths(d->watcher.files());
}

void RegistryWatcher::setChangeCallback(ChangeCallback callback) {
    d->changeCallback = std::move(callback);
}

} // namespace explorer::core