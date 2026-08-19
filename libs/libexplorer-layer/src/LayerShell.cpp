#include "LayerShell.h"
#include <QDebug>
#include <QWindow>
#include <QGuiApplication>

// Wayland 头文件
#include <wayland-client.h>
#include <wayland-protocols/drm-client-protocol.h>
#include <wayland-protocols/unstable/layer-shell/zwlr_layer_shell_v1.h>

namespace explorer::layer {

class LayerSurface::Impl {
public:
    Impl(LayerSurface* owner, QWindow* window, const LayerSurfaceOptions& options)
        : q(owner), m_window(window), m_options(options) {}

    // Wayland 回调
    static void pingHandler(void* data, zwlr_layer_surface_v1* surface, uint32_t serial) {
        auto self = static_cast<LayerSurface::Impl*>(data);
        zwlr_layer_surface_v1_pong(surface, serial);
    }

    static void configureHandler(void* data, zwlr_layer_surface_v1* surface,
                               uint32_t serial, uint32_t width, uint32_t height) {
        auto self = static_cast<LayerSurface::Impl*>(data);
        // 确认配置
        zwlr_layer_surface_v1_ack_configure(surface, serial);
        
        // 更新窗口几何
        if (self->m_window && self->m_window->isVisible()) {
            self->m_window->setWidth(width);
            self->m_window->setHeight(height);
        }
        
        // 发送配置变化信号
        QMetaObject::invokeMethod(self->q, "configured", Qt::QueuedConnection,
                                Q_ARG(uint32_t, width), Q_ARG(uint32_t, height));
    }

    static void closedHandler(void* data, zwlr_layer_surface_v1* surface) {
        auto self = static_cast<LayerSurface::Impl*>(data);
        self->m_surface = nullptr;
        QMetaObject::invokeMethod(self->q, &LayerSurface::closed, Qt::QueuedConnection);
    }

    static const zwlr_layer_surface_v1_listener layerSurfaceListener;

    LayerSurface* q;
    QWindow* m_window;
    LayerSurfaceOptions m_options;
    
    // Wayland 对象
    struct wl_display* m_display = nullptr;
    struct wl_registry* m_registry = nullptr;
    struct wl_compositor* m_compositor = nullptr;
    struct zwlr_layer_shell_v1* m_layerShell = nullptr;
    struct zwlr_layer_surface_v1* m_surface = nullptr;
    
    uint32_t m_configuredSerial = 0;
    bool m_initialized = false;
    bool m_visible = false;
};

// 静态成员初始化
const zwlr_layer_surface_v1_listener LayerSurface::Impl::layerSurfaceListener = {
    .ping = &LayerSurface::Impl::pingHandler,
    .configure = &LayerSurface::Impl::configureHandler,
    .closed = &LayerSurface::Impl::closedHandler
};

LayerSurface::LayerSurface(QWindow* window, const LayerSurfaceOptions& options)
    : QObject(window), d(std::make_unique<Impl>(this, window, options)) {}

LayerSurface::~LayerSurface() {
    shutdown();
}

bool LayerSurface::init() {
    if (d->m_initialized) return true;
    
    // 获取 Wayland 显示连接
    // 注意：这需要应用已经通过 wayland 方式启动
    // 或者我们需要从环境变量获取方式land 显示
    const char* displayName = qgetenv("WAYLAND_DISPLAY");
    if (!displayName) displayName = "wayland-0";
    
    d->m_display = wl_display_connect(displayName);
    if (!d->m_display) {
        qWarning() << "Failed to connect to Wayland display" << displayName;
        return false;
    }
    
    // 获取注册表
    d->m_registry = wl_display_get_registry(d->m_display);
    wl_registry_add_listener(d->m_registry, &(wl_registry_listener){
        .global = [](void* data, wl_registry* registry, uint32_t name,
                     const char* interface, uint32_t version) {
            auto self = static_cast<LayerSurface::Impl*>(data);
            if (strcmp(interface, wl_compositor_interface.name) == 0) {
                self->m_compositor = static_cast<wl_compositor*>(
                    wl_registry_bind(registry, name, &wl_compositor_interface, 1));
            } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
                self->m_layerShell = static_cast<zwlr_layer_shell_v1*>(
                    wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1));
            }
        },
        .global_remove = [](void* data, wl_registry* registry, uint32_t name) {
            // 忽略
        }
    }, this);
    
    // 处理事件以获取全局对象
    wl_display_roundtrip(d->m_display);
    
    if (!d->m_compositor || !d->m_layerShell) {
        qWarning() << "Failed to get required Wayland objects";
        wl_display_disconnect(d->m_display);
        d->m_display = nullptr;
        return false;
    }
    
    // 创建 wl_surface
    struct wl_surface* surface = wl_compositor_create_surface(d->m_compositor);
    if (!surface) {
        qWarning() << "Failed to create wl_surface";
        wl_display_disconnect(d->m_display);
        d->m_display = nullptr;
        return false;
    }
    
    // 创建 layer surface
    d->m_surface = zwlr_layer_shell_v1_get_layer_surface(
        d->m_layerShell, surface, nullptr,
        static_cast<uint32_t>(d->m_options.layer),
        qPrintable(d->m_options.namespace_.isEmpty() ? "explorer" : d->m_options.namespace_));
    
    if (!d->m_surface) {
        qWarning() << "Failed to create layer surface";
        wl_surface_destroy(surface);
        wl_display_disconnect(d->m_display);
        d->m_display = nullptr;
        return false;
    }
    
    // 设置监听器
    zwlr_layer_surface_v1_add_listener(d->m_surface, &Impl::layerSurfaceListener, this);
    
    // 设置初始属性
    if (!d->m_options.description.isEmpty()) {
        zwlr_layer_surface_v1_set_description(d->m_surface,
                                            qPrintable(d->m_options.description));
    }
    
    if (d->m_options.size.isValid() && 
        d->m_options.size.width() > 0 && 
        d->m_options.size.height() > 0) {
        zwlr_layer_surface_v1_set_size(d->m_surface,
                                       d->m_options.size.width(),
                                       d->m_options.size.height());
    }
    
    // 设置锚点
    zwlr_layer_surface_v1_set_anchor(d->m_surface,
                                   d->m_options.anchor.x() > 0 ? ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT : 0,
                                   d->m_options.anchor.y() > 0 ? ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP : 0,
                                   d->m_options.anchor.z() > 0 ? ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT : 0,
                                   d->m_options.anchor.w() > 0 ? ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM : 0);
    
    // 设置边距
    zwlr_layer_surface_v1_set_margin(d->m_surface,
                                   d->m_options.margin.left(),
                                   d->m_options.margin.top(),
                                   d->m_options.margin.right(),
                                   d->m_options.margin.bottom());
    
    // 设置专用区域
    if (d->m_options.exclusiveZone >= 0) {
        zwlr_layer_surface_v1_set_exclusive_zone(d->m_surface,
                                               d->m_options.exclusiveZone);
    }
    
    // 设置键盘交互性
    if (d->m_options.keyboardInteractivity) {
        zwlr_layer_surface_v1_set_keyboard_interactivity(d->m_surface);
    }
    
    d->m_initialized = true;
    return true;
}

