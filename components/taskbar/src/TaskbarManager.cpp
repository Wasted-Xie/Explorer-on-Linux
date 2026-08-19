#include "TaskbarManager.h"
#include <QApplication>
#include <QGuiApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QScreen>
#include <QMessageBox>

namespace explorer::components {

TaskbarManager::TaskbarManager(QObject* parent)
    : QObject(parent), startMenuWidget(nullptr), m_refreshTimer(nullptr) {}

TaskbarManager::~TaskbarManager() {
    if (startMenuWidget) {
        startMenuWidget->hide();
        delete startMenuWidget;
    }
    if (m_refreshTimer) {
        m_refreshTimer->stop();
        delete m_refreshTimer;
    }
}

bool TaskbarManager::initialize() {
    // 连接到 WindowManager
    connectToWindowManager();
    
    // 启动定期刷新定时器（作为备用，主要靠信号驱动）
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(5000); // 5秒刷新一次
    connect(m_refreshTimer, &QTimer::timeout, this, &TaskbarManager::refreshWindowList);
    m_refreshTimer->start();
    
    // 立即请求一次窗口列表
    refreshWindowList();
    
    return true;
}

void TaskbarManager::connectToWindowManager() {
    // 使用 libexplorer-ipc 的 DBusInterface
    m_windowManagerInterface = std::make_unique<explorer::ipc::DBusInterface>(
        "org.explorer.WindowManager",
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        QDBusConnection::sessionBus(),
        this
    );
    
    if (!m_windowManagerInterface->isValid()) {
        qWarning() << "Failed to connect to WindowManager service";
        m_windowManagerInterface.reset();
        return;
    }
    
    qInfo() << "Connected to WindowManager service";
    
    // 设置信号连接
    setupWindowManagerConnections();
}

void TaskbarManager::setupWindowManagerConnections() {
    if (!m_windowManagerInterface || !m_windowManagerInterface->isValid()) return;
    
    // 连接 windowRegistered 信号
    m_windowManagerInterface->connectSignal("windowRegistered", [this](const QString& id, const QVariantMap& info) {
        onWindowRegistered(id, info);
    });
    
    // 连接 windowUnregistered 信号
    m_windowManagerInterface->connectSignal("windowUnregistered", [this](const QString& id) {
        onWindowUnregistered(id);
    });
    
    // 连接 windowStateChanged 信号
    m_windowManagerInterface->connectSignal("windowStateChanged", [this](const QString& id, const QVariantMap& state) {
        onWindowStateChanged(id, state);
    });
    
    // 连接 activeWindowChanged 信号
    m_windowManagerInterface->connectSignal("activeWindowChanged", [this](const QString& id) {
        onActiveWindowChanged(id);
    });
    
    // 连接 windowListChanged 信号
    m_windowManagerInterface->connectSignal("windowListChanged", [this](const QVariantList& windows) {
        onWindowListChanged(windows);
    });
}

void TaskbarManager::refreshWindowList() {
    if (!m_windowManagerInterface || !m_windowManagerInterface->isValid()) {
        qWarning() << "WindowManager interface not valid, attempting to reconnect...";
        connectToWindowManager();
        return;
    }
    
    requestWindowList();
}

void TaskbarManager::requestWindowList() {
    if (!m_windowManagerInterface || !m_windowManagerInterface->isValid()) return;
    
    auto reply = m_windowManagerInterface->call("getWindowList");
    if (!reply.isValid()) {
        qWarning() << "Failed to get window list from WindowManager:" << reply.error().message();
        return;
    }
    
    QVariantList windowList = reply.value().toList();
    qDebug() << "Received window list with" << windowList.size() << "windows";
    
    // 发送窗口列表更新信号
    emit windowListUpdated(windowList);
}

void TaskbarManager::onWindowRegistered(const QString& id, const QVariantMap& info) {
    qDebug() << "Window registered:" << id << "title:" << info.value("title").toString();
    // 请求完整窗口列表更新
    requestWindowList();
}

void TaskbarManager::onWindowUnregistered(const QString& id) {
    qDebug() << "Window unregistered:" << id;
    // 请求完整窗口列表更新
    requestWindowList();
}

void TaskbarManager::onWindowStateChanged(const QString& id, const QVariantMap& state) {
    qDebug() << "Window state changed:" << id << "state:" << state;
    // 请求完整窗口列表更新（或直接处理状态变化）
    requestWindowList();
}

void TaskbarManager::onActiveWindowChanged(const QString& id) {
    qDebug() << "Active window changed:" << id;
    emit activeWindowChanged(id);
}

void TaskbarManager::onWindowListChanged(const QVariantList& windows) {
    qDebug() << "Window list changed, count:" << windows.size();
    emit windowListUpdated(windows);
}

void TaskbarManager::toggleStartMenu() {
    if (startMenuWidget && startMenuWidget->isVisible()) {
        startMenuWidget->hide();
        emit startMenuShown(false);
        return;
    }

    // Create a simple start menu widget if not exists
    if (!startMenuWidget) {
        startMenuWidget = new QWidget();
        startMenuWidget->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        startMenuWidget->setAttribute(Qt::WA_TranslucentBackground);
        startMenuWidget->setFixedWidth(250);

        QVBoxLayout* layout = new QVBoxLayout(startMenuWidget);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(2);

        // Add some placeholder items
        QStringList items = {"文件资源管理器", "设置", "电源"};
        for (const QString& text : items) {
            QPushButton* btn = new QPushButton(text, startMenuWidget);
            btn->setFixedHeight(30);
            btn->setCursor(Qt::PointingHandCursor);
            layout->addWidget(btn);
            // Connect button clicks (for demo, just show message)
            connect(btn, &QPushButton::clicked, [text]() {
                QMessageBox::information(nullptr, "启动", QString("启动 %1").arg(text));
            });
        }

        // Add a separator line
        QFrame* line = new QFrame(startMenuWidget);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);

        // Power button
        QPushButton* powerBtn = new QPushButton("电源选项", startMenuWidget);
        powerBtn->setFixedHeight(30);
        powerBtn->setCursor(Qt::PointingHandCursor);
        layout->addWidget(powerBtn);
        connect(powerBtn, &QPushButton::clicked, []() {
            QMessageBox::information(nullptr, "电源", "电源选项占位。");
        });

        // Optional: add shadow effect
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(startMenuWidget);
        shadow->setBlurRadius(10);
        shadow->setOffset(0, 2);
        shadow->setColor(QColor(0, 0, 0, 150));
        startMenuWidget->setGraphicsEffect(shadow);
    }

    // Position the start menu above the start button (we need to get the position from taskbar)
    // For simplicity, we'll show it at the cursor position or bottom-left of screen.
    // In a real implementation, the taskbar would pass the start button geometry.
    // We'll just show at the bottom-left of the primary screen for now.
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeo = screen->availableGeometry();
    startMenuWidget->move(screenGeo.x(), screenGeo.y() + screenGeo.height() - startMenuWidget->height());
    startMenuWidget->show();
    emit startMenuShown(true);
}

} // namespace explorer::components

#include "TaskbarManager.moc"