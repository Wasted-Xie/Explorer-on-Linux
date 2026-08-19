#include "HotkeyManager.h"
#include <QKeySequence>
#include <QWidget>
#include <QShortcut>
#include <QApplication>
#include <QDebug>
#include <QMutex>

namespace explorer::hotkeys {

class HotkeyManager::Impl {
public:
    Impl(HotkeyManager* owner) : q(owner) {
        // 初始化时尝试设置全局捕获
        // 注意：真正的全局热键需要平台特定的实现
        // 这里我们使用应用级别的快捷方式作为简化实现
    }

    bool registerHotkeyImpl(const Hotkey& hotkey) {
        if (!hotkey.enabled) return true;

        // 检查是否冲突
        QString keyStr = hotkey.sequence.toString(QKeySequence::NativeText);
        if (registeredSequences.contains(keyStr)) {
            qWarning() << "Hotkey sequence already registered:" << keyStr;
            return false;
        }

        // 创建快捷方式
        // 注意：QShortcut 默认是窗口级别的，不是真正的全局
        // 真正的全局热键需要使用 X11 GrabKey 或 KGlobalAccel
        QShortcut* shortcut = new QShortcut(hotkey.sequence, q);
        connect(shortcut, &QShortcut::activated, this, [this, hotkey]() {
            if (hotkey.enabled) {
                emit q->hotkeyActivated(hotkey.id);
            }
        });

        // 存储
        registeredHotkeys[hotkey.id] = shortcut;
        registeredSequences.insert(keyStr, hotkey.id);
        hotkeyMap[hotkey.id] = hotkey;

        return true;
    }

    void unregisterHotkeyImpl(const QString& id) {
        auto it = registeredHotkeys.find(id);
        if (it != registeredHotkeys.end()) {
            QShortcut* shortcut = it.value();
            registeredHotkeys.erase(it);

            // 移除序列映射
            auto hkIt = hotkeyMap.find(id);
            if (hkIt != hotkeyMap.end()) {
                QString keyStr = hkIt.value().sequence.toString(QKeySequence::NativeText);
                registeredSequences.remove(keyStr);
                hotkeyMap.erase(hkIt);
            }

            shortcut->deleteLater();
        }
    }

    HotkeyManager* q;
    QMap<QString, QShortcut*> registeredHotkeys;
    QMap<QString, QString> registeredSequences; // sequence -> id
    QMap<QString, Hotkey> hotkeyMap;
    bool globalEnabled = true;
    QMutex mutex;
};

HotkeyManager::HotkeyManager(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
HotkeyManager::~HotkeyManager() {
    shutdown();
}

bool HotkeyManager::init() {
    // 可以在这里进行平台特定的初始化
    // 比如注册到 KGlobalAccel 或设置 X11 事件过滤器
    return true;
}

void HotkeyManager::shutdown() {
    QMutexLocker lock(&d->mutex);
    qDeleteAll(d->registeredHotkeys);
    d->registeredHotkeys.clear();
    d->registeredSequences.clear();
    d->hotkeyMap.clear();
}

bool HotkeyManager::registerHotkey(const Hotkey& hotkey) {
    QMutexLocker lock(&d->mutex);
    return d->registerHotkeyImpl(hotkey);
}

bool HotkeyManager::unregisterHotkey(const QString& id) {
    QMutexLocker lock(&d->mutex);
    d->unregisterHotkeyImpl(id);
    return true;
}

bool HotkeyManager::updateHotkey(const QString& id, const Hotkey& hotkey) {
    QMutexLocker lock(&d->mutex);
    unregisterHotkeyImpl(id);
    return registerHotkeyImpl(hotkey);
}

bool HotkeyManager::isRegistered(const QString& id) const {
    QMutexLocker lock(&d->mutex);
    return d->registeredHotkeys.contains(id);
}

Hotkey HotkeyManager::getHotkey(const QString& id) const {
    QMutexLocker lock(&d->mutex);
    auto it = d->hotkeyMap.find(id);
    return it != d->hotkeyMap.end() ? it.value() : Hotkey();
}

QVector<Hotkey> HotkeyManager::allHotkeys() const {
    QMutexLocker lock(&d->mutex);
    return d->hotkeyMap.values().toVector();
}

QVector<Hotkey> HotkeyManager::hotkeysForContext(const QString& context) const {
    QMutexLocker lock(&d->mutex);
    QVector<Hotkey> result;
    for (const Hotkey& hk : d->hotkeyMap.values()) {
        if (hk.context == context) {
            result.append(hk);
        }
    }
    return result;
}

bool HotkeyManager::setEnabled(const QString& id, bool enabled) {
    QMutexLocker lock(&d->mutex);
    auto it = d->hotkeyMap.find(id);
    if (it != d->hotkeyMap.end()) {
        it.value().enabled = enabled;
        // 更新快捷方式状态
        auto shortcutIt = d->registeredHotkeys.find(id);
        if (shortcutIt != d->registeredHotkeys.end()) {
            shortcutIt.value()->setEnabled(enabled && d->globalEnabled);
        }
        return true;
    }
    return false;
}

bool HotkeyManager::setContextEnabled(const QString& context, bool enabled) {
    QMutexLocker lock(&d->mutex);
    bool changed = false;
    for (auto it = d->hotkeyMap.begin(); it != d->hotkeyMap.end(); ++it) {
        if (it.value().context == context) {
            it.value().enabled = enabled;
            auto shortcutIt = d->registeredHotkeys.find(it.key());
            if (shortcutIt != d->registeredHotkeys.end()) {
                shortcutIt.value()->setEnabled(enabled && d->globalEnabled);
                changed = true;
            }
        }
    }
    return changed;
}

void HotkeyManager::setGlobalEnabled(bool enabled) {
    QMutexLocker lock(&d->mutex);
    d->globalEnabled = enabled;
    for (auto it = d->registeredHotkeys.begin(); it != d->registeredHotkeys.end(); ++it) {
        auto hkIt = d->hotkeyMap.find(it.key());
        if (hkIt != d->hotkeyMap.end()) {
            it.value()->setEnabled(hkIt.value().enabled && enabled);
        }
    }
}

} // namespace explorer::hotkeys

#include "HotkeyManager.moc"