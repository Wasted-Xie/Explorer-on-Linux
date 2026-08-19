#include "RunDialog.h"

#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>

namespace explorer::components {

RunDialog::RunDialog(QWidget* parent)
    : QDialog(parent)
    , m_daemonInterface("org.explorer.Daemon", "/org/explorer/Daemon", "org.explorer.Daemon",
                        QDBusConnection::sessionBus())
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setModal(false);

    setupUI();
    setupLayerSurface();
    setupIPC();
    loadSettings();

    m_initialized = true;
    qDebug() << "RunDialog initialized";
}

RunDialog::~RunDialog() {
    saveSettings();
    if (m_layerSurface) {
        m_layerSurface->shutdown();
    }
    if (m_dbusService) {
        m_dbusService->stop();
    }
    if (m_messageBus) {
        // MessageBus will be cleaned up automatically
    }
    qDebug() << "RunDialog destroyed";
}

void RunDialog::setupUI() {
    // 主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(16, 16, 16, 16);
    m_mainLayout->setSpacing(12);

    // 创建一个容器widget用于圆角和背景
    QWidget* container = new QWidget(this);
    container->setObjectName("runDialogContainer");
    container->setStyleSheet(R"(
        #runDialogContainer {
            background-color: rgba(30, 30, 30, 240);
            border-radius: 12px;
            border: 1px solid rgba(255, 255, 255, 30);
        }
    )");

    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(20, 20, 20, 20);
    containerLayout->setSpacing(16);

    // 标签
    m_label = new explorer::ui::Label("Run:", container);
    m_label->setLabelType(explorer::ui::Label::LabelType::Body);
    m_label->setStyleSheet("color: #e0e0e0; font-weight: 500; font-size: 14px;");
    containerLayout->addWidget(m_label);

    // 输入框布局
    m_inputLayout = new QHBoxLayout();
    m_inputLayout->setSpacing(8);

    m_lineEdit = new QLineEdit(container);
    m_lineEdit->setPlaceholderText("Type a command...");
    m_lineEdit->setMinimumWidth(400);
    m_lineEdit->setMinimumHeight(36);
    m_lineEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: rgba(20, 20, 20, 200);
            border: 1px solid rgba(255, 255, 255, 40);
            border-radius: 6px;
            padding: 0 12px;
            color: #ffffff;
            font-size: 14px;
            selection-background-color: #0078d7;
        }
        QLineEdit:focus {
            border: 1px solid #0078d7;
            background-color: rgba(25, 25, 25, 220);
        }
        QLineEdit::placeholder {
            color: #888888;
        }
    )");
    m_inputLayout->addWidget(m_lineEdit);

    containerLayout->addLayout(m_inputLayout);

    // 按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();

    m_cancelButton = new explorer::ui::Button("Cancel", container);
    m_cancelButton->setButtonType(explorer::ui::Button::ButtonType::Secondary);
    m_cancelButton->setMinimumWidth(80);
    m_cancelButton->setMinimumHeight(32);
    m_buttonLayout->addWidget(m_cancelButton);

    m_okButton = new explorer::ui::Button("OK", container);
    m_okButton->setButtonType(explorer::ui::Button::ButtonType::Primary);
    m_okButton->setMinimumWidth(80);
    m_okButton->setMinimumHeight(32);
    m_okButton->setDefault(true);
    m_buttonLayout->addWidget(m_okButton);

    containerLayout->addLayout(m_buttonLayout);

    m_mainLayout->addWidget(container);

    // 设置固定大小
    setFixedSize(500, 180);

    // 连接信号
    connect(m_okButton, &QPushButton::clicked, this, &RunDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &RunDialog::onCancelClicked);
    connect(m_lineEdit, &QLineEdit::returnPressed, this, &RunDialog::onLineEditReturnPressed);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &RunDialog::onLineEditTextChanged);
}

