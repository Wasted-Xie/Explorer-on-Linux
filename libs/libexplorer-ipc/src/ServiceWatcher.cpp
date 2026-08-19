#include "ServiceWatcher.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>

namespace explorer::ipc {

class ServiceWatcher::Impl {
public:
    Impl(ServiceWatcher* owner) : q(owner) {
        connection = QDBusConnection::sessionBus();
        if (connection.isConnected()) {
            // 监视服务出现和消失的信号
            connection.connect("org.freedesktop.DBus",
                             "/org/freedesktop/DBus",
                             "org.freedesktop.DBus",
                             "NameOwnerChanged",
                             this, SLOT(onNameOwnerChanged(const QString&, const QString&, const QString&)));
        }
    }

    void onNameOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner) {
        // 只关注我们在监视的服务
        QMutexLocker lock(&mutex);
        if (watchedServices.contains(name)) {
            bool wasAvailable = !oldOwner.isEmpty();  // 之前有所有者表示可用
            bool isAvailable = !newOwner.isEmpty();   // 现在有所有者表示可用

            if (!wasAvailable && isAvailable) {
                // 服务出现
                emit q->serviceAppeared(name);
            } else if (wasAvailable && !isAvailable) {
                // 服务消失
                emit q->serviceVanished(name);
            }
        }
    }

    ServiceWatcher* q;
    QDBusConnection connection;
    QMutex mutex;
    QSet<QString> watchedServices;
};

ServiceWatcher::ServiceWatcher(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
ServiceWatcher::~ServiceWatcher() = default;

bool ServiceWatcher::watchService(const QString& serviceName) {
    if (!d->connection.isConnected()) return false;

    QMutexLocker lock(&d->mutex);
    d->watchedServices.insert(serviceName);
    return true;
}

void ServiceWatcher::unwatchService(const QString& serviceName) {
    QMutexLocker lock(&d->mutex);
    d->watchedServices.remove(serviceName);
}

void ServiceWatcher::unwatchAll() {
    QMutexLocker lock(&d->mutex);
    d->watchedServices.clear();
}

bool ServiceWatcher::isServiceAvailable(const QString& serviceName) const {
    if (!d->connection.isConnected()) return false;

    QDBusInterface interface("org.freedesktop.DBus",
                           "/org/freedesktop/DBus",
                           "org.freedesktop.DBus",
                           d->connection);
    if (!interface.isValid()) return false;

    QDBusReply<QStringList> reply = interface.call("ListNames");
    if (!reply.isValid()) return false;

    return reply.value().contains(serviceName);
}

} // namespace explorer::ipc

#include "ServiceWatcher.moc"