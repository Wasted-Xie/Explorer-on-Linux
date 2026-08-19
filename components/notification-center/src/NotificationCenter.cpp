#include "NotificationCenter.h"
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QUuid>
#include <explorer/utils/DateTimeUtils.h>
#include <explorer/ui/ThemeManager.h>
#include <explorer/ui/Button.h>
#include <explorer/ui/Label.h>

namespace explorer::notification {

// NotificationCenterDBusAdaptor 实现
NotificationCenterDBusAdaptor::NotificationCenterDBusAdaptor(QObject* parent)
    : QDBusAbstractAdaptor(parent) {}

NotificationCenterDBusAdaptor::~NotificationCenterDBusAdaptor() = default;

QString NotificationCenterDBusAdaptor::Notify(const QString& summary, const QString& body,
                                              const QString& iconName, int timeout) {
    NotificationCenter* center = qobject_cast<NotificationCenter*>(parent());
    if (center) {
        return center->Notify(summary, body, iconName, timeout);
    }
    return QString();
}

void NotificationCenterDBusAdaptor::CloseNotification(const QString& id) {
    NotificationCenter* center = qobject_cast<NotificationCenter*>(parent());
    if (center) {
        center->CloseNotification(id);
    }
}

QStringList NotificationCenterDBusAdaptor::GetCapabilities() {
    return QStringList() << "body" << "icon-static" << "actions";
}

void NotificationCenterDBusAdaptor::Show() {
    NotificationCenter* center = qobject_cast<NotificationCenter*>(parent());
    if (center) {
        center->Show();
    }
}

void NotificationCenterDBusAdaptor::Hide() {
    NotificationCenter* center = qobject_cast<NotificationCenter*>(parent());
    if (center) {
        center->Hide();
    }
}

// NotificationCenter 实现
NotificationCenter::NotificationCenter(QWidget* parent)
    : QMainWindow(parent), DBusObject(this) {
    setWindowTitle("Notification Center");
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
}

NotificationCenter::~NotificationCenter() {
    shutdown();
}

bool NotificationCenter::init() {
    setupUI();
    setupLayerSurface();
    setupDBus();
    setupConfig();
    loadConfig();

    // 连接模型信号
    connect(m_model, &NotificationModel::notificationAdded, this, [this](NotificationItem* item) {
        emit notificationAdded(item->id());
        // 显示通知中心
        if (!m_isVisible) {
            showCenter();
        }
        // 重置自动隐藏定时器
        if (m_autoHide && m_hideTimer) {
            m_hideTimer->start();
        }
    });

    connect(m_model, &NotificationModel::notificationRemoved, this, [this](const QString& id) {
        emit notificationRemoved(id);
    });

    // 定时清理过期通知
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(5000); // 5秒检查一次
    connect(m_cleanupTimer, &QTimer::timeout, this, &NotificationCenter::cleanupExpiredNotifications);
    m_cleanupTimer->start();

    // 自动隐藏定时器
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(m_popupTimeout);
    connect(m_hideTimer, &QTimer::timeout, this, &NotificationCenter::hideCenter);

    // 应用主题
    explorer::ui::ThemeManager::instance().applyTheme(this);

    qDebug() << "NotificationCenter initialized";
    return true;
}

void NotificationCenter::shutdown() {
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
        m_cleanupTimer->deleteLater();
        m_cleanupTimer = nullptr;
    }
    if (m_hideTimer) {
        m_hideTimer->stop();
        m_hideTimer->deleteLater();
        m_hideTimer = nullptr;
    }
    if (m_layerSurface) {
        m_layerSurface->hide();
        m_layerSurface->shutdown();
        m_layerSurface.reset();
    }
    unexportFromBus();
    saveConfig();
    qDebug() << "NotificationCenter shutdown";
}

