#include "WindowManager.h"
#include <QDateTime>
#include <QDBusMessage>
#include <QRandomGenerator>

namespace explorer::daemon {

WindowManager::WindowManager(QObject* parent)
    : QObject(parent), m_dbusService(nullptr) {}

WindowManager::~WindowManager() {
    shutdown();
}

bool WindowManager::init(const QDBusConnection& connection) {
    if (m_initialized) {
        qWarning() << "WindowManager already initialized";
        return true;
    }

    m_connection = connection;
    if (!m_connection.isConnected()) {
        qWarning() << "Invalid DBus connection for WindowManager";
        return false;
    }

    // 导出对象到 DBus
    const QString objectPath = "/org/explorer/WindowManager";
    if (!m_connection.registerObject(objectPath, this,
                                      QDBusConnection::ExportScriptableSlots |
                                      QDBusConnection::ExportScriptableSignals)) {
        qWarning() << "Failed to register WindowManager object at" << objectPath;
        return false;
    }

    m_initialized = true;
    qInfo() << "WindowManager initialized at" << objectPath;
    return true;
}

void WindowManager::shutdown() {
    if (!m_initialized) return;

    // 注销所有窗口
    for (auto it = m_windows.begin(); it != m_windows.end(); ++it) {
        QString id = it.key();
        QDBusMessage signal = QDBusMessage::createSignal(
            "/org/explorer/WindowManager",
            "org.explorer.WindowManager",
            "windowUnregistered"
        );
        signal.setArguments({id});
        m_connection.send(signal);
    }
    m_windows.clear();
    m_activeWindowId.clear();

    // 注销 DBus 对象
    if (m_connection.isConnected()) {
        m_connection.unregisterObject("/org/explorer/WindowManager");
    }

    m_initialized = false;
    qInfo() << "WindowManager shutdown";
}

QString WindowManager::generateWindowId() {
    // 生成唯一的窗口 ID: "win_" + UUID 前 8 位 + 时间戳后 4 位
    QUuid uuid = QUuid::createUuid();
    QString uuidStr = uuid.toString(QUuid::WithoutBraces).remove('-').left(8);
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch() & 0xFFFF, 16).right(4);
    return QString("win_%1%2").arg(uuidStr).arg(timestamp);
}

QString WindowManager::registerWindow(const QString& title, const QString& icon,
                                      const QString& appName,
                                      bool isMinimized, bool isMaximized, bool isActive) {
    if (!m_initialized) {
        qWarning() << "WindowManager not initialized";
        return QString();
    }

    QString id = generateWindowId();
    
    WindowInfo info(id, title, icon, appName, isMinimized, isMaximized, isActive);
    m_windows[id] = info;

    // 如果是第一个窗口或指定为激活，设为激活窗口
    if (isActive || m_activeWindowId.isEmpty()) {
        // 取消之前的激活窗口
        if (!m_activeWindowId.isEmpty() && m_windows.contains(m_activeWindowId)) {
            m_windows[m_activeWindowId].isActive = false;
            QDBusMessage signal = QDBusMessage::createSignal(
                "/org/explorer/WindowManager",
                "org.explorer.WindowManager",
                "windowStateChanged"
            );
            signal.setArguments({m_activeWindowId, m_windows[m_activeWindowId].toVariantMap()});
            m_connection.send(signal);
        }
        m_activeWindowId = id;
        info.isActive = true;
        m_windows[id] = info;
        emitActiveWindowChanged(id);
    }

    // 发送窗口注册信号
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowRegistered"
    );
    signal.setArguments({id, info.toVariantMap()});
    m_connection.send(signal);

    // 发送本地信号
    emit windowRegistered(id, info.toVariantMap());
    emit windowListChanged();

    qInfo() << "Window registered:" << id << "title:" << title << "app:" << appName;
    return id;
}

bool WindowManager::unregisterWindow(const QString& id) {
    if (!m_initialized) {
        qWarning() << "WindowManager not initialized";
        return false;
    }

    if (!m_windows.contains(id)) {
        qWarning() << "Window not found:" << id;
        return false;
    }

    bool wasActive = (id == m_activeWindowId);
    m_windows.remove(id);

    // 如果注销的是激活窗口，选择新的激活窗口
    if (wasActive) {
        m_activeWindowId.clear();
        // 选择最新的窗口作为激活窗口（基于时间戳）
        quint64 latestTime = 0;
        for (const auto& info : m_windows) {
            if (info.timestamp > latestTime) {
                latestTime = info.timestamp;
                m_activeWindowId = info.id;
            }
        }
        if (!m_activeWindowId.isEmpty()) {
            m_windows[m_activeWindowId].isActive = true;
            emitActiveWindowChanged(m_activeWindowId);
            
            QDBusMessage signal = QDBusMessage::createSignal(
                "/org/explorer/WindowManager",
                "org.explorer.WindowManager",
                "windowStateChanged"
            );
            signal.setArguments({m_activeWindowId, m_windows[m_activeWindowId].toVariantMap()});
            m_connection.send(signal);
        }
    }

    // 发送窗口注销信号
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowUnregistered"
    );
    signal.setArguments({id});
    m_connection.send(signal);

    emit windowUnregistered(id);
    emit windowListChanged();

    qInfo() << "Window unregistered:" << id;
    return true;
}

