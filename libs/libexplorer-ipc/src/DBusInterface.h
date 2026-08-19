#pragma once

#include <QObject>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <functional>

namespace explorer::ipc {

// DBus 接口包装 - 用于调用远程服务
class DBusInterface : public QObject {
    Q_OBJECT
public:
    explicit DBusInterface(const QString& service, const QString& path,
                          const QString& interface = "",
                          const QDBusConnection& connection = QDBusConnection::sessionBus(),
                          QObject* parent = nullptr);
    ~DBusInterface() override;

    // 检查服务是否可用
    bool isValid() const;

    // 调用方法
    template<typename... Args>
    QDBusReply<QVariant> call(const QString& method, Args&&... args);

    template<typename... Args>
    QDBusReply<void> callNoReply(const QString& method, Args&&... args);

    // 获取属性
    QDBusReply<QVariant> getProperty(const QString& propertyName);
    QDBusReply<void> setProperty(const QString& propertyName, const QVariant& value);

    // 监听信号 - 支持多种参数类型
    template<typename Func>
    QMetaObject::Connection connectSignal(const QString& signalName, Func&& func);

    // 监听信号 - 专用重载，支持常见参数组合
    // 无参数
    QMetaObject::Connection connectSignal(const QString& signalName, std::function<void()> func);
    // 单 QString 参数
    QMetaObject::Connection connectSignal(const QString& signalName, std::function<void(const QString&)> func);
    // 两个 QString 参数
    QMetaObject::Connection connectSignal(const QString& signalName, std::function<void(const QString&, const QString&)> func);
    // QString + QVariantMap
    QMetaObject::Connection connectSignal(const QString& signalName, std::function<void(const QString&, const QVariantMap&)> func);
    // QString + QVariantList
    QMetaObject::Connection connectSignal(const QString& signalName, std::function<void(const QString&, const QVariantList&)> func);
    // QVariantList
    QMetaObject::Connection connectSignal(const QString& signalName, std::function<void(const QVariantList&)> func);
    // QVariantMap
    QMetaObject::Connection connectSignal(const QString& signalName, std::function<void(const QVariantMap&)> func);

    // 直接访问底层接口
    QDBusInterface* interface() { return &m_interface; }
    const QDBusInterface* interface() const { return &m_interface; }

private:
    QDBusInterface m_interface;
};

// 构建函数
inline DBusInterface makeInterface(const QString& service, const QString& path,
                                  const QString& interface = "",
                                  const QDBusConnection& connection = QDBusConnection::sessionBus()) {
    return DBusInterface(service, path, interface, connection);
}

} // namespace explorer::ipc

#include "DBusInterface.moc"