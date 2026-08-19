#include "FileManagerWindow.h"
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QDesktopServices>
#include <QTimer>

namespace explorer::components {

FileManagerWindow::FileManagerWindow(QWidget* parent)
    : QMainWindow(parent) {
    setupUI();
    setupActions();
    setupMenus();
    setupToolBar();
    setupStatusBar();
    setupConnections();

    // 连接到 WindowManager
    connectToWindowManager();
    
    // 创建初始选项卡
    addTab(QStandardPaths::homeLocation());
    
    // 注册窗口（延迟一点，确保 WindowManager 已就绪）
    QTimer::singleShot(100, this, &FileManagerWindow::registerWindow);
}

FileManagerWindow::~FileManagerWindow() {
    unregisterWindow();
    
    // 清理所有选项卡
    for (auto it = m_tabContents.begin(); it != m_tabContents.end(); ++it) {
        delete it.value().model;
        delete it.value().treeView;
        delete it.value().listView;
        delete it.value().splitter;
        delete it.value().statusLabel;
    }
    m_tabContents.clear();
}

void FileManagerWindow::setupUI() {
    setWindowTitle("文件管理器 - Explorer Linux");
    setMinimumSize(800, 600);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    setCentralWidget(m_tabWidget);
}

void FileManagerWindow::setupActions() {
    // 动作将在 setupMenus 中创建
}

void FileManagerWindow::setupMenus() {
    m_menuBar = new QMenuBar(this);
    setMenuBar(m_menuBar);

    // 文件菜单
    QMenu* fileMenu = m_menuBar->addMenu("文件(&F)");
    
    QAction* newFolderAction = fileMenu->addAction("新建文件夹(&N)");
    newFolderAction->setShortcut(QKeySequence::New);
    connect(newFolderAction, &QAction::triggered, this, &FileManagerWindow::onNewFolder);

    QAction* deleteAction = fileMenu->addAction("删除(&D)");
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &FileManagerWindow::onDelete);

    fileMenu->addSeparator();

    QAction* refreshAction = fileMenu->addAction("刷新(&R)");
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &FileManagerWindow::onRefresh);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    // 位置菜单
    QMenu* locationMenu = m_menuBar->addMenu("位置(&L)");
    
    QAction* homeAction = locationMenu->addAction("主目录(&H)");
    homeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::KEY_H));
    connect(homeAction, &QAction::triggered, this, &FileManagerWindow::onHome);

    QAction* upAction = locationMenu->addAction("上一级(&U)");
    upAction->setShortcut(QKeySequence(Qt::ALT | Qt::KEY_UP));
    connect(upAction, &QAction::triggered, this, &FileManagerWindow::onGoUp);

    // 视图菜单
    QMenu* viewMenu = m_menuBar->addMenu("视图(&V)");
    
    QAction* toggleViewAction = viewMenu->addAction("切换视图(&T)");
    toggleViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::KEY_1));
    // TODO: 实现视图切换
}

void FileManagerWindow::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setMovable(false);
    addToolBar(m_toolBar);

    // 返回按钮
    QAction* backAction = m_toolBar->addAction("←", this, &FileManagerWindow::onGoUp);
    backAction->setToolTip("返回上一级");

    // 前进按钮 (简化实现)
    QAction* forwardAction = m_toolBar->addAction("→");
    forwardAction->setToolTip("前进");
    forwardAction->setEnabled(false);

    m_toolBar->addSeparator();

    // 地址栏
    m_pathBar = new QLineEdit(this);
    m_pathBar->setMinimumWidth(300);
    m_toolBar->addWidget(m_pathBar);

    QAction* goAction = m_toolBar->addAction("转到");
    connect(goAction, &QAction::triggered, this, &FileManagerWindow::onPathChanged);

    m_toolBar->addSeparator();

    // 新建文件夹
    QAction* newFolderAction = m_toolBar->addAction("新建文件夹");
    connect(newFolderAction, &QAction::triggered, this, &FileManagerWindow::onNewFolder);

    // 删除
    QAction* deleteAction = m_toolBar->addAction("删除");
    connect(deleteAction, &QAction::triggered, this, &FileManagerWindow::onDelete);

    m_toolBar->addSeparator();

    // 刷新
    QAction* refreshAction = m_toolBar->addAction("刷新");
    connect(refreshAction, &QAction::triggered, this, &FileManagerWindow::onRefresh);
}

