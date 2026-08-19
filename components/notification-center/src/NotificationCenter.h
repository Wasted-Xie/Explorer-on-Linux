#pragma once

#include <QMainWindow>
#include <QListView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QEvent>
#include "NotificationModel.h"
#include "NotificationDelegate.h"
#include <explorer/layer/LayerShell.h>
#include <explorer/ipc/DBusObject.h>
#include <explorer/core/Config.h>
#include <explorer/core/Registry.h>
#include <explorer/core/SignalDispatcher.h>

namespace explorer::notification {

// DBus 接口定义
class NotificationCenterDBusAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.explorer.NotificationCenter")
    Q_CLASSINFO("D-Bus Introspection", 
        "<interface name=\"org.explorer.NotificationCenter\">"
        "  <method name=\"Notify\">"
        "    <arg name=\"summary\" type=\"s\" direction=\"in\"/>"
        "    <arg name=\"body\" type=\"s\" direction=\"in\"/>"
        "    <arg name=\"iconName\" type=\"s\" direction=\"in\"/>"
        "    <arg name=\"timeout\" type=\"i\" direction=\"in\"/>"
        "    <arg name=\"id\" type=\"s\" direction=\"out\"/>"
        "  </method>"
        "  <method name=\"CloseNotification\">"
        "    <arg name=\"id\" type=\"s\" direction=\"in\"/>"
        "  </method>"
        "  <method name=\"GetCapabilities\">"
        "    <arg name=\"caps\" type=\"as\" direction=\"out\"/>"
        "  </method>"
        "  <method name=\"Show\">"
        "  </method>"
        "  <method name=\"Hide\">"
        "  </method>"
        "  <signal name=\"NotificationClosed\">"
        "    <arg name=\"id\" type=\"s\"/>"
        "    <arg name=\"reason\" type=\"u\"/>"
        "  </signal>"
        "</interface>")

public:
    explicit NotificationCenterDBusAdaptor(QObject* parent);
    virtual ~NotificationCenterDBusAdaptor();

    // DBus 方法
    Q_SCRIPTABLE QString Notify(const QString& summary, const QString& body,
                                const QString& iconName, int timeout);
    Q_SCRIPTABLE void CloseNotification(const QString& id);
    Q_SCRIPTABLE QStringList GetCapabilities();
    Q_SCRIPTABLE void Show();
    Q_SCRIPTABLE void Hide();

signals:
    void notificationClosed(const QString& id, uint reason);
};

// 通知中心主窗口
class NotificationCenter : public QMainWindow, public explorer::ipc::DBusObject {
    Q_OBJECT
public:
    explicit NotificationCenter(QWidget* parent = nullptr);
    ~NotificationCenter() override;

    // 初始化
    bool init();
    void shutdown();

    // 显示/隐藏
    void showCenter();
    void hideCenter();
    void toggleCenter();

    // DBus 导出接口
    QString dbusInterface() const override { return "org.explorer.NotificationCenter"; }
    QString dbusIntrospection() const override;

public slots:
    // DBus 方法槽
    QString Notify(const QString& summary, const QString& body,
                   const QString& iconName, int timeout);
    void CloseNotification(const QString& id);
    QStringList GetCapabilities();
    void Show();
    void Hide();

signals:
    void visibilityChanged(bool visible);
    void notificationAdded(const QString& id);
    void notificationRemoved(const QString& id);

protected:
    bool event(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void setupUI();
    void setupLayerSurface();
    void setupDBus();
    void setupConfig();
    void loadConfig();
    void saveConfig();
    void cleanupExpiredNotifications();
    void positionWindow();

    // UI 组件
    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    QLabel* m_titleLabel = nullptr;
    QListView* m_listView = nullptr;
    NotificationModel* m_model = nullptr;
    NotificationDelegate* m_delegate = nullptr;
    QPushButton* m_clearAllButton = nullptr;

    // LayerShell
    std::unique_ptr<explorer::layer::LayerSurface> m_layerSurface;

    // DBus
    std::unique_ptr<NotificationCenterDBusAdaptor> m_dbusAdaptor;

    // 定时器
    QTimer* m_cleanupTimer = nullptr;
    QTimer* m_hideTimer = nullptr;

    // 配置
    int m_maxNotifications = 20;
    int m_defaultTimeout = 5; // 秒
    int m_popupTimeout = 8000; // 毫秒
    QString m_anchorPosition = "bottom-right"; // bottom-right, top-right, etc.
    QMargins m_margins = QMargins(10, 10, 10, 50); // 底部留出任务栏空间

    // 状态
    bool m_isVisible = false;
    bool m_autoHide = true;

    // 信号连接ID
    int m_notificationAddedConnection = -1;
};

} // namespace explorer::notification

#include "NotificationCenter.moc"