void NotificationCenter::setupUI() {
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName("NotificationCenterWidget");
    m_centralWidget->setStyleSheet(R"(
        #NotificationCenterWidget {
            background-color: rgba(30, 30, 30, 240);
            border-radius: 12px;
            border: 1px solid rgba(255, 255, 255, 30);
        }
    )");

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 标题栏
    QWidget* titleBar = new QWidget();
    titleBar->setFixedHeight(48);
    titleBar->setStyleSheet("background-color: rgba(0, 0, 0, 50); border-top-left-radius: 12px; border-top-right-radius: 12px;");
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(16, 0, 16, 0);

    m_titleLabel = new explorer::ui::Label("Notifications");
    m_titleLabel->setLabelType(explorer::ui::Label::LabelType::Heading3);
    m_titleLabel->setStyleSheet("color: white;");
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    m_clearAllButton = new explorer::ui::Button("Clear All");
    m_clearAllButton->setButtonType(explorer::ui::Button::ButtonType::Link);
    m_clearAllButton->setFixedHeight(32);
    connect(m_clearAllButton, &QPushButton::clicked, this, [this]() {
        m_model->clearAll();
    });
    titleLayout->addWidget(m_clearAllButton);

    m_mainLayout->addWidget(titleBar);

    // 列表视图
    m_listView = new QListView();
    m_listView->setObjectName("NotificationListView");
    m_listView->setModel(m_model = new NotificationModel(this));
    m_listView->setItemDelegate(m_delegate = new NotificationDelegate(this));
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setSelectionMode(QAbstractItemView::NoSelection);
    m_listView->setFocusPolicy(Qt::NoFocus);
    m_listView->setStyleSheet(R"(
        #NotificationListView {
            background: transparent;
            border: none;
            outline: none;
        }
        #NotificationListView::item {
            background: transparent;
            border: none;
        }
        QScrollBar:vertical {
            background: rgba(255, 255, 255, 10);
            width: 8px;
            border-radius: 4px;
            margin: 8px;
        }
        QScrollBar::handle:vertical {
            background: rgba(255, 255, 255, 50);
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(255, 255, 255, 80);
        }
    )");

    // 设置委托颜色
    m_delegate->setColors(
        QColor(40, 40, 40, 200),    // background
        QColor(220, 220, 220),       // text
        QColor(255, 255, 255),       // summary
        QColor(160, 160, 160),       // timestamp
        QColor(80, 80, 80)           // border
    );

    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            NotificationItem* item = m_model->itemAt(index.row());
            if (item) {
                qDebug() << "Notification clicked:" << item->id() << item->summary();
                // TODO: 处理点击动作
            }
        }
    });

    m_mainLayout->addWidget(m_listView, 1);

    // 空状态标签
    QLabel* emptyLabel = new QLabel("No notifications");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: rgba(255, 255, 255, 120); padding: 40px; font-size: 14px;");
    m_listView->setProperty("emptyLabel", QVariant::fromValue<QObject*>(emptyLabel));

    setCentralWidget(m_centralWidget);

    // 固定宽度
    setFixedWidth(380);
    // 高度根据内容动态调整，最大限制
    setMaximumHeight(500);
    setMinimumHeight(100);
}

void NotificationCenter::setupLayerSurface() {
    if (!explorer::layer::LayerShellManager::instance().init()) {
        qWarning() << "Failed to initialize LayerShellManager";
        return;
    }

    explorer::layer::LayerSurfaceOptions options;
    options.layer = explorer::layer::Layer::Top;
    options.namespace_ = "notification-center";
    options.description = "Explorer Notification Center";
    options.size = QSize(380, 500);
    options.margin = m_margins;
    options.keyboardInteractivity = true;

    // 设置锚点：右下角
    if (m_anchorPosition == "bottom-right") {
        options.anchor = QPoint(1.0, 1.0); // 右下
    } else if (m_anchorPosition == "top-right") {
        options.anchor = QPoint(1.0, 0.0); // 右上
    } else if (m_anchorPosition == "bottom-left") {
        options.anchor = QPoint(0.0, 1.0); // 左下
    } else if (m_anchorPosition == "top-left") {
        options.anchor = QPoint(0.0, 0.0); // 左上
    }

    m_layerSurface = explorer::layer::LayerShellManager::instance().createSurface(windowHandle(), options);
    if (!m_layerSurface || !m_layerSurface->init()) {
        qWarning() << "Failed to create LayerSurface";
        return;
    }

    connect(m_layerSurface.get(), &explorer::layer::LayerSurface::closed, this, [this]() {
        m_isVisible = false;
        emit visibilityChanged(false);
    });

    // 初始隐藏
    hide();
}