void FileManagerWindow::setupStatusBar() {
    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);

    // 状态标签
    QLabel* statusLabel = new QLabel(this);
    m_statusBar->addWidget(statusLabel, 1); // 拉伸因子为1，使其占用剩余空间
}

void FileManagerWindow::setupConnections() {
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        closeTab(index);
    });

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        updatePathBar(index);
    });
}

void FileManagerWindow::addTab(const QString& path) {
    int tabIndex = m_tabWidget->count();
    QString tabTitle = QFileInfo(path).fileName();
    if (tabTitle.isEmpty()) tabTitle = path;
    
    m_tabWidget->addTab(new QWidget(this), tabTitle);
    m_tabWidget->setTabToolTip(tabIndex, path);

    // 创建选项卡内容
    TabContents contents;
    contents.model = new QFileSystemModel(this);
    contents.splitter = new QSplitter(Qt::Horizontal, this);
    contents.treeView = new QTreeView(contents.splitter);
    contents.listView = new QListView(contents.splitter);
    contents.statusLabel = new QLabel(this);

    // 设置模型
    contents.model->setRootPath(path);
    contents.treeView->setModel(contents.model);
    contents.listView->setModel(contents.model);

    // 设置根索引
    QModelIndex rootIndex = contents.model->index(path);
    contents.treeView->setRootIndex(rootIndex);
    contents.listView->setRootIndex(rootIndex);

    // 设置表头
    contents.treeView->header()->setStretchLastSection(false);
    contents.treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    contents.treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    contents.treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    contents.treeView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    contents.listView->setViewMode(QListView::ListMode);
    contents.listView->setUniformItemSizes(true);
    contents.listView->setWrapping(false);

    // 选择模式
    contents.treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    contents.listView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // 连接选择变化
    QItemSelectionModel* treeSelection = contents.treeView->selectionModel();
    QItemSelectionModel* listSelection = contents.listView->selectionModel();
    connect(treeSelection, &QItemSelectionModel::selectionChanged,
            this, &FileManagerWindow::onSelectionChanged);
    connect(listSelection, &QItemSelectionModel::selectionChanged,
            this, &FileManagerWindow::onSelectionChanged);

    // 连接双击打开
    connect(contents.treeView, &QTreeView::doubleClicked,
            this, &FileManagerWindow::onDoubleClicked);
    connect(contents.listView, &QListView::doubleClicked,
            this, &FileManagerWindow::onDoubleClicked);

    // 存储内容
    m_tabContents[tabIndex] = contents;

    // 设置为当前选项卡
    m_tabWidget->setCurrentIndex(tabIndex);

    // 更新路径栏
    updatePathBar(tabIndex);
}

void FileManagerWindow::closeTab(int index) {
    if (index < 0 || index >= m_tabWidget->count()) return;

    // 清理资源
    auto it = m_tabContents.find(index);
    if (it != m_tabContents.end()) {
        delete it.value().model;
        delete it.value().treeView;
        delete it.value().listView;
        delete it.value().splitter;
        delete it.value().statusLabel;
        m_tabContents.erase(it);
    }

    m_tabWidget->removeTab(index);
}

void FileManagerWindow::updatePathBar(int tabIndex) {
    auto it = m_tabContents.find(tabIndex);
    if (it == m_tabContents.end()) return;

    const TabContents& contents = it.value();
    QString path = contents.model->rootPath();
    m_pathBar->setText(path);
}

void FileManagerWindow::onDoubleClicked(const QModelIndex& index) {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) return;

    auto it = m_tabContents.find(currentIndex);
    if (it == m_tabContents.end()) return;

    const TabContents& contents = it.value();
    if (!index.isValid()) return;

    // 检查是否是目录
    if (contents.model->isDir(index)) {
        // 如果是目录，导航到该目录
        QString path = contents.model->filePath(index);
        contents.model->setRootPath(path);
        contents.treeView->setRootIndex(contents.model->index(path));
        contents.listView->setRootIndex(contents.model->index(path));
        updatePathBar(currentIndex);
    } else {
        // 如果是文件，使用默认应用打开
        QString path = contents.model->filePath(index);
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void FileManagerWindow::onNewFolder() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) return;

    auto it = m_tabContents.find(currentIndex);
    if (it == m_tabContents.end()) return;

    TabContents& contents = it.value();
    bool ok;
    QString folderName = QInputDialog::getText(this, "新建文件夹", "文件夹名称:",
                                               QLineEdit::Normal, "", &ok);
    if (ok && !folderName.isEmpty()) {
        QString currentPath = contents.model->rootPath();
        QString newPath = currentPath + "/" + folderName;
        
        if (contents.model->mkdir(newPath)) {
            // 刷新当前目录
            contents.model->refresh();
        } else {
            QMessageBox::warning(this, "错误", "创建文件夹失败");
        }
    }
}