bool WindowManager::updateWindowState(const QString& id, const WindowState& state) {
    if (!m_windows.contains(id)) {
        return false;
    }

    WindowInfo& info = m_windows[id];
    bool changed = false;

    if (state.hasMinimized && info.isMinimized != state.minimized) {
        info.isMinimized = state.minimized;
        changed = true;
    }
    if (state.hasMaximized && info.isMaximized != state.maximized) {
        info.isMaximized = state.maximized;
        changed = true;
    }
    if (state.hasActive && info.isActive != state.active) {
        // 处理激活状态变化
        if (state.active) {
            // 取消之前的激活窗口
            if (!m_activeWindowId.isEmpty() && m_activeWindowId != id && m_windows.contains(m_activeWindowId)) {
                m_windows[m_activeWindowId].isActive = false;
                QDBusMessage oldSignal = QDBusMessage::createSignal(
                    "/org/explorer/WindowManager",
                    "org.explorer.WindowManager",
                    "windowStateChanged"
                );
                oldSignal.setArguments({m_activeWindowId, m_windows[m_activeWindowId].toVariantMap()});
                m_connection.send(oldSignal);
            }
            m_activeWindowId = id;
            emitActiveWindowChanged(id);
        }
        info.isActive = state.active;
        changed = true;
    }

    if (changed) {
        info.timestamp = QDateTime::currentMSecsSinceEpoch();
        
        QDBusMessage signal = QDBusMessage::createSignal(
            "/org/explorer/WindowManager",
            "org.explorer.WindowManager",
            "windowStateChanged"
        );
        signal.setArguments({id, info.toVariantMap()});
        m_connection.send(signal);

        emit windowStateChanged(id, info.toVariantMap());
        emit windowListChanged();
    }

    return changed;
}

bool WindowManager::setWindowState(const QString& id, const QVariantMap& stateMap) {
    if (!m_initialized) {
        qWarning() << "WindowManager not initialized";
        return false;
    }

    WindowState state = WindowState::fromVariantMap(stateMap);
    return updateWindowState(id, state);
}

bool WindowManager::setWindowTitle(const QString& id, const QString& title) {
    if (!m_initialized || !m_windows.contains(id)) return false;
    if (m_windows[id].title == title) return false;

    m_windows[id].title = title;
    m_windows[id].timestamp = QDateTime::currentMSecsSinceEpoch();

    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowStateChanged"
    );
    signal.setArguments({id, m_windows[id].toVariantMap()});
    m_connection.send(signal);

    emit windowStateChanged(id, m_windows[id].toVariantMap());
    emit windowListChanged();
    return true;
}

bool WindowManager::setWindowIcon(const QString& id, const QString& icon) {
    if (!m_initialized || !m_windows.contains(id)) return false;
    if (m_windows[id].icon == icon) return false;

    m_windows[id].icon = icon;
    m_windows[id].timestamp = QDateTime::currentMSecsSinceEpoch();

    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowStateChanged"
    );
    signal.setArguments({id, m_windows[id].toVariantMap()});
    m_connection.send(signal);

    emit windowStateChanged(id, m_windows[id].toVariantMap());
    emit windowListChanged();
    return true;
}

bool WindowManager::setWindowMinimized(const QString& id, bool minimized) {
    if (!m_initialized) return false;
    WindowState state;
    state.hasMinimized = true;
    state.minimized = minimized;
    return updateWindowState(id, state);
}

bool WindowManager::setWindowMaximized(const QString& id, bool maximized) {
    if (!m_initialized) return false;
    WindowState state;
    state.hasMaximized = true;
    state.maximized = maximized;
    return updateWindowState(id, state);
}

bool WindowManager::setWindowActive(const QString& id, bool active) {
    if (!m_initialized) return false;
    WindowState state;
    state.hasActive = true;
    state.active = active;
    return updateWindowState(id, state);
}

QVariantList WindowManager::getWindowList() const {
    QVariantList list;
    for (const auto& info : m_windows) {
        list.append(info.toVariantMap());
    }
    return list;
}

QString WindowManager::getActiveWindow() const {
    return m_activeWindowId;
}

QVariantMap WindowManager::getWindowInfo(const QString& id) const {
    if (!m_windows.contains(id)) {
        return QVariantMap();
    }
    return m_windows[id].toVariantMap();
}

int WindowManager::getWindowCount() const {
    return m_windows.size();
}

bool WindowManager::activateWindow(const QString& id) {
    if (!m_initialized || !m_windows.contains(id)) {
        qWarning() << "Cannot activate window:" << id << "(not found)";
        return false;
    }

    // 发送激活请求信号（应用程序监听此信号来实际激活窗口）
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowActivateRequested"
    );
    signal.setArguments({id});
    m_connection.send(signal);

    // 同时更新本地状态
    setWindowActive(id, true);

    qInfo() << "Activate window requested:" << id;
    return true;
}

bool WindowManager::minimizeWindow(const QString& id) {
    if (!m_initialized || !m_windows.contains(id)) return false;
    
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowMinimizeRequested"
    );
    signal.setArguments({id});
    m_connection.send(signal);

    return setWindowMinimized(id, true);
}

bool WindowManager::maximizeWindow(const QString& id) {
    if (!m_initialized || !m_windows.contains(id)) return false;
    
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowMaximizeRequested"
    );
    signal.setArguments({id});
    m_connection.send(signal);

    return setWindowMaximized(id, true);
}

bool WindowManager::closeWindow(const QString& id) {
    if (!m_initialized || !m_windows.contains(id)) return false;
    
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowCloseRequested"
    );
    signal.setArguments({id});
    m_connection.send(signal);

    qInfo() << "Close window requested:" << id;
    return true;
}

void WindowManager::emitWindowListChanged() {
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "windowListChanged"
    );
    signal.setArguments({getWindowList()});
    m_connection.send(signal);
}

void WindowManager::emitActiveWindowChanged(const QString& id) {
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        "activeWindowChanged"
    );
    signal.setArguments({id});
    m_connection.send(signal);

    emit activeWindowChanged(id);
}

} // namespace explorer::daemon

#include "WindowManager.moc"