void RunDialog::setupLayerSurface() {
    // 确保窗口句柄已创建
    if (!windowHandle()) {
        createWinId();
    }

    explorer::layer::LayerSurfaceOptions options;
    options.layer = explorer::layer::Layer::Top;
    options.namespace_ = "run-dialog";
    options.description = "Explorer Linux Run Dialog";
    options.size = QSize(500, 180);
    options.anchor = QPoint(0.5, 0.3); // 居中偏上
    options.margin = QMargins(0, 0, 0, 0);
    options.exclusiveZone = -1;
    options.keyboardInteractivity = true;

    m_layerSurface = explorer::layer::LayerShellManager::instance().createSurface(windowHandle(), options);

    if (m_layerSurface) {
        connect(m_layerSurface.get(), &explorer::layer::LayerSurface::visibleChanged,
                this, [this](bool visible) {
                    if (!visible && isVisible()) {
                        hide();
                    }
                });
        qDebug() << "LayerSurface created for RunDialog";
    } else {
        qWarning() << "Failed to create LayerSurface for RunDialog, falling back to regular window";
    }
}

void RunDialog::setupIPC() {
    // 创建 DBus 服务
    m_dbusService = std::make_unique<explorer::ipc::DBusService>("org.explorer.RunDialog", this);

    // 将自己导出为 DBus 对象
    if (m_dbusService->start()) {
        m_dbusService->registerObject("/org/explorer/RunDialog", this);
        qDebug() << "RunDialog DBus service registered";
    } else {
        qWarning() << "Failed to start RunDialog DBus service";
    }

    // 创建消息总线用于接收守护进程信号
    m_messageBus = std::make_unique<explorer::ipc::MessageBus>(this);
    if (m_messageBus->init()) {
        // 订阅守护进程的显示/隐藏信号
        m_messageBus->subscribe("org.explorer.Daemon.RunDialog.Show",
            [this](const QString&, const QVariant&) {
                showDialog();
            });
        m_messageBus->subscribe("org.explorer.Daemon.RunDialog.Hide",
            [this](const QString&, const QVariant&) {
                hideDialog();
            });
        m_messageBus->subscribe("org.explorer.Daemon.RunDialog.Toggle",
            [this](const QString&, const QVariant&) {
                if (isDialogVisible()) {
                    hideDialog();
                } else {
                    showDialog();
                }
            });
        qDebug() << "RunDialog MessageBus initialized and subscribed to daemon signals";
    } else {
        qWarning() << "Failed to initialize RunDialog MessageBus";
    }

    // 也可以直接监听守护进程的 DBus 信号
    if (m_daemonInterface.isValid()) {
        m_daemonInterface.connectSignal("ShowRunDialog", [this]() {
            showDialog();
        });
        m_daemonInterface.connectSignal("HideRunDialog", [this]() {
            hideDialog();
        });
        qDebug() << "Connected to daemon DBus signals";
    }
}

void RunDialog::loadSettings() {
    explorer::core::Settings settings(CONFIG_GROUP);
    m_lastCommand = settings.getString(KEY_LAST_COMMAND, "");

    if (!m_lastCommand.isEmpty()) {
        m_lineEdit->setText(m_lastCommand);
        m_lineEdit->selectAll();
    }

    // 恢复窗口位置（如果需要）
    // QRect geometry = settings.getValue(KEY_WINDOW_GEOMETRY).toRect();
    // if (geometry.isValid()) {
    //     restoreGeometry(geometry);
    // }
}

void RunDialog::saveSettings() {
    explorer::core::Settings settings(CONFIG_GROUP);
    settings.setString(KEY_LAST_COMMAND, m_lineEdit->text().trimmed());
    // settings.setValue(KEY_WINDOW_GEOMETRY, saveGeometry());
    settings.sync();
}