void FileManagerWindow::onDelete() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) return;

    auto it = m_tabContents.find(currentIndex);
    if (it == m_tabContents.end()) return;

    TabContents& contents = it.value();
    QItemSelectionModel* selection = contents.treeView->selectionModel();
    if (!selection->hasSelection()) {
        selection = contents.listView->selectionModel();
    }

    if (!selection->hasSelection()) {
        QMessageBox::information(this, "提示", "请选择要删除的文件");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除", 
                                  "确定要删除选中的项目吗？",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QModelIndexList indexes = selection->selectedIndexes();
        // 去重（同一文件在树视图和列表视图中可能出现两次）
        QSet<QString> uniquePaths;
        for (const QModelIndex& index : indexes) {
            if (index.isValid()) {
                QString path = contents.model->filePath(index);
                uniquePaths.insert(path);
            }
        }

        QStringList paths;
        for (const QString& path : uniquePaths) {
            paths << path;
        }

        if (!paths.isEmpty()) {
            bool allSuccess = true;
            for (const QString& path : paths) {
                bool success = false;
                QFileInfo info(path);
                if (info.isDir()) {
                    QDir dir(path);
                    success = dir.removeRecursively();
                } else {
                    success = QFile::remove(path);
                }
                if (!success) allSuccess = false;
            }
            
            if (allSuccess) {
                // 刷新当前目录
                contents.model->refresh();
            } else {
                QMessageBox::warning(this, "错误", "部分文件删除失败");
            }
        }
    }
}

void FileManagerWindow::onRefresh() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) return;

    auto it = m_tabContents.find(currentIndex);
    if (it == m_tabContents.end()) return;

    TabContents& contents = it.value();
    contents.model->refresh();
}

void FileManagerWindow::onHome() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) return;

    QString homePath = QStandardPaths::homeLocation();
    addTab(homePath);
}

void FileManagerWindow::onGoUp() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) return;

    auto it = m_tabContents.find(currentIndex);
    if (it == m_tabContents.end()) return;

    TabContents& contents = it.value();
    QString currentPath = contents.model->rootPath();
    QFileInfo info(currentPath);
    QString parentPath = info.absolutePath();

    if (parentPath != currentPath) { // 避免在根目录时无限循环
        contents.model->setRootPath(parentPath);
        contents.treeView->setRootIndex(contents.model->index(parentPath));
        contents.listView->setRootIndex(contents.model->index(parentPath));
        updatePathBar(currentIndex);
    }
}

void FileManagerWindow::onPathChanged(const QString& path) {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) return;

    QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        QMessageBox::warning(this, "错误", "路径不存在或不是目录");
        return;
    }

    auto it = m_tabContents.find(currentIndex);
    if (it == m_tabContents.end()) return;

    TabContents& contents = it.value();
    contents.model->setRootPath(path);
    contents.treeView->setRootIndex(contents.model->index(path));
    contents.listView->setRootIndex(contents.model->index(path));
    updatePathBar(currentIndex);
}

void FileManagerWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) return;

    auto it = m_tabContents.find(currentIndex);
    if (it == m_tabContents.end()) return;

    TabContents& contents = it.value();
    int count = selected.indexes().count();
    if (count > 0) {
        // 计算选中文件的总大小
        qint64 totalSize = 0;
        int fileCount = 0;
        for (const QModelIndex& index : selected.indexes()) {
            if (index.isValid()) {
                QString path = contents.model->filePath(index);
                QFileInfo info(path);
                if (info.isFile()) {
                    totalSize += info.size();
                    fileCount++;
                }
            }
        }

        QString statusText = QString("%d 项已选中").arg(count);
        if (fileCount > 0) {
            statusText += ", 大小: " + QString::number(totalSize) + " 字节";
        }
        contents.statusLabel->setText(statusText);
        // 更新状态栏
        m_statusBar->showMessage(statusText, 2000);
    } else {
        contents.statusLabel->clear();
        m_statusBar->clearMessage();
    }
}

