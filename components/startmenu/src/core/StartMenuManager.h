#pragma once

#include <QObject>
#include <QList>
#include <QPair>
#include <QPoint>
#include "libs/libexplorer-ipc/src/DBusInterface.h"
#include "libs/libexplorer-ipc/src/MessageBus.h"
#include "libs/libexplorer-ipc/src/ServiceWatcher.h"
#include "libs/libexplorer-core/src/signals/SignalDispatcher.h"
#include "libs/libexplorer-utils/src/FileUtils.h"
#include "StartMenuWindow.h"

namespace explorer::startmenu {

struct DesktopEntry {
    QString name;
    QString exec;
    QString icon;
    QString comment;
    QStringList categories;
    bool noDisplay = false;
    bool terminal = false;
};

class StartMenuManager : public QObject {
    Q_OBJECT
public:
    explicit StartMenuManager(QObject* parent = nullptr);
    ~StartMenuManager() override;

    // 初始化
    bool init();
    void shutdown();

    // 显示/隐藏菜单
    void showAt(const QPoint& globalPos);
    void hide();
    void toggle(const QPoint& globalPos);

    // 重新加载应用列表
    void reloadApplications();

    // 检查是否可见
    bool isVisible() const;

signals:
    void applicationLaunched(const QString& exec);
    void visibilityChanged(bool visible);

private slots:
    void onShowRequested(const QPoint& pos);
    void onHideRequested();
    void onToggleRequested(const QPoint& pos);
    void onDaemonAppeared();
    void onDaemonVanished();
    void onApplicationLaunched(const QString& exec);

private:
    void registerWithDaemon();
    void unregisterFromDaemon();
    QList<DesktopEntry> parseDesktopFiles(const QString& dir);
    DesktopEntry parseDesktopFile(const QString& filePath);
    QIcon loadIcon(const QString& iconName);
    void setupDBusSignals();

    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::startmenu

#include "StartMenuManager.moc"