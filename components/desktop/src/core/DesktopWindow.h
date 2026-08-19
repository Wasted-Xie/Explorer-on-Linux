#pragma once

#include <QObject>
#include <QWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QStandardPaths>
#include <memory>

#include <libexplorer-core/FileSystem.h>
#include <libexplorer-core/FileSystemModel.h>
#include <libexplorer-layer/LayerShell.h>
#include <libexplorer-ipc/MessageBus.h>
#include <libexplorer-ipc/ServiceWatcher.h>
#include <libexplorer-utils/FileUtils.h>

#include "../ui/DesktopView.h"

namespace explorer::desktop {

class DesktopWindow : public QWidget {
    Q_OBJECT
public:
    explicit DesktopWindow(QWidget* parent = nullptr);
    ~DesktopWindow() override;

    // 初始化桌面
    bool initialize();

    // 显示/隐藏
    void showDesktop();
    void hideDesktop();

    // 刷新桌面图标
    void refresh();

    // 设置壁纸
    void setWallpaper(const QString& path);
    void setWallpaperColor(const QColor& color);

    // 获取桌面路径
    QString desktopPath() const;

signals:
    void initialized(bool success);
    void wallpaperChanged(const QString& path);
    void itemActivated(const QString& path);
    void itemContextMenuRequested(const QString& path, const QPoint& globalPos);

private slots:
    void onDesktopDirectoryChanged(const QString& path);
    void onDaemonServiceAppeared(const QString& serviceName);
    void onDaemonServiceVanished(const QString& serviceName);
    void onItemDoubleClicked(const QModelIndex& index);
    void onItemCustomContextMenuRequested(const QPoint& pos);
    void onFileSystemWatcherChanged(const QString& path);

private:
    void setupLayerSurface();
    void setupIpc();
    void setupFileWatcher();
    void loadConfiguration();
    void applyWallpaper();

    // LayerSurface 相关
    std::unique_ptr<explorer::layer::LayerSurface> m_layerSurface;
    explorer::layer::LayerSurfaceOptions m_layerOptions;

    // IPC 相关
    std::unique_ptr<explorer::ipc::MessageBus> m_messageBus;
    std::unique_ptr<explorer::ipc::ServiceWatcher> m_serviceWatcher;
    const QString m_daemonServiceName = "org.explorer.Daemon";

    // 文件系统模型
    std::unique_ptr<explorer::core::FileSystemModel> m_fileSystemModel;

    // 桌面视图
    DesktopView* m_desktopView = nullptr;

    // 配置
    QString m_desktopPath;
    QString m_wallpaperPath;
    QColor m_wallpaperColor;
    bool m_useWallpaperImage = false;

    // 文件监视
    explorer::utils::FileUtils::FileWatcher m_fileWatcher;

    bool m_initialized = false;
};

} // namespace explorer::desktop