void LayerSurface::shutdown() {
    if (!d->m_initialized) return;
    
    if (d->m_surface) {
        zwlr_layer_surface_v1_destroy(d->m_surface);
        d->m_surface = nullptr;
    }
    
    if (d->m_display) {
        wl_display_disconnect(d->m_display);
        d->m_display = nullptr;
    }
    
    d->m_layerShell = nullptr;
    d->m_compositor = nullptr;
    d->m_registry = nullptr;
    d->m_initialized = false;
}

void LayerSurface::setLayer(Layer layer) {
    if (d->m_options.layer == layer) return;
    d->m_options.layer = layer;
    // 需要重新创建 surface 来更改层
    // 为了简化，这里我们只是更新选项，实际应用中需要重建
    emit layerChanged(layer);
}

void LayerSurface::setNamespace(const QString& ns) {
    if (d->m_options.namespace_ == ns) return;
    d->m_options.namespace_ = ns;
    emit namespaceChanged(ns);
}

void LayerSurface::setDescription(const QString& desc) {
    if (d->m_options.description == desc) return;
    d->m_options.description = desc;
    if (d->m_surface && !desc.isEmpty()) {
        zwlr_layer_surface_v1_set_description(d->m_surface, qPrintable(desc));
    }
    emit descriptionChanged(desc);
}

void LayerSurface::setSize(const QSize& size) {
    if (d->m_options.size == size) return;
    d->m_options.size = size;
    if (d->m_surface && size.isValid() && 
        size.width() > 0 && size.height() > 0) {
        zwlr_layer_surface_v1_set_size(d->m_surface, size.width(), size.height());
    }
    emit sizeChanged(size);
}

void LayerSurface::setAnchor(qreal left, qreal top, qreal right, qreal bottom) {
    QPointF anchor(left, top, right, bottom);
    if (d->m_options.anchor == anchor) return;
    d->m_options.anchor = anchor;
    // 注意：这里需要重新应用锚点设置
    // 为了简化，我们只更新选项
    emit anchorChanged(left, top, right, bottom);
}

void LayerSurface::setMargin(int left, int top, int right, int bottom) {
    QMargins margin(left, top, right, bottom);
    if (d->m_options.margin == margin) return;
    d->m_options.margin = margin;
    if (d->m_surface) {
        zwlr_layer_surface_v1_set_margin(d->m_surface, left, top, right, bottom);
    }
    emit marginChanged(left, top, right, bottom);
}

void LayerSurface::setExclusiveZone(qreal zone) {
    if (qFuzzyCompare(d->m_options.exclusiveZone, zone)) return;
    d->m_options.exclusiveZone = zone;
    if (d->m_surface && zone >= 0) {
        zwlr_layer_surface_v1_set_exclusive_zone(d->m_surface, zone);
    }
    emit exclusiveZoneChanged(zone);
}

