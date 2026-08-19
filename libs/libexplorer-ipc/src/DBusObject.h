#pragma once

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QMap>
#include <functional>

namespace explorer::ipc {

// DBus 对象基类 - 可以导出为 DBus 对象
class DBusObject : public QObject {
    Q_OBJECT
public:
    explicit DBusObject(QObject* parent = nullptr);
    virtual ~DBusObject();

    // 导出到 DBus
    bool exportToBus(const QString& serviceName, const QString& objectPath,
                     const QDBusConnection& connection = QDBusConnection::sessionBus());
    void unexportFromBus();

    // 检查是否已导出
    bool isExported() const;
    QString serviceName() const;
    QString objectPath() const;
    QDBusConnection connection() const;

protected:
    // 子类重写此方法来定义 DBus 接口
    virtual QString dbusInterface() const { return ""; }
    virtual QString dbusIntrospection() const { return ""; }

private:
    class Adaptor : public QDBusAbstractAdaptor {
        Q_OBJECT
    public:
        Adaptor(DBusObject* parent) : QDBusAbstractAdaptor(parent) {}

        // 处理方法调用
        Q_SCRIPTABLE Q_NOREPLY void methodCall(const QString& method, const QVariantList& args) {
            // 尝试调用父对象的同名方法
            QMetaObject::invokeMethod(parent(), method.toUtf8().constData(),
                                    Q_RETURN_ARG(QVariant, result),
                                    Q_ARG(QVariantList, args));
        }

        // 处理属性访问
        Q_SCRIPTABLE QVariant property(const QString& name) const {
            QVariant result;
            QMetaObject::invokeMethod(parent(), ("get" + name).toUtf8().constData(),
                                    Q_RETURN_ARG(QVariant, result));
            return result;
        }

        Q_SCRIPTABLE void setProperty(const QString& name, const QVariant& value) {
            QMetaObject::invokeMethod(parent(), ("set" + name).toUtf8().constData(),
                                    Q_ARG(QVariant, value));
        }
    };

    QString m_serviceName;
    QString m_objectPath;
    QDBusConnection m_connection;
    bool m_exported = false;
    std::unique_ptr<Adaptor> m_adaptor;
};

} // namespace explorer::ipc

#include "DBusObject.moc"