void NotificationCenter::setupDBus() {
    m_dbusAdaptor = std::make_unique<NotificationCenterDBusAdaptor>(this);
    
    // 导出到 Session Bus
    if (!exportToBus("org.explorer.NotificationCenter", "/org/explorer/NotificationCenter")) {
        qWarning() << "Failed to export NotificationCenter to DBus:" << lastError().message();
    } else {
        qDebug() << "NotificationCenter exported to DBus";
    }
}

void NotificationCenter::setupConfig() {
    // 从配置加载设置
    auto& config = explorer::core::Config::instance();
    m_maxNotifications = config.value("notification-center/maxNotifications", 20).toInt();
    m_defaultTimeout = config.value("notification-center/defaultTimeout", 5).toInt();
    m_popupTimeout = config.value("notification-center/popupTimeout", 8000).toInt();
    m_anchorPosition = config.value("notification-center/anchorPosition", "bottom-right").toString();
    
    int left = config.value("notification-center/marginLeft", 10).toInt();
    int top = config.value("notification-center/marginTop", 10).toInt();
    int right = config.value("notification-center/marginRight", 10).toInt();
    int bottom = config.value("notification-center/marginBottom", 50).toInt();
    m_margins = QMargins(left, top, right, bottom);
    
    m_autoHide = config.value("notification-center/autoHide", true).toBool();
}

void NotificationCenter::loadConfig() {
    if (m_model) {
        m_model->setMaxNotifications(m_maxNotifications);
    }
    if (m_hideTimer) {
        m_hideTimer->setInterval(m_popupTimeout);
    }
}

void NotificationCenter::saveConfig() {
    auto& config = explorer::core::Config::instance();
    config.setValue("notification-center/maxNotifications", m_maxNotifications);
    config.setValue("notification-center/defaultTimeout", m_defaultTimeout);
    config.setValue("notification-center/popupTimeout", m_popupTimeout);
    config.setValue("notification-center/anchorPosition", m_anchorPosition);
    config.setValue("notification-center/marginLeft", m_margins.left());
    config.setValue("notification-center/marginTop", m_margins.top());
    config.setValue("notification-center/marginRight", m_margins.right());
    config.setValue("notification-center/marginBottom", m_margins.bottom());
    config.setValue("notification-center/autoHide", m_autoHide);
    config.sync();
}

void NotificationCenter::cleanupExpiredNotifications() {
    if (!m_model) return;
    
    QDateTime now = QDateTime::currentDateTime();
    for (int i = m_model->rowCount() - 1; i >= 0; --i) {
        NotificationItem* item = m_model->itemAt(i);
        if (item && item->timeout() > 0 && !item->isExpired()) {
            qint64 elapsed = item->timestamp().secsTo(now);
            if (elapsed >= item->timeout()) {
                item->setExpired(true);
                m_model->removeNotification(item->id());
            }
        }
    }
}

void NotificationCenter::positionWindow() {
    if (!m_layerSurface || !windowHandle()) return;
    
    QScreen* screen = windowHandle()->screen();
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QRect screenGeom = screen->availableGeometry();
    QSize windowSize = size();
    
    QPoint pos;
    if (m_anchorPosition == "bottom-right") {
        pos = QPoint(screenGeom.right() - windowSize.width() - m_margins.right(),
                     screenGeom.bottom() - windowSize.height() - m_margins.bottom());
    } else if (m_anchorPosition == "top-right") {
        pos = QPoint(screenGeom.right() - windowSize.width() - m_margins.right(),
                     screenGeom.top() + m_margins.top());
    } else if (m_anchorPosition == "bottom-left") {
        pos = QPoint(screenGeom.left() + m_margins.left(),
                     screenGeom.bottom() - windowSize.height() - m_margins.bottom());
    } else { // top-left
        pos = QPoint(screenGeom.left() + m_margins.left(),
                     screenGeom.top() + m_margins.top());
    }
    
    move(pos);
}

