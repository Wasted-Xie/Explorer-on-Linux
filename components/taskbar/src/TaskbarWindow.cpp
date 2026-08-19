#include "TaskbarWindow.h"
#include "StartButton.h"
#include "SystemTray.h"
#include "ClockLabel.h"
#include "WindowButton.h"
#include "TaskbarManager.h"

#include <QApplication>
#include <QScreen>
#include <QPalette>
#include <QResizeEvent>
#include <QDebug>
#include <QTimer>
#include <QMenu>
#include <QVariant>
#include <QVariantMap>
#include <QList>
#include <QIcon>

#include <explorer/core/Config.h>
#include <explorer/ui/ThemeManager.h>
#include <explorer/layer/LayerShell.h>
#include <explorer/ipc/MessageBus.h>
#include <explorer/ipc/DBusInterface.h>

namespace explorer::components {

TaskbarWindow::TaskbarWindow(QWidget* parent)
    : QMainWindow(parent) {
    setupUI();
    loadConfiguration();
    applyConfiguration();
    setupLayerShell();
    setupConnections();
}

TaskbarWindow::~TaskbarWindow() {
    if (m_layerSurface) {
        m_layerSurface->hide();
        m_layerSurface->shutdown();
    }
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
    }
}

bool TaskbarWindow::initialize() {
    // 初始化管理器
    m_manager = new TaskbarManager(this);
    if (!m_manager->initialize()) {
        qWarning() << "Failed to initialize TaskbarManager";
        return false;
    }
    
    // 连接 TaskbarManager 的窗口列表信号
    connect(m_manager, &TaskbarManager::windowListUpdated, this, &TaskbarWindow::onWindowListUpdated);
    connect(m_manager, &TaskbarManager::activeWindowChanged, this, &TaskbarWindow::onActiveWindowChanged);
    
    // 创建直接连接到 WindowManager 的 DBus 接口（用于调用 ActivateWindow 等方法）
    m_windowManagerInterface = std::make_unique<explorer::ipc::DBusInterface>(
        "org.explorer.WindowManager",
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        QDBusConnection::sessionBus(),
        this
    );
    
    if (!m_windowManagerInterface->isValid()) {
        qWarning() << "Failed to create DBus interface to WindowManager";
    } else {
        qInfo() << "Direct WindowManager DBus interface created";
    }
     
    // 启动更新定时器
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000); // 1秒更新一次
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        if (m_clockLabel) {
            m_clockLabel->updateTime();
        }
    });
    m_updateTimer->start();
     
    // 显示任务栏
    if (m_layerSurface) {
        m_layerSurface->show();
    }
    show();
    
    qInfo() << "Taskbar initialized successfully";
    return true;
}

TaskbarManager* TaskbarWindow::manager() const {
    return m_manager;
}

void TaskbarWindow::setupUI() {
    // 设置窗口标志 - 无边框、透明背景、不激活、保持在底部
    setWindowFlags(Qt::FramelessWindowHint 
                 | Qt::WindowStaysOnBottomHint 
                 | Qt::WindowDoesNotAcceptFocus
                 | Qt::X11BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_NoSystemBackground);
    
    // 创建中央部件
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName("taskbarCentralWidget");
    setCentralWidget(m_centralWidget);
    
    // 主布局
    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(4, 2, 4, 2);
    m_mainLayout->setSpacing(4);
    
    // 创建开始按钮
    m_startButton = new StartButton(this);
    m_mainLayout->addWidget(m_startButton);
    
    // 创建窗口按钮容器
    m_windowButtonsContainer = new QWidget(this);
    m_windowButtonsLayout = new QHBoxLayout(m_windowButtonsContainer);
    m_windowButtonsLayout->setContentsMargins(0, 0, 0, 0);
    m_windowButtonsLayout->setSpacing(2);
    m_mainLayout->addWidget(m_windowButtonsContainer, 1); // Stretch factor 1
    
    // 中间伸缩空间（减小以便窗口按钮可以扩展）
    m_mainLayout->addStretch(0);
    
    // 创建时钟标签
    m_clockLabel = new ClockLabel(this);
    m_mainLayout->addWidget(m_clockLabel);
    
    // 创建系统托盘
    m_systemTray = new SystemTray(this);
    m_mainLayout->addWidget(m_systemTray);
    
    // 设置固定高度
    setFixedHeight(m_taskbarHeight);
    
    // 应用主题
    auto& theme = explorer::ui::ThemeManager::instance();
    QPalette pal = palette();
    pal.setColor(QPalette::Window, theme.color(explorer::ui::ThemeManager::ColorRole::Window));
    pal.setColor(QPalette::WindowText, theme.color(explorer::ui::ThemeManager::ColorRole::WindowText));
    m_centralWidget->setPalette(pal);
    m_centralWidget->setAutoFillBackground(true);
}

