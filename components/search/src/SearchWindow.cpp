#include "SearchWindow.h"
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QAbstractItemView>
#include <QPushButton>
#include <QLineEdit>
#include <QListView>

namespace explorer::search {

SearchWindow::SearchWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setupUI();
    setupLayerSurface();
    setupIPC();
    setupConnections();
    loadSettings();

    m_initialized = true;
}

SearchWindow::~SearchWindow() {
    saveSettings();

    if (m_layerSurface) {
        m_layerSurface->hide();
        m_layerSurface->shutdown();
    }

    if (m_dbusService) {
        m_dbusService->unregisterObject("/org/explorer/Search");
    }
}

void SearchWindow::setupUI() {
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName("centralWidget");
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 输入区域
    m_inputWidget = new QWidget();
    m_inputWidget->setObjectName("inputWidget");
    m_inputWidget->setFixedHeight(56);

    m_inputLayout = new QHBoxLayout(m_inputWidget);
    m_inputLayout->setContentsMargins(12, 8, 12, 8);
    m_inputLayout->setSpacing(8);

    // 搜索图标
    m_searchIconLabel = new explorer::ui::Label("🔍");
    m_searchIconLabel->setObjectName("searchIconLabel");
    m_searchIconLabel->setFixedSize(24, 24);
    m_searchIconLabel->setAlignment(Qt::AlignCenter);
    m_searchIconLabel->setLabelType(explorer::ui::Label::LabelType::Body);

    // 搜索输入框
    m_lineEdit = new QLineEdit();
    m_lineEdit->setObjectName("searchLineEdit");
    m_lineEdit->setPlaceholderText("搜索应用程序、文件、文件夹...");
    m_lineEdit->setClearButtonEnabled(false); // 我们使用自定义清除按钮

    // 清除按钮
    m_clearButton = new explorer::ui::Button("✕");
    m_clearButton->setObjectName("clearButton");
    m_clearButton->setFixedSize(28, 28);
    m_clearButton->setButtonType(explorer::ui::Button::ButtonType::Link);
    m_clearButton->setToolTip("清除搜索");
    m_clearButton->setVisible(false);

    m_inputLayout->addWidget(m_searchIconLabel);
    m_inputLayout->addWidget(m_lineEdit, 1);
    m_inputLayout->addWidget(m_clearButton);

    // 结果列表
    m_listView = new QListView();
    m_listView->setObjectName("searchResultsView");
    m_listView->setAlternatingRowColors(false);
    m_listView->setUniformItemSizes(true);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listView->setFocusPolicy(Qt::NoFocus); // 焦点保持在输入框

    // 模型和委托
    m_model = new SearchModel(this);
    m_delegate = new SearchDelegate(this);

    m_listView->setModel(m_model);
    m_listView->setItemDelegate(m_delegate);

    // 添加到主布局
    m_mainLayout->addWidget(m_inputWidget);
    m_mainLayout->addWidget(m_listView, 1);

    // 设置窗口大小（初始值，后面会根据屏幕调整）
    resize(800, 500);
}

void SearchWindow::setupLayerSurface() {
    using namespace explorer::layer;

    LayerSurfaceOptions options;
    options.layer = Layer::Overlay;
    options.namespace_ = "explorer-search";
    options.description = "Explorer Linux Global Search";
    options.keyboardInteractivity = true;
    options.exclusiveZone = 0; // 不占用专用区域

    // 锚定在顶部中心
    options.anchor = QPoint(0.5, 0.0); // 水平中心，垂直顶部
    options.margin = QMargins(0, 50, 0, 0); // 距离顶部 50px

    m_layerSurface = LayerShellManager::instance().createSurface(windowHandle(), options);

    if (!m_layerSurface || !m_layerSurface->init()) {
        qWarning() << "Failed to initialize layer surface, falling back to regular window";
        // 回退到普通窗口模式
        setWindowFlags(windowFlags() | Qt::Window);
    }

    connect(m_layerSurface.get(), &LayerSurface::closed, this, &SearchWindow::onLayerSurfaceClosed);
    connect(m_layerSurface.get(), &LayerSurface::visibleChanged, this, [this](bool visible) {
        m_isVisible = visible;
        emit visibilityChanged(visible);
    });
}