void RunDialog::executeCommand(const QString& command) {
    if (command.trimmed().isEmpty()) {
        return;
    }

    QString cmd = command.trimmed();
    m_lastCommand = cmd;
    saveSettings();

    qDebug() << "Executing command:" << cmd;

    // 尝试解析命令
    QString program;
    QStringList args;

    // 简单解析：第一个词是程序，其余是参数
    // 处理带引号的参数
    QStringList parts;
    QString current;
    bool inQuotes = false;
    QChar quoteChar;

    for (int i = 0; i < cmd.size(); ++i) {
        QChar c = cmd[i];
        if ((c == '"' || c == '\'') && !inQuotes) {
            inQuotes = true;
            quoteChar = c;
        } else if (c == quoteChar && inQuotes) {
            inQuotes = false;
        } else if (c == ' ' && !inQuotes) {
            if (!current.isEmpty()) {
                parts.append(current);
                current.clear();
            }
        } else {
            current.append(c);
        }
    }
    if (!current.isEmpty()) {
        parts.append(current);
    }

    if (parts.isEmpty()) {
        return;
    }

    program = parts[0];
    if (parts.size() > 1) {
        args = parts.mid(1);
    }

    // 查找可执行文件
    QString executable = explorer::utils::ProcessUtils::findExecutable(program);

    if (executable.isEmpty()) {
        // 尝试直接执行（可能是内置命令或完整路径）
        executable = program;
    }

    // 使用 QProcess::startDetached 在后台运行
    bool success = false;
    if (!args.isEmpty()) {
        success = QProcess::startDetached(executable, args);
    } else {
        success = QProcess::startDetached(executable);
    }

    if (success) {
        qDebug() << "Command started successfully:" << executable;
        emit commandExecuted(cmd);
        hideDialog();
    } else {
        QString errorMsg = QString("Failed to execute: %1").arg(cmd);
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);

        // 显示错误提示（简单的方式）
        m_lineEdit->setStyleSheet(m_lineEdit->styleSheet() + "QLineEdit { border: 1px solid #e81123; }");
        QTimer::singleShot(2000, this, [this]() {
            m_lineEdit->setStyleSheet(R"(
                QLineEdit {
                    background-color: rgba(20, 20, 20, 200);
                    border: 1px solid rgba(255, 255, 255, 40);
                    border-radius: 6px;
                    padding: 0 12px;
                    color: #ffffff;
                    font-size: 14px;
                    selection-background-color: #0078d7;
                }
                QLineEdit:focus {
                    border: 1px solid #0078d7;
                    background-color: rgba(25, 25, 25, 220);
                }
                QLineEdit::placeholder {
                    color: #888888;
                }
            )");
        });
    }
}

void RunDialog::centerOnScreen() {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int x = screenGeometry.x() + (screenGeometry.width() - width()) / 2;
        int y = screenGeometry.y() + (screenGeometry.height() - height()) / 3; // 偏上显示
        move(x, y);
    }
}

void RunDialog::applyTheme() {
    // 主题应用由 BaseWidget 自动处理
    // 这里可以添加额外的主题相关逻辑
}

void RunDialog::showDialog() {
    if (isDialogVisible()) {
        return;
    }

    qDebug() << "Showing RunDialog";

    centerOnScreen();

    // 先显示 layer surface，再显示窗口
    if (m_layerSurface) {
        m_layerSurface->show();
    }

    QDialog::show();
    raise();
    activateWindow();

    // 聚焦到输入框
    QTimer::singleShot(50, this, [this]() {
        m_lineEdit->setFocus(Qt::ActiveWindowFocusReason);
        if (!m_lastCommand.isEmpty() && m_lineEdit->text() == m_lastCommand) {
            m_lineEdit->selectAll();
        }
    });

    emit visibilityChanged(true);
}

void RunDialog::hideDialog() {
    if (!isDialogVisible()) {
        return;
    }

    qDebug() << "Hiding RunDialog";

    if (m_layerSurface) {
        m_layerSurface->hide();
    }

    QDialog::hide();

    emit visibilityChanged(false);
}

bool RunDialog::isDialogVisible() const {
    return isVisible() && (m_layerSurface ? m_layerSurface->isVisible() : true);
}

void RunDialog::onOkClicked() {
    executeCommand(m_lineEdit->text());
}

void RunDialog::onCancelClicked() {
    hideDialog();
}

void RunDialog::onLineEditReturnPressed() {
    executeCommand(m_lineEdit->text());
}

void RunDialog::onLineEditTextChanged(const QString& text) {
    // 可以在这里添加自动补全逻辑
    Q_UNUSED(text);
}

void RunDialog::onDaemonSignal(const QString& signalName, const QVariant& args) {
    Q_UNUSED(args);
    if (signalName == "ShowRunDialog") {
        showDialog();
    } else if (signalName == "HideRunDialog") {
        hideDialog();
    }
}

void RunDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hideDialog();
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}

void RunDialog::closeEvent(QCloseEvent* event) {
    // 不要真正关闭，只是隐藏
    event->ignore();
    hideDialog();
}

void RunDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (m_layerSurface && !m_layerSurface->isVisible()) {
        m_layerSurface->show();
    }
}

void RunDialog::hideEvent(QHideEvent* event) {
    QDialog::hideEvent(event);
    if (m_layerSurface && m_layerSurface->isVisible()) {
        m_layerSurface->hide();
    }
}

} // namespace explorer::components