void LayerSurface::setKeyboardInteractivity(bool enabled) {
    if (d->m_options.keyboardInteractivity == enabled) return;
    d->m_options.keyboardInteractivity = enabled;
    if (d->m_surface) {
        if (enabled) {
            zwlr_layer_surface_v1_set_keyboard_interactivity(d->m_surface);
        } else {
            // 没有直接的方法来禁用键盘交互性，可能需要重新创建 surface
        }
    }
    emit keyboardInteractivityChanged(enabled);
}

bool LayerSurface::show() {
    if (!d->m_initialized) {
        if (!init()) return false;
    }
    
    if (!d->m_surface) return false;
    
    // 显示表面
    wl_surface_commit(wl_surface_get_user_data(d->m_surface)); // 简化处理
    d->m_visible = true;
    emit visibleChanged(true);
    return true;
}

void LayerSurface::hide() {
    if (!d->m_visible) return;
    d->m_visible = false;
    // 隐藏表面 - 实际上我们可能需要取消映射或设置为不可见
    // 为了简化，我们这里只是更新状态
    emit visibleChanged(false);
}

bool LayerSurface::isVisible() const {
    return d->m_visible;
}

QWindow* LayerSurface::window() const {
    return d->m_window;
}

// LayerShellManager 实现
class LayerShellManager::Impl {
public:
    Impl() {
        // 尝试初始化
        init();
    }
    
    bool init() {
        if (m_initialized) return true;
        
        // 检查是否在 Wayland 会话中
        const char* sessionType = qgetenv("XDG_SESSION_TYPE");
        if (QString::fromLocal8Bit(sessionType).toLower() != "wayland") {
            qDebug() << "Not in Wayland session, layer-shell not available";
            return false;
        }
        
        m_initialized = true;
        return true;
    }
    
    void shutdown() {
        m_initialized = false;
    }
    
    std::unique_ptr<LayerSurface> createSurface(QWindow* window,
                                              const LayerSurfaceOptions& options) {
        if (!m_initialized) {
            if (!init()) return nullptr;
        }
        
        auto surface = std::make_unique<LayerSurface>(window, options);
        if (surface->init()) {
            return surface;
        }
        return nullptr;
    }
    
    bool isAvailable() const {
        return m_initialized;
    }
    
    wl_display* display() const {
        return m_display;
    }
    
private:
    bool m_initialized = false;
    wl_display* m_display = nullptr;
};

LayerShellManager& LayerShellManager::instance() {
    static LayerShellManager inst;
    return inst;
}

LayerShellManager::LayerShellManager() : d(std::make_unique<Impl>()) {}
LayerShellManager::~LayerShellManager() {
    shutdown();
}

bool LayerShellManager::init() {
    return d->init();
}

void LayerShellManager::shutdown() {
    d->shutdown();
}

std::unique_ptr<LayerSurface> LayerShellManager::createSurface(QWindow* window,
                                                             const LayerSurfaceOptions& options) {
    return d->createSurface(window, options);
}

bool LayerShellManager::isAvailable() const {
    return d->isAvailable();
}

wl_display* LayerShellManager::display() const {
    return d->display();
}

// 工具函数实现
namespace utils {

QString layerToString(Layer layer) {
    switch (layer) {
        case Layer::Background: return "background";
        case Layer::Bottom: return "bottom";
        case Layer::Top: return "top";
        case Layer::Overlay: return "overlay";
        default: return "unknown";
    }
}

std::optional<Layer> stringToLayer(const QString& str) {
    QString lower = str.toLower();
    if (lower == "background") return Layer::Background;
    if (lower == "bottom") return Layer::Bottom;
    if (lower == "top") return Layer::Top;
    if (lower == "overlay") return Layer::Overlay;
    return std::nullopt;
}

LayerSurfaceOptions defaultOptionsForLayer(Layer layer) {
    LayerSurfaceOptions options;
    options.layer = layer;
    switch (layer) {
        case Layer::Background:
            options.namespace_ = "desktop-background";
            options.description = "Desktop Background";
            options.anchor = QPointF(1.0, 1.0, 1.0, 1.0); // 全屏锚点
            break;
        case Layer::Bottom:
            options.namespace_ = "panel";
            options.description = "Panel/Taskbar";
            options.anchor = QPointF(0.0, 1.0, 1.0, 0.0); // 底部锚点
            options.exclusiveZone = 30; // 典型任务栏高度
            break;
        case Layer::Top:
            options.namespace_ = "notification";
            options.description = "Notification Area";
            options.anchor = QPointF(1.0, 0.0, 0.0, 0.0); // 顶部锚点
            break;
        case Layer::Overlay:
            options.namespace_ = "overlay";
            options.description = "Overlay (e.g., lock screen)";
            options.anchor = QPointF(1.0, 1.0, 1.0, 1.0); // 全屏锚点
            options.keyboardInteractivity = true;
            break;
    }
    return options;
}

} // namespace utils

} // namespace explorer::layer

#include "LayerShell.moc"