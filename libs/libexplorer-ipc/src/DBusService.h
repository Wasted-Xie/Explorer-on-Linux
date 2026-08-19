#pragma once

#include <QObject>
#include <QString>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusAbstractAdaptor>
#include <QDBusAbstractInterface>
#include <QMap>
#include <functional>

namespace explorer::ipc {

// DBus 服务基类
class DBusService : public QObject {
    Q_OBJECT
public:
    explicit DBusService(const QString& serviceName, QObject* parent = nullptr);
    virtual ~DBusService();

    // 启动服务
    bool start();
    void stop();

    // 检查服务是否存在
    static bool exists(const QString& serviceName);
    static bool isRunning(const QString& serviceName);

    // 获取连接
    QDBusConnection connection() const;

    // 注册对象
    bool registerObject(const QString& path, QObject* object);
    bool unregisterObject(const QString& path);

    // 发送信号
    template<typename... Args>
    void emitSignal(const QString& interface, const QString& signalName, Args&&... args);

protected:
    QString m_serviceName;
    QDBusConnection m_connection;
    QMap<QString, QObject*> m_registeredObjects;
};

// DBus 接口适配器 - 用于导出 Qt 对象作为 DBus 接口
template<typename Interface>
class DBusAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", Interface::staticInterfaceName())
public:
    explicit DBusAdaptor(QObject* parent) : QDBusAbstractAdaptor(parent) {}

    // 导出方法
    Q_SCRIPTABLE Q_NOREPLY void methodCall(const QString& method, const QVariantList& args) {
        QMetaObject::invokeMethod(parent(), method.toUtf8().constData(),
                                  Q_RETURN_ARG(QVariant, result),
                                  Q_ARG(QVariantList, args));
    }
};

} // namespace explorer::ipc