// WindowManager 相关方法

void FileManagerWindow::connectToWindowManager() {
    m_windowManagerInterface = std::make_unique<explorer::ipc::DBusInterface>(
        "org.explorer.WindowManager",
        "/org/explorer/WindowManager",
        "org.explorer.WindowManager",
        QDBusConnection::sessionBus(),
        this
    );
    
    if (!m_windowManagerInterface->isValid()) {
        qWarning() << "Failed to connect to WindowManager service";
        m_windowManagerInterface.reset();
        return;
    }
    
    qInfo() << "FileManagerWindow connected to WindowManager";
    
    // 连接 windowActivateRequested 信号
    m_windowManagerInterface->connectSignal("windowActivateRequested", [this](const QString& windowId) {
        handleWindowActivateRequested(windowId);
    });
    
    // 连接 windowMinimizeRequested 信号
    m_windowManagerInterface->connectSignal("windowMinimizeRequested", [this](const QString& windowId) {
        if (windowId == m_windowId) {
            showMinimized();
        }
    });
    
    // 连接 windowMaximizeRequested 信号
    m_windowManagerInterface->connectSignal("windowMaximizeRequested", [this](const QString& windowId) {
        if (windowId == m_windowId) {
            if (isMaximized()) {
                showNormal();
            } else {
                showMaximized();
            }
        }
    });
    
    // 连接 windowCloseRequested 信号
    m_windowManagerInterface->connectSignal("windowCloseRequested", [this](const QString& windowId) {
        if (windowId == m_windowId) {
            close();
        }
    });
}

void FileManagerWindow::registerWindow() {
    if (!m_windowManagerInterface || !m_windowManagerInterface->isValid()) {
        qWarning() << "WindowManager interface not available, retrying...";
        connectToWindowManager();
        if (!m_windowManagerInterface || !m_windowManagerInterface->isValid()) {
            qWarning() << "Failed to register window: WindowManager not available";
            return;
        }
    }
    
    // 注册窗口
    auto reply = m_windowManagerInterface->call("registerWindow", 
        "文件管理器 - Explorer Linux",  // title
        "folder",                      // icon
        "FileManager",                 // appName
        false,                         // isMinimized
        false,                         // isMaximized
        true                           // isActive
    );
    
    if (!reply.isValid()) {
        qWarning() << "Failed to register window with WindowManager:" << reply.error().message();
        return;
    }
    
    m_windowId = reply.value().toString();
    if (m_windowId.isEmpty()) {
        qWarning() << "WindowManager returned empty window ID";
        return;
    }
    
    qInfo() << "FileManagerWindow registered with WindowManager, ID:" << m_windowId;
    
    // 监听窗口状态变化，更新本地状态
    connect(this, &QWidget::windowTitleChanged, this, &FileManagerWindow::updateWindowTitle);
}

void FileManagerWindow::unregisterWindow() {
    if (m_windowId.isEmpty() || !m_windowManagerInterface || !m_windowManagerInterface->isValid()) {
        return;
    }
    
    auto reply = m_windowManagerInterface->call("unregisterWindow", m_windowId);
    if (!reply.isValid()) {
        qWarning() << "Failed to unregister window from WindowManager:" << reply.error().message();
    } else {
        qInfo() << "FileManagerWindow unregistered from WindowManager, ID:" << m_windowId;
    }
    
    m_windowId.clear();
}

void FileManagerWindow::updateWindowTitle(const QString& title) {
    if (m_windowId.isEmpty() || !m_windowManagerInterface || !m_windowManagerInterface->isValid()) {
        return;
    }
    
    auto reply = m_windowManagerInterface->call("setWindowTitle", m_windowId, title);
    if (!reply.isValid()) {
        qWarning() << "Failed to update window title:" << reply.error().message();
    }
}

void FileManagerWindow::handleWindowActivateRequested(const QString& windowId) {
    if (windowId != m_windowId) return;
    
    qInfo() << "Activate requested for FileManagerWindow";
    
    // 如果窗口最小化，恢复它
    if (isMinimized()) {
        showNormal();
    }
    
    // 激活窗口（带到前台）
    activateWindow();
    raise();
    
    // 更新 WindowManager 中的激活状态
    m_windowManagerInterface->call("setWindowActive", m_windowId, true);
}

} // namespace explorer::components

#include "FileManagerWindow.moc"