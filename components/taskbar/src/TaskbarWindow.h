#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QList>
#include <QVariant>
#include <QVariantMap>
#include <QMap>

#include <explorer/layer/LayerShell.h>
#include <explorer/core/Config.h>
#include <explorer/ui/Button.h>
#include <explorer/ui/Label.h>
#include <explorer/ipc/MessageBus.h>
#include <explorer/ipc/DBusInterface.h>

namespace explorer::components {

class StartButton;
class SystemTray;
class ClockLabel;
class TaskbarManager;
class WindowButton;

class TaskbarWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit TaskbarWindow(QWidget* parent = nullptr);
    ~TaskbarWindow() override;

    // 初始化任务栏
    bool initialize();
    
    // 获取管理器
    TaskbarManager* manager() const;

protected:
    // 处理窗口状态变化
    void changeEvent(QEvent* event) override;
    
    // 处理大小变化
    void resizeEvent(QResizeEvent* event) override;

private slots:
    // WindowManager 信号处理
    void onWindowListUpdated(const QVariantList& windows);
    void onActiveWindowChanged(const QString& windowId);

private:
    void setupUI();
    void setupLayerShell();
    void setupConnections();
    void loadConfiguration();
    void applyConfiguration();

    // Window button management
    void addWindowButton(const QString& windowId, const QString& title, const QIcon& icon = QIcon());
    void removeWindowButton(const QString& windowId);
    void updateWindowButton(const QString& windowId, const QVariantMap& info);
    void setActiveWindow(const QString& windowId);
    void updateWindowButtonsFromList(const QVariantList& windows);

    // UI 组件
    QWidget* m_centralWidget = nullptr;
    QHBoxLayout* m_mainLayout = nullptr;
    
    StartButton* m_startButton = nullptr;
    SystemTray* m_systemTray = nullptr;
    ClockLabel* m_clockLabel = nullptr;
    
    // Window buttons container
    QWidget* m_windowButtonsContainer = nullptr;
    QHBoxLayout* m_windowButtonsLayout = nullptr;
    // windowId -> WindowButton
    QMap<QString, WindowButton*> m_windowButtons;
    
    // LayerShell 集成
    std::unique_ptr<explorer::layer::LayerSurface> m_layerSurface;
    
    // DBus 接口（用于直接调用 WindowManager）
    std::unique_ptr<explorer::ipc::DBusInterface> m_windowManagerInterface;
    
    // 管理器
    TaskbarManager* m_manager = nullptr;
    
    // 配置
    int m_taskbarHeight = 40;
    QString m_layerNamespace = "explorer-taskbar";
    
    // 定时器用于定期检查
    QTimer* m_updateTimer = nullptr;
};

} // namespace explorer::components

#include "TaskbarWindow.moc"