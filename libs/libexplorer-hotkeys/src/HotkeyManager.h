#pragma once

#include <QObject>
#include <QString>
#include <QKeySequence>
#include <QMap>
#include <QSet>
#include <functional>
#include <QVector>
#include <QMutex>

namespace explorer::hotkeys {

// 热键定义
struct Hotkey {
    QString id;              // 唯一标识
    QKeySequence sequence;   // 键序列 (如 Ctrl+Alt+T)
    QString description;     // 描述
    bool enabled = true;     // 是否启用
    QString context;         // 上下文 (全局、应用特定等)
};

// 热键管理器 - 处理热键注册、解绑和事件处理
class HotkeyManager : public QObject {
    Q_OBJECT
public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    // 初始化
    bool init();
    void shutdown();

    // 注册热键
    bool registerHotkey(const Hotkey& hotkey);
    bool unregisterHotkey(const QString& id);
    bool updateHotkey(const QString& id, const Hotkey& hotkey);

    // 检查热键是否已注册
    bool isRegistered(const QString& id) const;
    Hotkey getHotkey(const QString& id) const;

    // 获取所有热键
    QVector<Hotkey> allHotkeys() const;
    QVector<Hotkey> hotkeysForContext(const QString& context) const;

    // 启用/禁用热键
    bool setEnabled(const QString& id, bool enabled);
    bool setContextEnabled(const QString& context, bool enabled);

    // 全局启用/禁用
    void setGlobalEnabled(bool enabled);

signals:
    void hotkeyActivated(const QString& id);
    void hotkeyRegistered(const QString& id);
    void hotkeyUnregistered(const QString& id);
    void hotkeyUpdated(const QString& id);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

// 全局快捷方式 - 简化的热键注册接口
class GlobalShortcut : public QObject {
    Q_OBJECT
public:
    explicit GlobalShortcut(QObject* parent = nullptr);
    ~GlobalShortcut() override;

    // 注册全局快捷方式
    static bool registerShortcut(const QString& id, const QKeySequence& sequence,
                               const std::function<void()>& callback,
                               const QString& description = "");

    static bool unregisterShortcut(const QString& id);
    static bool isRegistered(const QString& id);

private:
    class Impl;
    static std::unique_ptr<Impl> d;
};

} // namespace explorer::hotkeys

#include "HotkeyManager.moc"
#include "GlobalShortcut.moc"