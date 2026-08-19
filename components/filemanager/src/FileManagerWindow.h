#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QSplitter>
#include <QTreeView>
#include <QListView>
#include <QToolBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QLineEdit>
#include <QAction>
#include <QStandardPaths>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTimer>

#include <libexplorer-ipc/DBusInterface.h>

namespace explorer::components {

class FileManagerWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit FileManagerWindow(QWidget* parent = nullptr);
    ~FileManagerWindow() override;

private:
    void setupUI();
    void setupActions();
    void setupMenus();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();

    // 文件操作槽
    void onNewFolder();
    void onDelete();
    void onRefresh();
    void onHome();
    void onGoUp();
    void onPathChanged(const QString& path);
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onDoubleClicked(const QModelIndex& index);

    // WindowManager 相关
    void connectToWindowManager();
    void registerWindow();
    void unregisterWindow();
    void updateWindowTitle(const QString& title);
    void handleWindowActivateRequested(const QString& windowId);

    // UI 组件
    QTabWidget* m_tabWidget = nullptr;
    QToolBar* m_toolBar = nullptr;
    QStatusBar* m_statusBar = nullptr;
    QMenuBar* m_menuBar = nullptr;
    QLineEdit* m_pathBar = nullptr;

    // 当前选项卡的组件
    struct TabContents {
        QFileSystemModel* model = nullptr;
        QTreeView* treeView = nullptr;
        QListView* listView = nullptr;
        QSplitter* splitter = nullptr;
        QLabel* statusLabel = nullptr;
    };

    QMap<int, TabContents> m_tabContents; // tab index -> contents

    // WindowManager 连接
    std::unique_ptr<explorer::ipc::DBusInterface> m_windowManagerInterface;
    QString m_windowId;
};

} // namespace explorer::components

#include "FileManagerWindow.moc"