void TaskbarWindow::setupLayerShell() {
    // 创建 LayerShell 管理器并初始化
    auto& layerManager = explorer::layer::LayerShellManager::instance();
    if (!layerManager.init()) {
        qWarning() << "Failed to initialize LayerShell manager, falling back to regular window";
        return;
    }
    
    if (!layerManager.isAvailable()) {
        qWarning() << "LayerShell protocol not available on this compositor";
        return;
    }
    
    // 配置 LayerSurface 选项
    explorer::layer::LayerSurfaceOptions options;
    options.layer = explorer::layer::Layer::Bottom;
    options.namespace_ = m_layerNamespace;
    options.description = "Explorer Linux Taskbar";
    options.size = QSize(0, m_taskbarHeight); // 宽度自适应，高度固定
    options.anchor = QPoint(0, 1); // 底部锚定
    options.margin = QMargins(0, 0, 0, 0);
    options.exclusiveZone = m_taskbarHeight;
    options.keyboardInteractivity = false;
    
    // 创建 LayerSurface
    // 注意：我们需要先创建一个 QWindow 来关联
    QWindow* window = windowHandle();
    if (!window) {
        // 如果还没有窗口句柄，创建一个隐藏的窗口
        window = new QWindow();
        window->setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
    }
    
    m_layerSurface = layerManager.createSurface(window, options);
    if (!m_layerSurface) {
        qWarning() << "Failed to create LayerSurface";
        return;
    }
    
    // 连接信号
    connect(m_layerSurface.get(), &explorer::layer::LayerSurface::closed, this, [this]() {
        qInfo() << "LayerSurface closed";
        QCoreApplication::quit();
    });
    
    connect(m_layerSurface.get(), &explorer::layer::LayerSurface::visibleChanged, this, [this](bool visible) {
        qDebug() << "LayerSurface visibility changed:" << visible;
    });
    
    qInfo() << "LayerShell setup complete";
}

void TaskbarWindow::setupConnections() {
    // 连接开始按钮点击
    connect(m_startButton, &StartButton::clicked, this, [this]() {
        if (m_manager) {
            m_manager->toggleStartMenu();
        }
    });
    
    // 连接系统托盘信号
    connect(m_systemTray, &SystemTray::iconActivated, this, [this](const QString& iconName) {
        qDebug() << "System tray icon activated:" << iconName;
    });
}

void TaskbarWindow::loadConfiguration() {
    auto& config = explorer::core::Config::instance();
    
    // 读取任务栏配置
    m_taskbarHeight = config.value("taskbar/height", 40).toInt();
    m_layerNamespace = config.value("taskbar/layerNamespace", "explorer-taskbar").toString();
    
    // 确保合理的范围
    if (m_taskbarHeight < 24) m_taskbarHeight = 24;
    if (m_taskbarHeight > 100) m_taskbarHeight = 100;
}

void TaskbarWindow::applyConfiguration() {
    setFixedHeight(m_taskbarHeight);
    
    // 更新 LayerSurface 配置
    if (m_layerSurface) {
        m_layerSurface->setSize(QSize(0, m_taskbarHeight));
        m_layerSurface->setExclusiveZone(m_taskbarHeight);
    }
}

// Window button management

