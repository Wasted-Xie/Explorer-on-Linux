#include "DBusInterface.h"
#include <QDebug>
#include <QDBusArgument>

namespace explorer::ipc {

DBusInterface::DBusInterface(const QString& service, const QString& path,
                            const QString& interface,
                            const QDBusConnection& connection,
                            QObject* parent)
    : QObject(parent)
{
    QString iface = interface;
    if (iface.isEmpty()) {
        // 如果没有指定接口，使用服务名作为接口名（常见模式）
        iface = service;
    }
    m_interface = QDBusInterface(service, path, iface, connection, this);
}

DBusInterface::~DBusInterface() = default;

bool DBusInterface::isValid() const {
    return m_interface.isValid();
}

template<typename... Args>
QDBusReply<QVariant> DBusInterface::call(const QString& method, Args&&... args) {
    if (!m_interface.isValid()) {
        return QDBusReply<QVariant>(QDBusError::NotSupported, "Invalid DBus interface");
    }

    QVariantList argList = {QVariant::fromValue(std::forward<Args>(args))...};
    return m_interface.call(method, argList);
}

template<typename... Args>
QDBusReply<void> DBusInterface::callNoReply(const QString& method, Args&&... args) {
    if (!m_interface.isValid()) {
        return QDBusReply<void>(QDBusError::NotSupported, "Invalid DBus interface");
    }

    QVariantList argList = {QVariant::fromValue(std::forward<Args>(args))...};
    return m_interface.callWithCallback(QDBus::NoBlock, method, argList);
}

QDBusReply<QVariant> DBusInterface::getProperty(const QString& propertyName) {
    if (!m_interface.isValid()) {
        return QDBusReply<QVariant>(QDBusError::NotSupported, "Invalid DBus interface");
    }
    return m_interface.property(propertyName.toUtf8().constData());
}

QDBusReply<void> DBusInterface::setProperty(const QString& propertyName, const QVariant& value) {
    if (!m_interface.isValid()) {
        return QDBusReply<void>(QDBusError::NotSupported, "Invalid DBus interface");
    }
    return m_interface.setProperty(propertyName.toUtf8().constData(), value);
}

// 通用模板实现 - 尝试解析参数并调用 func
template<typename Func>
QMetaObject::Connection DBusInterface::connectSignal(const QString& signalName, Func&& func) {
    if (!m_interface.isValid()) {
        return QMetaObject::Connection();
    }
    return QObject::connect(&m_interface, &QDBusInterface::signalCalled,
                           this, [func, signalName](const QString& name, const QDBusArgument& args) {
        if (name != signalName) return;
        
        // 尝试解析参数并调用 func
        // 这里需要根据 Func 的签名来解析参数
        // 由于模板类型擦除的限制，我们只能处理已知的几种常见情况
        // 实际调用会通过下面的重载来处理
    });
}

// 专用重载实现

// 无参数
QMetaObject::Connection DBusInterface::connectSignal(const QString& signalName, std::function<void()> func) {
    if (!m_interface.isValid()) return QMetaObject::Connection();
    return QObject::connect(&m_interface, &QDBusInterface::signalCalled,
                           this, [func, signalName](const QString& name, const QDBusArgument&) {
        if (name == signalName) func();
    });
}

// 单 QString 参数
QMetaObject::Connection DBusInterface::connectSignal(const QString& signalName, std::function<void(const QString&)> func) {
    if (!m_interface.isValid()) return QMetaObject::Connection();
    return QObject::connect(&m_interface, &QDBusInterface::signalCalled,
                           this, [func, signalName](const QString& name, const QDBusArgument& args) {
        if (name != signalName) return;
        QDBusArgument a = args;
        if (a.currentType() == QDBusArgument::BasicType) {
            QString value;
            a.beginStructure(); // 信号参数通常作为结构体传递
            a >> value;
            a.endStructure();
            func(value);
        }
    });
}

// 两个 QString 参数
QMetaObject::Connection DBusInterface::connectSignal(const QString& signalName, std::function<void(const QString&, const QString&)> func) {
    if (!m_interface.isValid()) return QMetaObject::Connection();
    return QObject::connect(&m_interface, &QDBusInterface::signalCalled,
                           this, [func, signalName](const QString& name, const QDBusArgument& args) {
        if (name != signalName) return;
        QDBusArgument a = args;
        if (a.currentType() == QDBusArgument::StructureType) {
            QString arg1, arg2;
            a.beginStructure();
            a >> arg1 >> arg2;
            a.endStructure();
            func(arg1, arg2);
        }
    });
}

// QString + QVariantMap
QMetaObject::Connection DBusInterface::connectSignal(const QString& signalName, std::function<void(const QString&, const QVariantMap&)> func) {
    if (!m_interface.isValid()) return QMetaObject::Connection();
    return QObject::connect(&m_interface, &QDBusInterface::signalCalled,
                           this, [func, signalName](const QString& name, const QDBusArgument& args) {
        if (name != signalName) return;
        QDBusArgument a = args;
        if (a.currentType() == QDBusArgument::StructureType) {
            QString arg1;
            QVariantMap arg2;
            a.beginStructure();
            a >> arg1 >> arg2;
            a.endStructure();
            func(arg1, arg2);
        }
    });
}

// QString + QVariantList
QMetaObject::Connection DBusInterface::connectSignal(const QString& signalName, std::function<void(const QString&, const QVariantList&)> func) {
    if (!m_interface.isValid()) return QMetaObject::Connection();
    return QObject::connect(&m_interface, &QDBusInterface::signalCalled,
                           this, [func, signalName](const QString& name, const QDBusArgument& args) {
        if (name != signalName) return;
        QDBusArgument a = args;
        if (a.currentType() == QDBusArgument::StructureType) {
            QString arg1;
            QVariantList arg2;
            a.beginStructure();
            a >> arg1 >> arg2;
            a.endStructure();
            func(arg1, arg2);
        }
    });
}

// QVariantList
QMetaObject::Connection DBusInterface::connectSignal(const QString& signalName, std::function<void(const QVariantList&)> func) {
    if (!m_interface.isValid()) return QMetaObject::Connection();
    return QObject::connect(&m_interface, &QDBusInterface::signalCalled,
                           this, [func, signalName](const QString& name, const QDBusArgument& args) {
        if (name != signalName) return;
        QDBusArgument a = args;
        if (a.currentType() == QDBusArgument::StructureType) {
            QVariantList value;
            a.beginStructure();
            a >> value;
            a.endStructure();
            func(value);
        } else if (a.currentType() == QDBusArgument::ArrayType) {
            QVariantList value;
            a >> value;
            func(value);
        }
    });
}

// QVariantMap
QMetaObject::Connection DBusInterface::connectSignal(const QString& signalName, std::function<void(const QVariantMap&)> func) {
    if (!m_interface.isValid()) return QMetaObject::Connection();
    return QObject::connect(&m_interface, &QDBusInterface::signalCalled,
                           this, [func, signalName](const QString& name, const QDBusArgument& args) {
        if (name != signalName) return;
        QDBusArgument a = args;
        if (a.currentType() == QDBusArgument::StructureType) {
            QVariantMap value;
            a.beginStructure();
            a >> value;
            a.endStructure();
            func(value);
        } else if (a.currentType() == QDBusArgument::MapType) {
            QVariantMap value;
            a >> value;
            func(value);
        }
    });
}

// 显式实例化常用的调用类型
template QDBusReply<QVariant> DBusInterface::call<>(const QString&);
template QDBusReply<QVariant> DBusInterface::call<int>(const QString&, int);
template QDBusReply<QVariant> DBusInterface::call<QString>(const QString&, QString);
template QDBusReply<QVariant> DBusInterface::call<int, QString>(const QString&, int, QString);
template QDBusReply<QVariant> DBusInterface::call<QString, QString>(const QString&, QString, QString);
template QDBusReply<QVariant> DBusInterface::call<bool>(const QString&, bool);
template QDBusReply<QVariant> DBusInterface::call<int, int>(const QString&, int, int);
template QDBusReply<QVariant> DBusInterface::call<QString, QVariantMap>(const QString&, QString, QVariantMap);

template QDBusReply<void> DBusInterface::callNoReply<>(const QString&);
template QDBusReply<void> DBusInterface::callNoReply<int>(const QString&, int);
template QDBusReply<void> DBusInterface::callNoReply<QString>(const QString&, QString);
template QDBusReply<void> DBusInterface::callNoReply<int, QString>(const QString&, int, QString);
template QDBusReply<void> DBusInterface::callNoReply<QString, QString>(const QString&, QString, QString);
template QDBusReply<void> DBusInterface::callNoReply<bool>(const QString&, bool);
template QDBusReply<void> DBusInterface::callNoReply<int, int>(const QString&, int, int);

} // namespace explorer::ipc

#include "DBusInterface.moc"