void SearchWindow::setupIPC() {
    using namespace explorer::ipc;

    // 连接到会话总线
    m_messageBus = std::make_unique<MessageBus>(this);
    if (!m_messageBus->init()) {
        qWarning() << "Failed to initialize message bus";
    }

    // 连接到守护进程
    m_daemonInterface = DBusInterface("org.explorer.Daemon", "/org/explorer/Daemon",
                                      "org.explorer.Daemon", QDBusConnection::sessionBus());

    // 注册自己为搜索服务
    m_dbusService = std::make_unique<DBusService>("org.explorer.Search", this);
    if (!m_dbusService->registerObject("/org/explorer/Search", this)) {
        qWarning() << "Failed to register DBus object";
    }
    if (!m_dbusService->registerService()) {
        qWarning() << "Failed to register DBus service";
    }

    // 订阅守护进程信号
    if (m_messageBus) {
        m_messageBus->subscribe("org.explorer.Daemon.ShowSearch", [this](const QString&, const QVariant&) {
            showWindow();
        });
        m_messageBus->subscribe("org.explorer.Daemon.HideSearch", [this](const QString&, const QVariant&) {
            hideWindow();
        });
        m_messageBus->subscribe("org.explorer.Daemon.ToggleSearch", [this](const QString&, const QVariant&) {
            toggleWindow();
        });
    }

    // 监听核心信号
    explorer::core::SignalDispatcher::instance().connect("search.show", [this]() {
        showWindow();
    });
    explorer::core::SignalDispatcher::instance().connect("search.hide", [this]() {
        hideWindow();
    });
    explorer::core::SignalDispatcher::instance().connect("search.toggle", [this]() {
        toggleWindow();
    });
}

void SearchWindow::setupConnections() {
    // 搜索输入框
    connect(m_lineEdit, &QLineEdit::textChanged, this, &SearchWindow::onLineEditTextChanged);
    connect(m_lineEdit, &QLineEdit::returnPressed, this, &SearchWindow::onLineEditReturnPressed);

    // 清除按钮
    connect(m_clearButton, &QPushButton::clicked, this, &SearchWindow::onClearButtonClicked);

    // 结果列表
    connect(m_listView, &QListView::clicked, this, &SearchWindow::onResultClicked);
    connect(m_listView, &QListView::activated, this, &SearchWindow::onResultActivated);

    // 模型信号
    connect(m_model, &SearchModel::searchFinished, this, &SearchWindow::onModelSearchFinished);
    connect(m_model, &SearchModel::resultLaunched, this, &SearchWindow::resultLaunched);
}

void SearchWindow::loadSettings() {
    auto& config = explorer::core::Config::instance();

    // 恢复最后查询
    m_lastQuery = config.value(CONFIG_GROUP, KEY_LAST_QUERY, "").toString();

    // 恢复窗口几何信息
    QVariant geometry = config.value(CONFIG_GROUP, KEY_WINDOW_GEOMETRY);
    if (geometry.isValid() && geometry.canConvert<QByteArray>()) {
        restoreGeometry(geometry.toByteArray());
    }

    // 窗口宽度因子（相对于屏幕宽度）
    double widthFactor = config.value(CONFIG_GROUP, KEY_WINDOW_WIDTH_FACTOR, 0.5).toDouble();
    int height = config.value(CONFIG_GROUP, KEY_WINDOW_HEIGHT, 500).toInt();

    // 应用大小设置
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeom = screen->geometry();
        int width = qRound(screenGeom.width() * widthFactor);
        width = qBound(400, width, screenGeom.width() - 100);
        height = qBound(300, height, screenGeom.height() - 200);
        resize(width, height);
    }
}

