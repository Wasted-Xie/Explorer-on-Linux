#pragma once

#include <QObject>
#include <QString>
#include <QWindow>
#include <QSize>
#include <QPoint>
#include <QMargins>
#include <functional>
#include <optional>

// 为了避免直接包含 wayland 头文件导致的兼容性问题，我们使用前向声明
// 实际的 wayland 类型将在 .cpp 文件中包含
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_surface;
struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;

namespace explorer::layer {

// Layer 枚举 - 定义层的位置
enum class Layer {
    Background,   // 桌面背景层
    Bottom,       // 底部层 (如任务栏)
    Top,          // 顶部层 (如通知)
    Overlay       // 覆盖层 (如锁屏)
};

// LayerShell 错误
enum class Error {
    None,
    FailedToInitialize,
    FailedToCreateSurface,
    FailedToConfigure
};

// LayerSurface 配置
struct LayerSurfaceOptions {
    Layer layer = Layer::Top;
    QString namespace_; // 用于分组表面
    QString description;
    QSize size;         // 0 表示使用窗口大小
    QPoint anchor;      // 锚点 (左上角为原点的比例值 0-1)
    QMargins margin;    // 外边距
    qreal exclusiveZone = -1; // 专用区域 (-1 表示自动)
    bool keyboardInteractivity = false;
};

// LayerSurface - 表示一个 layer-shell 表面
class LayerSurface : public QObject {
    Q_OBJECT
public:
    explicit LayerSurface(QWindow* window, const LayerSurfaceOptions& options = {});
    ~LayerSurface() override;

    // 初始化
    bool init();
    void shutdown();

    // 属性设置
    void setLayer(Layer layer);
    void setNamespace(const QString& ns);
    void setDescription(const QString& desc);
    void setSize(const QSize& size);
    void setAnchor(qreal left, qreal top, qreal right, qreal bottom);
    void setMargin(int left, int top, int right, int bottom);
    void setExclusiveZone(qreal zone);
    void setKeyboardInteractivity(bool enabled);

    // 显示/隐藏
    bool show();
    void hide();

    // 是否可见
    bool isVisible() const;

    // 获取关联的窗口
    QWindow* window() const;

signals:
    void layerChanged(Layer layer);
    void namespaceChanged(const QString& ns);
    void descriptionChanged(const QString& desc);
    void sizeChanged(const QSize& size);
    void anchorChanged(qreal left, qreal top, qreal right, qreal bottom);
    void marginChanged(int left, int top, int right, int bottom);
    void exclusiveZoneChanged(qreal zone);
    void keyboardInteractivityChanged(bool enabled);
    void visibleChanged(bool visible);
    void closed(); // 表面被关闭时

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

// LayerShell 管理器 - 单例，管理 layer-shell 连接和表面
class LayerShellManager : public QObject {
    Q_OBJECT
public:
    static LayerShellManager& instance();

    LayerShellManager(const LayerShellManager&) = delete;
    LayerShellManager& operator=(const LayerShellManager&) = delete;

    // 初始化
    bool init();
    void shutdown();

    // 创建 LayerSurface
    std::unique_ptr<LayerSurface> createSurface(QWindow* window,
                                              const LayerSurfaceOptions& options = {});

    // 检查是否可用
    bool isAvailable() const;

    // 获取方式land 显示连接
    wl_display* display() const;

private:
    LayerShellManager();
    ~LayerShellManager();

    class Impl;
    std::unique_ptr<Impl> d;
};

// 工具函数
namespace utils {
    // 将 Layer 转换为字符串
    QString layerToString(Layer layer);
    // 从字符串转换为 Layer
    std::optional<Layer> stringToLayer(const QString& str);
    
    // 创建默认选项
    LayerSurfaceOptions defaultOptionsForLayer(Layer layer);
}

} // namespace explorer::layer

#include "LayerShell.moc"