#include "DBusService.h"
#include <QDBusConnection>
#include <QDebug>

namespace explorer::ipc {

DBusService::DBusService(const QString& serviceName, QObject* parent)
    : QObject(parent), m_serviceName(serviceName) {}

DBusService::~DBusService() {
    stop();
}

bool DBusService::start() {
    // 连接到会话总线
    m_connection = QDBusConnection::sessionBus();
    if (!m_connection.isConnected()) {
        qWarning() << "Failed to connect to session bus";
        return false;
    }

    // 尝试注册服务名
    if (!m_connection.registerService(m_serviceName)) {
        if (m_connection.lastError().type() == QDBusError::NameExists) {
            qWarning() << "Service" << m_serviceName << "already exists";
            return false;
        }
        qWarning() << "Failed to register service:" << m_connection.lastError().message();
        return false;
    }

    return true;
}

void DBusService::stop() {
    // 注销所有对象
    for (auto it = m_registeredObjects.begin(); it != m_registeredObjects.end(); ++it) {
        m_connection.unregisterObject(it.key());
    }
    m_registeredObjects.clear();

    // 注销服务
    if (m_connection.isConnected()) {
        m_connection.unregisterService(m_serviceName);
    }
}

bool DBusService::exists(const QString& serviceName) {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) return false;

    QDBusInterface interface("org.freedesktop.DBus",
                            "/org/freedesktop/DBus",
                            "org.freedesktop.DBus",
                            bus);
    if (!interface.isValid()) return false;

    QDBusReply<QStringList> reply = interface.call("ListNames");
    if (!reply.isValid()) return false;

    return reply.value().contains(serviceName);
}

bool DBusService::isRunning(const QString& serviceName) {
    return exists(serviceName);
}

QDBusConnection DBusService::connection() const {
    return m_connection;
}

bool DBusService::registerObject(const QString& path, QObject* object) {
    if (!m_connection.isConnected()) return false;
    if (!object) return false;

    bool success = m_connection.registerObject(path, object);
    if (success) {
        m_registeredObjects[path] = object;
    }
    return success;
}

bool DBusService::unregisterObject(const QString& path) {
    if (!m_connection.isConnected()) return false;

    bool success = m_connection.unregisterObject(path);
    if (success) {
        m_registeredObjects.remove(path);
    }
    return success;
}

template<typename... Args>
void DBusService::emitSignal(const QString& interface, const QString& signalName, Args&&... args) {
    if (!m_connection.isConnected()) return;

    QDBusMessage message = QDBusMessage::createSignal(
        "/",  // 对象路径 - 实际使用时应该更具体
        interface.toUtf8().constData(),
        signalName.toUtf8().constData()
    );

    // 添加参数
    QVariantList argList = {QVariant::fromValue(std::forward<Args>(args))...};
    message.setArguments(argList);

    m_connection.send(message);
}

// 显式实例化常用的信号类型
template void DBusService::emitSignal<>(const QString&, const QString&);
template void DBusService::emitSignal<int>(const QString&, const QString&, int);
template void DBusService::emitSignal<QString>(const QString&, const QString&, QString);
template void DBusService::emitSignal<int, QString>(const QString&, const QString&, int, QString);
template void DBusService::emitSignal<QString, QString>(const QString&, const QString&, QString, QString);
template void DBusService::emitSignal<bool>(const QString&, const QString&, bool);
template void DBusService::emitSignal<int, int>(const QString&, const QString&, int, int);

} // namespace explorer::ipc

// 必须在 .cpp 文件末尾包含 moc
#include "DBusService.moc"