void SearchWindow::saveSettings() {
    auto& config = explorer::core::Config::instance();

    // 保存最后查询
    config.setValue(CONFIG_GROUP, KEY_LAST_QUERY, m_lineEdit->text());

    // 保存窗口几何信息
    config.setValue(CONFIG_GROUP, KEY_WINDOW_GEOMETRY, saveGeometry());

    // 保存宽度因子和高度
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        double widthFactor = static_cast<double>(width()) / screen->geometry().width();
        config.setValue(CONFIG_GROUP, KEY_WINDOW_WIDTH_FACTOR, widthFactor);
        config.setValue(CONFIG_GROUP, KEY_WINDOW_HEIGHT, height());
    }

    config.sync();
}

void SearchWindow::applyTheme() {
    // 应用主题样式
    QString styleSheet = R"(
        #centralWidget {
            background-color: rgba(30, 30, 30, 230);
            border-radius: 12px;
            border: 1px solid rgba(80, 80, 80, 150);
        }
        #inputWidget {
            background-color: transparent;
        }
        #searchLineEdit {
            background-color: rgba(45, 45, 45, 220);
            border: 1px solid rgba(80, 80, 80, 180);
            border-radius: 6px;
            padding: 8px 12px;
            color: white;
            font-size: 14px;
            selection-background-color: rgba(0, 120, 215, 180);
        }
        #searchLineEdit:focus {
            border: 1px solid rgba(0, 120, 215, 200);
            background-color: rgba(50, 50, 50, 230);
        }
        #clearButton {
            background-color: transparent;
            color: #aaa;
            border: none;
            border-radius: 4px;
        }
        #clearButton:hover {
            background-color: rgba(255, 255, 255, 30);
            color: white;
        }
        #clearButton:pressed {
            background-color: rgba(255, 255, 255, 50);
        }
        #searchResultsView {
            background-color: transparent;
            border: none;
            outline: none;
        }
        #searchResultsView::item {
            border: none;
        }
        #searchResultsView::item:selected {
            background-color: transparent; /* 由委托处理 */
        }
        QScrollBar:vertical {
            background-color: transparent;
            width: 8px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background-color: rgba(100, 100, 100, 180);
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: rgba(130, 130, 130, 200);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )";

    setStyleSheet(styleSheet);
}

void SearchWindow::positionWindow() {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QRect screenGeom = screen->geometry();
    QSize windowSize = size();

    // 顶部居中，距离顶部 50px
    int x = screenGeom.x() + (screenGeom.width() - windowSize.width()) / 2;
    int y = screenGeom.y() + 50;

    move(x, y);

    // 如果使用 LayerSurface，还需要更新 layer surface 位置
    if (m_layerSurface) {
        m_layerSurface->setMargin(0, 50, 0, 0);
    }
}

void SearchWindow::focusSearchInput() {
    m_lineEdit->setFocus(Qt::ActiveWindowFocusReason);
    m_lineEdit->selectAll();
}

void SearchWindow::clearSearch() {
    m_lineEdit->clear();
    m_clearButton->setVisible(false);
    m_model->clearResults();
}

void SearchWindow::showWindow() {
    if (m_isVisible) return;

    applyTheme();
    positionWindow();

    if (m_layerSurface && m_layerSurface->isAvailable()) {
        m_layerSurface->show();
    } else {
        show();
        raise();
        activateWindow();
    }

    m_isVisible = true;
    focusSearchInput();

    // 恢复上次查询
    if (!m_lastQuery.isEmpty() && m_lineEdit->text().isEmpty()) {
        m_lineEdit->setText(m_lastQuery);
    }

    emit visibilityChanged(true);
    explorer::core::SignalDispatcher::instance().emit("search.shown");
}