void NotificationCenter::showCenter() {
    if (m_isVisible) return;
    
    positionWindow();
    show();
    raise();
    activateWindow();
    
    if (m_layerSurface) {
        m_layerSurface->show();
    }
    
    m_isVisible = true;
    emit visibilityChanged(true);
    
    if (m_autoHide && m_hideTimer) {
        m_hideTimer->start();
    }
}

void NotificationCenter::hideCenter() {
    if (!m_isVisible) return;
    
    hide();
    
    if (m_layerSurface) {
        m_layerSurface->hide();
    }
    
    m_isVisible = false;
    emit visibilityChanged(false);
}

void NotificationCenter::toggleCenter() {
    if (m_isVisible) {
        hideCenter();
    } else {
        showCenter();
    }
}

QString NotificationCenter::Notify(const QString& summary, const QString& body,
                                   const QString& iconName, int timeout) {
    if (summary.isEmpty()) {
        qWarning() << "Notification summary cannot be empty";
        return QString();
    }

    // 生成唯一 ID
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // 使用默认超时
    if (timeout <= 0) {
        timeout = m_defaultTimeout;
    }

    // 创建通知项
    NotificationItem* item = new NotificationItem(id, summary, body, iconName, timeout, m_model);
    m_model->addNotification(item);

    qDebug() << "New notification:" << id << summary;
    return id;
}

void NotificationCenter::CloseNotification(const QString& id) {
    if (m_model) {
        m_model->removeNotification(id);
    }
}

QStringList NotificationCenter::GetCapabilities() {
    return QStringList() << "body" << "icon-static" << "actions";
}

void NotificationCenter::Show() {
    showCenter();
}

void NotificationCenter::Hide() {
    hideCenter();
}

QString NotificationCenter::dbusIntrospection() const {
    return R"(<interface name="org.explorer.NotificationCenter">
  <method name="Notify">
    <arg name="summary" type="s" direction="in"/>
    <arg name="body" type="s" direction="in"/>
    <arg name="iconName" type="s" direction="in"/>
    <arg name="timeout" type="i" direction="in"/>
    <arg name="id" type="s" direction="out"/>
  </method>
  <method name="CloseNotification">
    <arg name="id" type="s" direction="in"/>
  </method>
  <method name="GetCapabilities">
    <arg name="caps" type="as" direction="out"/>
  </method>
  <method name="Show"/>
  <method name="Hide"/>
  <signal name="NotificationClosed">
    <arg name="id" type="s"/>
    <arg name="reason" type="u"/>
  </signal>
</interface>)";
}

bool NotificationCenter::event(QEvent* event) {
    // 点击窗口外部隐藏
    if (event->type() == QEvent::WindowDeactivate && m_autoHide) {
        // 延迟隐藏，避免点击内部控件时触发
        QTimer::singleShot(100, this, [this]() {
            if (!hasFocus() && !m_listView->hasFocus() && m_isVisible) {
                hideCenter();
            }
        });
    }
    return QMainWindow::event(event);
}

void NotificationCenter::closeEvent(QCloseEvent* event) {
    // 不关闭窗口，只是隐藏
    event->ignore();
    hideCenter();
}

void NotificationCenter::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hideCenter();
        event->accept();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void NotificationCenter::focusOutEvent(QFocusEvent* event) {
    QMainWindow::focusOutEvent(event);
    if (m_autoHide) {
        QTimer::singleShot(200, this, [this]() {
            if (!hasFocus() && m_isVisible) {
                hideCenter();
            }
        });
    }
}

} // namespace explorer::notification