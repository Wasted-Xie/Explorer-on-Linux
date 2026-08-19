#include "HotkeyManager.h"
// Hotkey 是一个简单的结构体，不需要额外的实现
// 但我们创建这个文件来保持一致性

namespace explorer::hotkeys {

// Hotkey 结构体已经在头文件中定义了
// 这里可以添加一些辅助函数如果需要的话

QString toString(const Hotkey& hotkey) {
    return QString("%1: %2 (%3)").arg(hotkey.id, hotkey.sequence.toString(), hotkey.description);
}

Hotkey fromString(const QString& str) {
    // 简单的解析实现
    Hotkey hotkey;
    // 实际实现需要更复杂的解析
    return hotkey;
}

} // namespace explorer::hotkeys