void SearchWindow::hideWindow() {
    if (!m_isVisible) return;

    // 保存当前查询
    m_lastQuery = m_lineEdit->text();
    saveSettings();

    if (m_layerSurface) {
        m_layerSurface->hide();
    } else {
        hide();
    }

    m_isVisible = false;
    clearSearch();

    emit visibilityChanged(false);
    explorer::core::SignalDispatcher::instance().emit("search.hidden");
}

void SearchWindow::toggleWindow() {
    if (m_isVisible) {
        hideWindow();
    } else {
        showWindow();
    }
}

bool SearchWindow::isWindowVisible() const {
    return m_isVisible;
}

void SearchWindow::onLineEditTextChanged(const QString& text) {
    m_clearButton->setVisible(!text.isEmpty());
    m_model->setQuery(text);
}

void SearchWindow::onLineEditReturnPressed() {
    // 回车键启动第一个结果
    QModelIndex firstIndex = m_model->index(0, 0);
    if (firstIndex.isValid()) {
        onResultActivated(firstIndex);
    }
}

void SearchWindow::onClearButtonClicked() {
    clearSearch();
    m_lineEdit->setFocus();
}

void SearchWindow::onResultClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    m_listView->setCurrentIndex(index);
}

void SearchWindow::onResultActivated(const QModelIndex& index) {
    if (!index.isValid()) return;

    int row = index.row();
    if (m_model->launchResult(row)) {
        // 成功启动，隐藏窗口
        hideWindow();
    }
}

void SearchWindow::onModelSearchFinished(int count) {
    // 搜索完成，如果有结果且当前没有选中项，选中第一项
    if (count > 0 && !m_listView->currentIndex().isValid()) {
        m_listView->setCurrentIndex(m_model->index(0, 0));
    }
    qDebug() << "Search finished, results:" << count;
}

void SearchWindow::onDaemonSignal(const QString& signalName, const QVariant& args) {
    qDebug() << "Daemon signal:" << signalName << args;
}

void SearchWindow::onLayerSurfaceClosed() {
    hideWindow();
}

void SearchWindow::onEscapePressed() {
    if (m_isVisible) {
        hideWindow();
    }
}

void SearchWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Escape:
            onEscapePressed();
            break;

        case Qt::Key_Up:
            // 向上导航结果列表
            if (m_listView->hasFocus() || m_lineEdit->hasFocus()) {
                QModelIndex current = m_listView->currentIndex();
                int row = current.isValid() ? current.row() : 0;
                if (row > 0) {
                    m_listView->setCurrentIndex(m_model->index(row - 1, 0));
                    m_listView->scrollTo(m_model->index(row - 1, 0));
                }
            }
            break;

        case Qt::Key_Down:
            // 向下导航结果列表
            if (m_listView->hasFocus() || m_lineEdit->hasFocus()) {
                QModelIndex current = m_listView->currentIndex();
                int row = current.isValid() ? current.row() : -1;
                if (row < m_model->rowCount() - 1) {
                    m_listView->setCurrentIndex(m_model->index(row + 1, 0));
                    m_listView->scrollTo(m_model->index(row + 1, 0));
                }
            }
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            // 在列表中回车启动选中项
            if (m_listView->hasFocus()) {
                onResultActivated(m_listView->currentIndex());
            }
            break;

        default:
            QMainWindow::keyPressEvent(event);
            break;
    }
}

void SearchWindow::closeEvent(QCloseEvent* event) {
    // 不真正关闭，只是隐藏
    event->ignore();
    hideWindow();
}

void SearchWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (m_layerSurface) {
        m_layerSurface->show();
    }
}

void SearchWindow::hideEvent(QHideEvent* event) {
    QMainWindow::hideEvent(event);
}

void SearchWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (m_layerSurface) {
        m_layerSurface->setSize(size());
    }
}

} // namespace explorer::search

#include "SearchWindow.moc"