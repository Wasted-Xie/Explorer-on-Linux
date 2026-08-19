#pragma once

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QVariant>
#include <QVariantList>

#include <libexplorer-ipc/DBusInterface.h>

namespace explorer::components {

class TaskbarManager : public QObject {
    Q_OBJECT
public:
    explicit TaskbarManager(QObject* parent = nullptr);
    ~TaskbarManager() override;

    bool initialize();
    void toggleStartMenu();
    void updateWindowList();  // 从 WindowManager 获取窗口列表并更新 UI

    // WindowManager 相关
    void connectToWindowManager();
    void refreshWindowList();

signals:
    void startMenuShown(bool shown);
    void windowListUpdated(const QVariantList& windows);
    void activeWindowChanged(const QString& windowId);

private slots:
    void onWindowRegistered(const QString& id, const QVariantMap& info);
    void onWindowUnregistered(const QString& id);
    void onWindowStateChanged(const QString& id, const QVariantMap& state);
    void onActiveWindowChanged(const QString& id);
    void onWindowListChanged(const QVariantList& windows);

private:
    QWidget* startMenuWidget = nullptr;
    
    // WindowManager DBus 接口
    std::unique_ptr<explorer::ipc::DBusInterface> m_windowManagerInterface;
    QTimer* m_refreshTimer = nullptr;
    
    void setupWindowManagerConnections();
    void requestWindowList();
};

} // namespace explorer::components

#include "TaskbarManager.moc"