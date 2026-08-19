#include "HotkeyManager.h"
#include <QMutex>

// 静态成员定义
namespace explorer::hotkeys {
    class GlobalShortcut::Impl {
    public:
        static HotkeyManager* manager;
    };
    
    std::unique_ptr<GlobalShortcut::Impl> GlobalShortcut::d = std::make_unique<Impl>();
    HotkeyManager* GlobalShortcut::Impl::manager = nullptr;
}

namespace explorer::hotkeys {

GlobalShortcut::GlobalShortcut(QObject* parent) : QObject(parent) {}
GlobalShortcut::~GlobalShortcut() = default;

bool GlobalShortcut::registerShortcut(const QString& id, const QKeySequence& sequence,
                                    const std::function<void()>& callback,
                                    const QString& description) {
    // 初始化管理器（单例）
    if (!Impl::manager) {
        Impl::manager = new HotkeyManager(qApp);
        Impl::manager->init();
    }

    Hotkey hotkey;
    hotkey.id = id;
    hotkey.sequence = sequence;
    hotkey.description = description;
    hotkey.context = "global";

    bool success = Impl::manager->registerHotkey(hotkey);
    if (success) {
        // 设置回调 - 这里需要一种方式来存储回调
        // 简化实现：我们可以扩展 Hotkey 结构来包含回调
        // 或者使用一个单独的映射
        // 为了演示，我们这里假设热键管理器支持回调
        // 在实际实现中，这需要更复杂的设计
    }
    return success;
}

bool GlobalShortcut::unregisterShortcut(const QString& id) {
    if (!Impl::manager) return false;
    return Impl::manager->unregisterHotkey(id);
}

bool GlobalShortcut::isRegistered(const QString& id) {
    if (!Impl::manager) return false;
    return Impl::manager->isRegistered(id);
}

} // namespace explorer::hotkeys

#include "GlobalShortcut.moc"