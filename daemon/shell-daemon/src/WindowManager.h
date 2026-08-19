#pragma once

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QUuid>
#include <QDateTime>

#include <libexplorer-ipc/DBusService.h>
#include <libexplorer-core/SignalDispatcher.h>

namespace explorer::daemon {

// 窗口状态结构
struct WindowInfo {
    QString id;
    QString title;
    QString icon;       // 图标名称或路径
    QString appName;    // 应用程序名称
    bool isMinimized = false;
    bool isMaximized = false;
    bool isActive = false;
    quint64 timestamp = 0;  // 注册时间戳，用于排序

    WindowInfo() = default;
    WindowInfo(const QString& id_, const QString& title_, const QString& icon_,
               const QString& appName_, bool minimized, bool maximized, bool active)
        : id(id_), title(title_), icon(icon_), appName(appName_),
          isMinimized(minimized), isMaximized(maximized), isActive(active),
          timestamp(QDateTime::currentMSecsSinceEpoch()) {}

    // 转换为 QVariantMap 用于 DBus 传输
    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["id"] = id;
        map["title"] = title;
        map["icon"] = icon;
        map["appName"] = appName;
        map["isMinimized"] = isMinimized;
        map["isMaximized"] = isMaximized;
        map["isActive"] = isActive;
        map["timestamp"] = static_cast<qlonglong>(timestamp);
        return map;
    }

    // 从 QVariantMap 创建
    static WindowInfo fromVariantMap(const QVariantMap& map) {
        WindowInfo info;
        info.id = map.value("id").toString();
        info.title = map.value("title").toString();
        info.icon = map.value("icon").toString();
        info.appName = map.value("appName").toString();
        info.isMinimized = map.value("isMinimized").toBool();
        info.isMaximized = map.value("isMaximized").toBool();
        info.isActive = map.value("isActive").toBool();
        info.timestamp = static_cast<quint64>(map.value("timestamp").toLongLong());
        return info;
    }
};

// 窗口状态枚举（用于 SetWindowState）
struct WindowState {
    bool minimized = false;
    bool maximized = false;
    bool active = false;
    bool hasMinimized = false;
    bool hasMaximized = false;
    bool hasActive = false;

    WindowState() = default;
    WindowState(bool min, bool max, bool act)
        : minimized(min), maximized(max), active(act),
          hasMinimized(true), hasMaximized(true), hasActive(true) {}

    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["minimized"] = minimized;
        map["maximized"] = maximized;
        map["active"] = active;
        map["hasMinimized"] = hasMinimized;
        map["hasMaximized"] = hasMaximized;
        map["hasActive"] = hasActive;
        return map;
    }

    static WindowState fromVariantMap(const QVariantMap& map) {
        WindowState state;
        if (map.contains("minimized")) {
            state.minimized = map["minimized"].toBool();
            state.hasMinimized = true;
        }
        if (map.contains("maximized")) {
            state.maximized = map["maximized"].toBool();
            state.hasMaximized = true;
        }
        if (map.contains("active")) {
            state.active = map["active"].toBool();
            state.hasActive = true;
        }
        if (map.contains("hasMinimized")) state.hasMinimized = map["hasMinimized"].toBool();
        if (map.contains("hasMaximized")) state.hasMaximized = map["hasMaximized"].toBool();
        if (map.contains("hasActive")) state.hasActive = map["hasActive"].toBool();
        return state;
    }
};

/**
 * WindowManager 服务 - 管理顶层窗口列表
 * 
 * 提供 DBus 接口供组件注册/注销窗口，任务栏查询窗口列表
 * DBus 服务名: org.explorer.WindowManager
 * DBus 对象路径: /org/explorer/WindowManager
 * DBus 接口名: org.explorer.WindowManager
 */
class WindowManager : public QObject {
    Q_OBJECT

    // DBus 接口定义
    Q_CLASSINFO("D-Bus Interface", "org.explorer.WindowManager")

public:
    explicit WindowManager(QObject* parent = nullptr);
    ~WindowManager() override;

    // 初始化服务（连接 DBus，注册对象）
    bool init(const QDBusConnection& connection);
    void shutdown();

    // 检查是否已初始化
    bool isInitialized() const { return m_initialized; }

    // 窗口注册/注销（由应用程序调用）
    Q_INVOKABLE QString registerWindow(const QString& title, const QString& icon,
                                       const QString& appName,
                                       bool isMinimized = false,
                                       bool isMaximized = false,
                                       bool isActive = false);
    Q_INVOKABLE bool unregisterWindow(const QString& id);

    // 窗口状态更新
    Q_INVOKABLE bool setWindowState(const QString& id, const QVariantMap& state);
    Q_INVOKABLE bool setWindowTitle(const QString& id, const QString& title);
    Q_INVOKABLE bool setWindowIcon(const QString& id, const QString& icon);
    Q_INVOKABLE bool setWindowMinimized(const QString& id, bool minimized);
    Q_INVOKABLE bool setWindowMaximized(const QString& id, bool maximized);
    Q_INVOKABLE bool setWindowActive(const QString& id, bool active);

    // 查询接口（由任务栏等调用）
    Q_INVOKABLE QVariantList getWindowList() const;
    Q_INVOKABLE QString getActiveWindow() const;
    Q_INVOKABLE QVariantMap getWindowInfo(const QString& id) const;
    Q_INVOKABLE int getWindowCount() const;

    // 窗口操作（由任务栏等调用）
    Q_INVOKABLE bool activateWindow(const QString& id);
    Q_INVOKABLE bool minimizeWindow(const QString& id);
    Q_INVOKABLE bool maximizeWindow(const QString& id);
    Q_INVOKABLE bool closeWindow(const QString& id);

    // 信号：窗口列表变化
    Q_SIGNAL void windowRegistered(const QString& id, const QVariantMap& info);
    Q_SIGNAL void windowUnregistered(const QString& id);
    Q_SIGNAL void windowStateChanged(const QString& id, const QVariantMap& state);
    Q_SIGNAL void activeWindowChanged(const QString& id);
    Q_SIGNAL void windowListChanged();

    // 获取 DBus 连接（用于发送信号）
    QDBusConnection connection() const { return m_connection; }

private:
    // 内部辅助方法
    bool updateWindowState(const QString& id, const WindowState& state);
    void emitWindowListChanged();
    void emitActiveWindowChanged(const QString& id);
    QString generateWindowId();

    // 数据成员
    QDBusConnection m_connection;
    QMap<QString, WindowInfo> m_windows;
    QString m_activeWindowId;
    bool m_initialized = false;

    // DBus 服务引用（用于发送信号）
    explorer::ipc::DBusService* m_dbusService = nullptr;
};

} // namespace explorer::daemon

#include "WindowManager.moc"