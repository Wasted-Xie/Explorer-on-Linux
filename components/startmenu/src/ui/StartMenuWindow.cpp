#include "StartMenuWindow.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>
#include <QProcess>
#include <QFrame>
#include "libs/libexplorer-core/src/config/Config.h"
#include "libs/libexplorer-utils/src/ProcessUtils.h"
#include "libs/libexplorer-ui/src/ThemeManager.h"

namespace explorer::startmenu {

class StartMenuWindow::Impl {
public:
    QWidget* container = nullptr;
    QVBoxLayout* mainLayout = nullptr;
    QScrollArea* scrollArea = nullptr;
    QWidget* scrollContent = nullptr;
    QVBoxLayout* scrollLayout = nullptr;
    
    std::unique_ptr<explorer::layer::LayerSurface> layerSurface;
    bool visible = false;
    int width = 320;
    int maxHeight = 480;
    int itemHeight = 44;
    int margin = 8;
    QPoint lastPosition;
};

StartMenuWindow::StartMenuWindow(QWindow* parent)
    : QWindow(parent), d(std::make_unique<Impl>()) {
    setTitle("Explorer Start Menu");
    setFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    
    // 从配置读取设置
    auto& config = explorer::core::Config::instance();
    d->width = config.value("startmenu/width", 320).toInt();
    d->maxHeight = config.value("startmenu/maxHeight", 480).toInt();
    d->itemHeight = config.value("startmenu/itemHeight", 44).toInt();
    d->margin = config.value("startmenu/margin", 8).toInt();
    
    createLayerSurface();
    setupUI();
}

StartMenuWindow::~StartMenuWindow() {
    if (d->layerSurface) {
        d->layerSurface->hide();
        d->layerSurface->shutdown();
    }
}

void StartMenuWindow::createLayerSurface() {
    explorer::layer::LayerSurfaceOptions options;
    options.layer = explorer::layer::Layer::Top;
    options.namespace_ = "explorer-startmenu";
    options.description = "Explorer Linux Start Menu";
    options.size = QSize(d->width, d->maxHeight);
    options.anchor = QPoint(0, 0); // Will be positioned manually
    options.margin = QMargins(d->margin, d->margin, d->margin, d->margin);
    options.exclusiveZone = -1;
    options.keyboardInteractivity = true;
    
    auto& manager = explorer::layer::LayerShellManager::instance();
    if (!manager.isAvailable()) {
        qWarning() << "LayerShell not available, falling back to regular window";
        return;
    }
    
    d->layerSurface = manager.createSurface(this, options);
    if (d->layerSurface) {
        if (!d->layerSurface->init()) {
            qWarning() << "Failed to initialize LayerSurface";
            d->layerSurface.reset();
        } else {
            connect(d->layerSurface.get(), &explorer::layer::LayerSurface::visibleChanged,
                    this, [this](bool visible) {
                        d->visible = visible;
                        if (!visible) {
                            emit closed();
                        }
                    });
        }
    }
}

void StartMenuWindow::setupUI() {
    // 创建容器 widget
    d->container = new QWidget();
    d->container->setAttribute(Qt::WA_TranslucentBackground);
    d->container->setWindowFlags(Qt::FramelessWindowHint);
    
    d->mainLayout = new QVBoxLayout(d->container);
    d->mainLayout->setContentsMargins(0, 0, 0, 0);
    d->mainLayout->setSpacing(0);
    
    // 标题栏
    auto* header = new explorer::ui::Label("Applications", d->container);
    header->setLabelType(explorer::ui::Label::LabelType::Heading3);
    header->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setFixedHeight(48);
    header->setContentsMargins(16, 0, 16, 0);
    header->applyTheme();
    d->mainLayout->addWidget(header);
    
    // 分隔线
    auto* separator1 = new QFrame(d->container);
    separator1->setFrameShape(QFrame::HLine);
    separator1->setFrameShadow(QFrame::Sunken);
    separator1->setFixedHeight(1);
    d->mainLayout->addWidget(separator1);
    
    // 滚动区域
    d->scrollArea = new QScrollArea(d->container);
    d->scrollArea->setWidgetResizable(true);
    d->scrollArea->setFrameShape(QFrame::NoFrame);
    d->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    d->scrollContent = new QWidget();
    d->scrollContent->setAttribute(Qt::WA_TranslucentBackground);
    
    d->scrollLayout = new QVBoxLayout(d->scrollContent);
    d->scrollLayout->setContentsMargins(8, 8, 8, 8);
    d->scrollLayout->setSpacing(4);
    d->scrollLayout->addStretch();
    
    d->scrollArea->setWidget(d->scrollContent);
    d->mainLayout->addWidget(d->scrollArea, 1);
    
    // 分隔线
    auto* separator2 = new QFrame(d->container);
    separator2->setFrameShape(QFrame::HLine);
    separator2->setFrameShadow(QFrame::Sunken);
    separator2->setFixedHeight(1);
    d->mainLayout->addWidget(separator2);
    
    // 电源选项区域
    auto* powerSection = new QWidget(d->container);
    auto* powerLayout = new QVBoxLayout(powerSection);
    powerLayout->setContentsMargins(8, 8, 8, 8);
    powerLayout->setSpacing(4);
    
    // Shutdown
    auto* shutdownBtn = new ApplicationItem("Shutdown", "systemctl poweroff", QIcon::fromTheme("system-shutdown"), powerSection);
    connect(shutdownBtn, &ApplicationItem::launched, this, &StartMenuWindow::applicationLaunched);
    powerLayout->addWidget(shutdownBtn);
    
    // Restart
    auto* restartBtn = new ApplicationItem("Restart", "systemctl reboot", QIcon::fromTheme("system-reboot"), powerSection);
    connect(restartBtn, &ApplicationItem::launched, this, &StartMenuWindow::applicationLaunched);
    powerLayout->addWidget(restartBtn);
    
    // Logout
    auto* logoutBtn = new ApplicationItem("Logout", "gnome-session-quit --logout --no-prompt", QIcon::fromTheme("system-log-out"), powerSection);
    connect(logoutBtn, &ApplicationItem::launched, this, &StartMenuWindow::applicationLaunched);
    powerLayout->addWidget(logoutBtn);
    
    d->mainLayout->addWidget(powerSection);
    
    // 设置窗口内容
    setContent(d->container);
    resize(d->width, d->maxHeight);
}

void StartMenuWindow::setApplications(const QList<QPair<QString, QString>>& apps) {
    // 清除现有项目（保留 stretch）
    while (d->scrollLayout->count() > 1) {
        QLayoutItem* item = d->scrollLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // 添加新应用
    for (const auto& app : apps) {
        auto* item = new ApplicationItem(app.first, app.second, QIcon::fromTheme("application-x-executable"), d->scrollContent);
        connect(item, &ApplicationItem::launched, this, &StartMenuWindow::applicationLaunched);
        d->scrollLayout->insertWidget(d->scrollLayout->count() - 1, item);
    }
    
    d->scrollContent->adjustSize();
}

void StartMenuWindow::showAt(const QPoint& globalPos) {
    d->lastPosition = globalPos;
    positionAt(globalPos);
    
    if (d->layerSurface) {
        d->layerSurface->show();
    } else {
        // Fallback: 普通窗口显示
        setPosition(globalPos);
        QWindow::show();
    }
    d->visible = true;
    raise();
    requestActivate();
}

void StartMenuWindow::hide() {
    if (d->layerSurface) {
        d->layerSurface->hide();
    } else {
        QWindow::hide();
    }
    d->visible = false;
}

bool StartMenuWindow::isVisible() const {
    return d->visible;
}

void StartMenuWindow::positionAt(const QPoint& globalPos) {
    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    QRect screenGeom = screen->availableGeometry();
    
    int x = globalPos.x();
    int y = globalPos.y();
    
    // 确保菜单在屏幕内
    if (x + d->width > screenGeom.right()) {
        x = screenGeom.right() - d->width;
    }
    if (x < screenGeom.left()) {
        x = screenGeom.left();
    }
    if (y + d->maxHeight > screenGeom.bottom()) {
        y = screenGeom.bottom() - d->maxHeight;
    }
    if (y < screenGeom.top()) {
        y = screenGeom.top();
    }
    
    // 更新 LayerSurface 位置
    if (d->layerSurface) {
        d->layerSurface->setAnchor(0, 0, 0, 0);
        d->layerSurface->setMargin(QMargins(y, x, 0, 0));
    }
    
    setPosition(QPoint(x, y));
    resize(d->width, d->maxHeight);
}

void StartMenuWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hide();
        emit closed();
    }
    QWindow::keyPressEvent(event);
}

} // namespace explorer::startmenu