#include "DesktopWindow.h"
#include "../ui/DesktopView.h"

#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QPalette>
#include <QBrush>
#include <QVBoxLayout>
#include <QMargins>

#include <libexplorer-core/Config.h>
#include <libexplorer-core/SignalDispatcher.h>

using namespace explorer::core;
using namespace explorer::layer;
using namespace explorer::ipc;
using namespace explorer::utils;
using namespace explorer::desktop;

DesktopWindow::DesktopWindow(QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);

    // 设置默认壁纸颜色
    m_wallpaperColor = QColor(30, 30, 46); // 深蓝灰色

    // 获取桌面路径
    m_desktopPath = FileSystem::desktopPath();
    if (m_desktopPath.isEmpty()) {
        m_desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }

    qDebug() << "Desktop path:" << m_desktopPath;
}

DesktopWindow::~DesktopWindow() {
    if (m_layerSurface) {
        m_layerSurface->shutdown();
    }
    if (m_messageBus) {
        // MessageBus 自动清理
    }
    if (m_serviceWatcher) {
        m_serviceWatcher->unwatchAll();
    }
}

bool DesktopWindow::initialize() {
    if (m_initialized) {
        return true;
    }

    // 1. 加载配置
    loadConfiguration();

    // 2. 设置 LayerSurface (背景层)
    setupLayerSurface();

    // 3. 设置 IPC 通信
    setupIpc();

    // 4. 设置文件监视
    setupFileWatcher();

    // 5. 创建桌面视图和模型
    m_fileSystemModel = std::make_unique<FileSystemModel>(this);
    m_fileSystemModel->setRootPath(m_desktopPath);

    m_desktopView = new DesktopView(m_fileSystemModel.get(), this);
    m_desktopView->setContextMenuPolicy(Qt::CustomContextMenu);

    // 布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_desktopView);

    // 连接信号
    connect(m_desktopView, &DesktopView::doubleClicked,
            this, &DesktopWindow::onItemDoubleClicked);
    connect(m_desktopView, &DesktopView::customContextMenuRequested,
            this, &DesktopWindow::onItemCustomContextMenuRequested);

    // 6. 应用壁纸
    applyWallpaper();

    // 7. 调整大小为屏幕大小
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        resize(screen->geometry().size());
    }

    m_initialized = true;
    emit initialized(true);

    qDebug() << "Desktop initialized successfully";
    return true;
}

void DesktopWindow::setupLayerSurface() {
    // 创建窗口句柄
    if (!windowHandle()) {
        createWinId();
    }

    QWindow* window = windowHandle();
    if (!window) {
        qWarning() << "Failed to create window handle";
        return;
    }

    // 配置 LayerSurface 选项 - 背景层
    m_layerOptions.layer = Layer::Background;
    m_layerOptions.namespace_ = "explorer-desktop";
    m_layerOptions.description = "Explorer Linux Desktop";
    m_layerOptions.anchor = QPoint(0, 0); // 锚定所有边
    m_layerOptions.margin = QMargins(0, 0, 0, 0);
    m_layerOptions.exclusiveZone = 0; // 背景层不占用空间
    m_layerOptions.keyboardInteractivity = false;

    // 创建 LayerSurface
    m_layerSurface = LayerShellManager::instance().createSurface(window, m_layerOptions);
    if (m_layerSurface) {
        m_layerSurface->init();
        m_layerSurface->show();
        qDebug() << "LayerSurface created and shown";
    } else {
        qWarning() << "Failed to create LayerSurface";
    }
}

void DesktopWindow::setupIpc() {
    m_messageBus = std::make_unique<MessageBus>(this);
    if (!m_messageBus->init()) {
        qWarning() << "Failed to initialize MessageBus";
    }

    m_serviceWatcher = std::make_unique<ServiceWatcher>(this);
    connect(m_serviceWatcher.get(), &ServiceWatcher::serviceAppeared,
            this, &DesktopWindow::onDaemonServiceAppeared);
    connect(m_serviceWatcher.get(), &ServiceWatcher::serviceVanished,
            this, &DesktopWindow::onDaemonServiceVanished);

    // 监视守护进程服务
    m_serviceWatcher->watchService(m_daemonServiceName);
    qDebug() << "Watching daemon service:" << m_daemonServiceName;
}

void DesktopWindow::setupFileWatcher() {
    m_fileWatcher.setCallback([this](const QString& path) {
        onFileSystemWatcherChanged(path);
    });

    if (!m_fileWatcher.addPath(m_desktopPath)) {
        qWarning() << "Failed to watch desktop directory:" << m_desktopPath;
    } else {
        qDebug() << "Watching desktop directory:" << m_desktopPath;
    }
}

void DesktopWindow::loadConfiguration() {
    Config& config = Config::instance();

    // 读取壁纸设置
    m_wallpaperPath = config.value("desktop/wallpaper/path", "").toString();
    m_wallpaperColor = config.value("desktop/wallpaper/color", QColor(30, 30, 46)).value<QColor>();
    m_useWallpaperImage = config.value("desktop/wallpaper/useImage", false).toBool();

    qDebug() << "Loaded config - wallpaper:" << m_wallpaperPath
             << "color:" << m_wallpaperColor.name()
             << "useImage:" << m_useWallpaperImage;
}