void TaskbarWindow::addWindowButton(const QString& windowId, const QString& title, const QIcon& icon) {
    // 检查是否已经存在
    if (m_windowButtons.contains(windowId)) {
        WindowButton* btn = m_windowButtons[windowId];
        btn->setWindowTitle(title);
        btn->setWindowIcon(icon);
        return;
    }
    
    // 创建新按钮
    WindowButton* button = new WindowButton(windowId, title, icon, m_windowButtonsContainer);
    m_windowButtons[windowId] = button;
    m_windowButtonsLayout->addWidget(button);
    
    // 连接点击信号
    connect(button, &WindowButton::windowClicked, this, [this](const QString& winId) {
        // 通过 WindowManager 激活窗口
        activateWindow(winId);
    });
}

void TaskbarWindow::removeWindowButton(const QString& windowId) {
    if (!m_windowButtons.contains(windowId)) return;
    
    WindowButton* btn = m_windowButtons[windowId];
    m_windowButtonsLayout->removeWidget(btn);
    m_windowButtons.remove(windowId);
    btn->deleteLater();
}

void TaskbarWindow::updateWindowButton(const QString& windowId, const QVariantMap& info) {
    if (!m_windowButtons.contains(windowId)) {
        // 如果不存在，创建新按钮
        QString title = info.value("title").toString();
        QString iconName = info.value("icon").toString();
        QIcon icon = iconName.isEmpty() ? QIcon() : QIcon::fromTheme(iconName);
        addWindowButton(windowId, title, icon);
        return;
    }
    
    WindowButton* btn = m_windowButtons[windowId];
    QString title = info.value("title").toString();
    QString iconName = info.value("icon").toString();
    
    if (!title.isEmpty()) btn->setWindowTitle(title);
    if (!iconName.isEmpty()) btn->setWindowIcon(QIcon::fromTheme(iconName));
    
    bool isActive = info.value("isActive").toBool();
    btn->setIsActive(isActive);
}

void TaskbarWindow::setActiveWindow(const QString& windowId) {
    // 更新所有按钮的激活状态
    for (auto it = m_windowButtons.begin(); it != m_windowButtons.end(); ++it) {
        it.value()->setIsActive(it.key() == windowId);
    }
}

void TaskbarWindow::activateWindow(const QString& windowId) {
    if (!m_windowManagerInterface || !m_windowManagerInterface->isValid()) {
        qWarning() << "WindowManager interface not available";
        return;
    }
    
    // 调用 WindowManager 的 activateWindow 方法
    auto reply = m_windowManagerInterface->call("activateWindow", windowId);
    if (!reply.isValid()) {
        qWarning() << "Failed to activate window:" << reply.error().message();
    } else {
        qInfo() << "Activate window request sent for:" << windowId;
        // 立即更新本地 UI 状态
        setActiveWindow(windowId);
    }
}

void TaskbarWindow::updateWindowButtonsFromList(const QVariantList& windows) {
    // 记录当前存在的窗口 ID
    QSet<QString> currentIds;
    
    // 更新或添加窗口按钮
    for (const QVariant& var : windows) {
        QVariantMap info = var.toMap();
        QString windowId = info.value("id").toString();
        if (windowId.isEmpty()) continue;
        
        currentIds.insert(windowId);
        updateWindowButton(windowId, info);
    }
    
    // 移除不再存在的窗口
    QStringList toRemove;
    for (auto it = m_windowButtons.begin(); it != m_windowButtons.end(); ++it) {
        if (!currentIds.contains(it.key())) {
            toRemove.append(it.key());
        }
    }
    
    for (const QString& id : toRemove) {
        removeWindowButton(id);
    }
}

// Slots for WindowManager signals

void TaskbarWindow::onWindowListUpdated(const QVariantList& windows) {
    qDebug() << "Window list updated, count:" << windows.size();
    updateWindowButtonsFromList(windows);
}

void TaskbarWindow::onActiveWindowChanged(const QString& windowId) {
    qDebug() << "Active window changed:" << windowId;
    setActiveWindow(windowId);
}

void TaskbarWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        // 确保任务栏始终保持在底层
        if (windowState() & Qt::WindowMinimized) {
            setWindowState(Qt::WindowNoState);
        }
    }
    QMainWindow::changeEvent(event);
}

void TaskbarWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    
    // 更新 LayerSurface 大小
    if (m_layerSurface) {
        m_layerSurface->setSize(QSize(width(), m_taskbarHeight));
    }
}

} // namespace explorer::components

#include "TaskbarWindow.moc"