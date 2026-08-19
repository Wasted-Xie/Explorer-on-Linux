#include "DBusObject.h"
#include <QDBusConnection>
#include <QDebug>

namespace explorer::ipc {

DBusObject::DBusObject(QObject* parent) : QObject(parent) {}
DBusObject::~DBusObject() {
    unexportFromBus();
}

bool DBusObject::exportToBus(const QString& serviceName, const QString& objectPath,
                           const QDBusConnection& connection) {
    if (!connection.isConnected()) {
        qWarning() << "DBus connection not available";
        return false;
    }

    m_serviceName = serviceName;
    m_objectPath = objectPath;
    m_connection = connection;

    // 创建适配器
    m_adaptor = std::make_unique<Adaptor>(this);

    // 注册服务名（如果还没有注册的话）
    if (!m_connection.registerService(serviceName)) {
        if (m_connection.lastError().type() == QDBusError::NameExists) {
            // 服务已存在，继续尝试注册对象
            qDebug() << "Service" << serviceName << "already exists, continuing...";
        } else {
            qWarning() << "Failed to register service" << serviceName << ":" << m_connection.lastError().message();
            return false;
        }
    }

    // 注册对象
    if (!m_connection.registerObject(objectPath, this)) {
        qWarning() << "Failed to register object at" << objectPath << ":" << m_connection.lastError().message();
        return false;
    }

    m_exported = true;
    return true;
}

void DBusObject::unexportFromBus() {
    if (!m_exported || !m_connection.isConnected()) return;

    m_connection.unregisterObject(m_objectPath);
    // 不要注销服务名，因为其他对象可能还在使用它
    m_exported = false;
}

bool DBusObject::isExported() const {
    return m_exported;
}

QString DBusObject::serviceName() const {
    return m_serviceName;
}

QString DBusObject::objectPath() const {
    return m_objectPath;
}

QDBusConnection DBusObject::connection() const {
    return m_connection;
}

} // namespace explorer::ipc

#include "DBusObject.moc"