void DesktopWindow::applyWallpaper() {
    if (m_useWallpaperImage && !m_wallpaperPath.isEmpty()) {
        QPixmap pixmap(m_wallpaperPath);
        if (!pixmap.isNull()) {
            // 缩放壁纸以适应屏幕
            QScreen* screen = QGuiApplication::primaryScreen();
            if (screen) {
                QSize screenSize = screen->geometry().size();
                pixmap = pixmap.scaled(screenSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            }
            QPalette palette;
            palette.setBrush(QPalette::Window, QBrush(pixmap));
            setPalette(palette);
            setAutoFillBackground(true);
            return;
        }
    }

    // 使用纯色背景
    QPalette palette;
    palette.setColor(QPalette::Window, m_wallpaperColor);
    setPalette(palette);
    setAutoFillBackground(true);
}

void DesktopWindow::showDesktop() {
    show();
    if (m_layerSurface) {
        m_layerSurface->show();
    }
}

void DesktopWindow::hideDesktop() {
    hide();
    if (m_layerSurface) {
        m_layerSurface->hide();
    }
}

void DesktopWindow::refresh() {
    if (m_fileSystemModel) {
        m_fileSystemModel->refresh();
    }
    qDebug() << "Desktop refreshed";
}

void DesktopWindow::setWallpaper(const QString& path) {
    m_wallpaperPath = path;
    m_useWallpaperImage = true;

    // 保存配置
    Config& config = Config::instance();
    config.setValue("desktop/wallpaper/path", path);
    config.setValue("desktop/wallpaper/useImage", true);
    config.sync();

    applyWallpaper();
    emit wallpaperChanged(path);
}

void DesktopWindow::setWallpaperColor(const QColor& color) {
    m_wallpaperColor = color;
    m_useWallpaperImage = false;

    Config& config = Config::instance();
    config.setValue("desktop/wallpaper/color", color);
    config.setValue("desktop/wallpaper/useImage", false);
    config.sync();

    applyWallpaper();
    emit wallpaperChanged("");
}

QString DesktopWindow::desktopPath() const {
    return m_desktopPath;
}

void DesktopWindow::onDesktopDirectoryChanged(const QString& path) {
    Q_UNUSED(path);
    refresh();
}

void DesktopWindow::onDaemonServiceAppeared(const QString& serviceName) {
    if (serviceName == m_daemonServiceName) {
        qDebug() << "Daemon service appeared:" << serviceName;
        // 向守护进程注册自己
        if (m_messageBus) {
            m_messageBus->publish("org.explorer.Desktop.Register", QVariantMap{
                {"pid", QCoreApplication::applicationPid()},
                {"desktopPath", m_desktopPath}
            });
        }
    }
}

void DesktopWindow::onDaemonServiceVanished(const QString& serviceName) {
    if (serviceName == m_daemonServiceName) {
        qDebug() << "Daemon service vanished:" << serviceName;
    }
}

void DesktopWindow::onItemDoubleClicked(const QModelIndex& index) {
    if (!index.isValid() || !m_fileSystemModel) {
        return;
    }

    QString filePath = m_fileSystemModel->filePath(index);
    if (filePath.isEmpty()) {
        return;
    }

    qDebug() << "Item double clicked:" << filePath;

    // 检查是否为目录
    if (m_fileSystemModel->isDir(index)) {
        // 打开文件管理器
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    } else {
        // 使用默认应用打开文件
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }

    emit itemActivated(filePath);
}

void DesktopWindow::onItemCustomContextMenuRequested(const QPoint& pos) {
    QModelIndex index = m_desktopView->indexAt(pos);
    QString filePath;

    if (index.isValid() && m_fileSystemModel) {
        filePath = m_fileSystemModel->filePath(index);
    }

    QMenu contextMenu(this);

    if (!filePath.isEmpty()) {
        // 文件/文件夹的右键菜单
        QAction* openAction = contextMenu.addAction("Open");
        connect(openAction, &QAction::triggered, [this, filePath]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
            emit itemActivated(filePath);
        });

        QAction* propertiesAction = contextMenu.addAction("Properties");
        connect(propertiesAction, &QAction::triggered, [this, filePath]() {
            qDebug() << "Properties for:" << filePath;
            // TODO: 实现属性对话框
        });

        contextMenu.addSeparator();

        QAction* deleteAction = contextMenu.addAction("Delete");
        connect(deleteAction, &QAction::triggered, [this, filePath]() {
            if (m_fileSystemModel) {
                m_fileSystemModel->remove(filePath);
            }
        });
    } else {
        // 空白区域的右键菜单
        QAction* refreshAction = contextMenu.addAction("Refresh");
        connect(refreshAction, &QAction::triggered, this, &DesktopWindow::refresh);

        QAction* newFolderAction = contextMenu.addAction("New Folder");
        connect(newFolderAction, &QAction::triggered, [this]() {
            if (m_fileSystemModel) {
                m_fileSystemModel->mkdir(m_desktopPath, "New Folder");
            }
        });

        QAction* wallpaperAction = contextMenu.addAction("Change Wallpaper...");
        connect(wallpaperAction, &QAction::triggered, [this]() {
            qDebug() << "Change wallpaper requested";
            // TODO: 实现壁纸选择对话框
        });
    }

    contextMenu.exec(m_desktopView->viewport()->mapToGlobal(pos));

    if (!filePath.isEmpty()) {
        emit itemContextMenuRequested(filePath, m_desktopView->viewport()->mapToGlobal(pos));
    }
}

void DesktopWindow::onFileSystemWatcherChanged(const QString& path) {
    qDebug() << "File system changed:" << path;
    // 延迟刷新以避免频繁触发
    QTimer::singleShot(100, this, &DesktopWindow::refresh);
}