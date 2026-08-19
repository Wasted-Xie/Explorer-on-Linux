#include "LockScreenWindow.h"
#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QDebug>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QDateTime>
#include <QTimer>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>

namespace explorer::lockscreen {

// Private implementation class
class LockScreenWindowPrivate {
public:
    explicit LockScreenWindowPrivate(LockScreenWindow* parent) : q(parent) {}

    LockScreenWindow* q = nullptr;
    
    // Layer shell surface
    std::unique_ptr<explorer::layer::LayerSurface> layerSurface;
    
    // UI components
    QWidget* centralWidget = nullptr;
    QVBoxLayout* mainLayout = nullptr;
    
    // Background overlay
    QWidget* backgroundWidget = nullptr;
    QGraphicsOpacityEffect* backgroundOpacity = nullptr;
    
    // Content container
    QFrame* contentFrame = nullptr;
    QVBoxLayout* contentLayout = nullptr;
    
    // Time/date labels
    explorer::ui::Label* timeLabel = nullptr;
    explorer::ui::Label* dateLabel = nullptr;
    
    // Message label
    explorer::ui::Label* messageLabel = nullptr;
    
    // Password input
    QLineEdit* passwordEdit = nullptr;
    
    // Error label
    explorer::ui::Label* errorLabel = nullptr;
    
    // Unlock button
    explorer::ui::Button* unlockButton = nullptr;
    
    // Timers
    QTimer* dateTimeTimer = nullptr;
    QTimer* errorClearTimer = nullptr;
    
    // State
    bool locked = false;
    QString password; // For MVP, simple password from config
    bool passwordSet = false;
    
    // DBus
    std::unique_ptr<explorer::ipc::DBusInterface> daemonInterface;
    std::unique_ptr<explorer::ipc::MessageBus> messageBus;
    
    // Animation
    QPropertyAnimation* fadeAnimation = nullptr;
};

LockScreenWindow::LockScreenWindow(QWidget* parent)
    : QMainWindow(parent)
    , d(std::make_unique<LockScreenWindowPrivate>(this))
{
    // Window setup
    setWindowTitle("Explorer Lock Screen");
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    // Setup UI
    setupUI();
    
    // Setup layer shell
    setupLayerShell();
    
    // Setup DBus
    setupDBus();
    
    // Setup timers
    setupTimers();
    
    // Load password from config
    auto& config = explorer::core::Config::instance();
    if (config.contains("lockscreen", "password")) {
        d->password = config.value("lockscreen", "password").toString();
        d->passwordSet = true;
    } else {
        // Default password for MVP (empty means any non-empty input works)
        d->password = "";
        d->passwordSet = false;
    }
    
    // Apply styles
    applyStyles();
    
    // Initial time update
    updateTimeDateLabels();
}

LockScreenWindow::~LockScreenWindow() {
    if (d->layerSurface) {
        d->layerSurface->hide();
        d->layerSurface->shutdown();
    }
    
    if (d->messageBus) {
        // MessageBus will be cleaned up automatically
    }
}

bool LockScreenWindow::initialize() {
    if (!d->layerSurface) {
        qWarning() << "Layer surface not created";
        return false;
    }
    
    if (!d->layerSurface->init()) {
        qWarning() << "Failed to initialize layer surface";
        return false;
    }
    
    // Connect to daemon if available
    if (d->daemonInterface && d->daemonInterface->isValid()) {
        qInfo() << "Connected to daemon via DBus";
        
        // Register ourselves with the daemon
        QDBusReply<void> reply = d->daemonInterface->call("RegisterLockScreen");
        if (!reply.isValid()) {
            qWarning() << "Failed to register with daemon:" << reply.error().message();
        }
    } else {
        qWarning() << "Daemon not available, running in standalone mode";
    }
    
    // Start message bus
    if (d->messageBus) {
        d->messageBus->init(QDBusConnection::sessionBus());
        
        // Subscribe to lock/unlock messages
        d->messageBus->subscribe("org.explorer.LockScreen.Lock", 
            [this](const QString&, const QVariant&) {
                QMetaObject::invokeMethod(this, "onLockRequested", Qt::QueuedConnection);
            });
        
        d->messageBus->subscribe("org.explorer.LockScreen.Unlock", 
            [this](const QString&, const QVariant&) {
                QMetaObject::invokeMethod(this, "onUnlockRequested", Qt::QueuedConnection);
            });
    }
    
    qInfo() << "Lock screen initialized successfully";
    return true;
}

void LockScreenWindow::showLockScreen() {
    if (d->locked) return;
    
    d->locked = true;
    
    // Show layer surface
    if (d->layerSurface) {
        d->layerSurface->show();
    }
    
    // Show the window
    showFullScreen();
    raise();
    activateWindow();
    
    // Focus password field
    if (d->passwordEdit) {
        d->passwordEdit->setFocus(Qt::ActiveWindowFocusReason);
        d->passwordEdit->clear();
    }
    
    // Clear error
    if (d->errorLabel) {
        d->errorLabel->setText("");
        d->errorLabel->hide();
    }
    
    // Update time/date
    updateTimeDateLabels();
    
    // Start fade-in animation
    if (d->fadeAnimation) {
        d->fadeAnimation->setDirection(QAbstractAnimation::Forward);
        d->fadeAnimation->start();
    }
    
    emit lockStateChanged(true);
    emit lockRequested();
    
    qInfo() << "Lock screen shown";
}

void LockScreenWindow::hideLockScreen() {
    if (!d->locked) return;
    
    d->locked = false;
    
    // Hide layer surface
    if (d->layerSurface) {
        d->layerSurface->hide();
    }
    
    // Hide the window
    hide();
    
    // Stop fade animation
    if (d->fadeAnimation) {
        d->fadeAnimation->stop();
    }
    
    emit lockStateChanged(false);
    emit unlockRequested();
    emit unlockSignal();
    
    qInfo() << "Lock screen hidden";
}

bool LockScreenWindow::isLocked() const {
    return d->locked;
}

void LockScreenWindow::setPassword(const QString& password) {
    d->password = password;
    d->passwordSet = !password.isEmpty();
    
    // Save to config
    auto& config = explorer::core::Config::instance();
    config.setValue("lockscreen", "password", password);
    config.sync();
}

QString LockScreenWindow::currentTimeString() const {
    return explorer::utils::DateTimeUtils::toString(
        explorer::utils::DateTimeUtils::currentTime(), "HH:mm");
}

QString LockScreenWindow::currentDateString() const {
    return explorer::utils::DateTimeUtils::toString(
        explorer::utils::DateTimeUtils::today(), "dddd, MMMM d, yyyy");
}

void LockScreenWindow::setupUI() {
    // Central widget
    d->centralWidget = new QWidget(this);
    setCentralWidget(d->centralWidget);
    
    d->mainLayout = new QVBoxLayout(d->centralWidget);
    d->mainLayout->setContentsMargins(0, 0, 0, 0);
    d->mainLayout->setSpacing(0);
    
    // Background widget (semi-transparent dark overlay)
    d->backgroundWidget = new QWidget(d->centralWidget);
    d->backgroundWidget->setStyleSheet("background-color: rgba(0, 0, 0, 0.8);");
    d->backgroundWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    d->backgroundOpacity = new QGraphicsOpacityEffect(d->backgroundWidget);
    d->backgroundOpacity->setOpacity(0.8);
    d->backgroundWidget->setGraphicsEffect(d->backgroundOpacity);
    
    d->mainLayout->addWidget(d->backgroundWidget);
    
    // Content frame (centered card)
    d->contentFrame = new QFrame(d->backgroundWidget);
    d->contentFrame->setObjectName("contentFrame");
    d->contentFrame->setFixedWidth(400);
    d->contentFrame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    
    d->contentLayout = new QVBoxLayout(d->contentFrame);
    d->contentLayout->setContentsMargins(40, 40, 40, 40);
    d->contentLayout->setSpacing(20);
    d->contentLayout->setAlignment(Qt::AlignCenter);
    
    // Time label
    d->timeLabel = new explorer::ui::Label(d->contentFrame);
    d->timeLabel->setLabelType(explorer::ui::Label::LabelType::Heading1);
    d->timeLabel->setAlignment(Qt::AlignCenter);
    d->timeLabel->setTextColor(Qt::white);
    d->timeLabel->setObjectName("timeLabel");
    d->contentLayout->addWidget(d->timeLabel);
    
    // Date label
    d->dateLabel = new explorer::ui::Label(d->contentFrame);
    d->dateLabel->setLabelType(explorer::ui::Label::LabelType::Body);
    d->dateLabel->setAlignment(Qt::AlignCenter);
    d->dateLabel->setTextColor(QColor(200, 200, 200));
    d->dateLabel->setObjectName("dateLabel");
    d->contentLayout->addWidget(d->dateLabel);
    
    // Spacer
    d->contentLayout->addSpacing(20);
    
    // Separator line
    QFrame* separator = new QFrame(d->contentFrame);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("color: rgba(255, 255, 255, 0.3);");
    separator->setFixedHeight(1);
    d->contentLayout->addWidget(separator);
    
    // Spacer
    d->contentLayout->addSpacing(20);
    
    // Message label
    d->messageLabel = new explorer::ui::Label("Locked - Enter password to unlock", d->contentFrame);
    d->messageLabel->setLabelType(explorer::ui::Label::LabelType::Body);
    d->messageLabel->setAlignment(Qt::AlignCenter);
    d->messageLabel->setTextColor(Qt::white);
    d->messageLabel->setWordWrap(true);
    d->messageLabel->setObjectName("messageLabel");
    d->contentLayout->addWidget(d->messageLabel);
    
    // Spacer
    d->contentLayout->addSpacing(20);
    
    // Password input
    d->passwordEdit = new QLineEdit(d->contentFrame);
    d->passwordEdit->setEchoMode(QLineEdit::Password);
    d->passwordEdit->setPlaceholderText("Password");
    d->passwordEdit->setObjectName("passwordEdit");
    d->passwordEdit->setFixedHeight(44);
    d->passwordEdit->setStyleSheet(R"(
        QLineEdit#passwordEdit {
            background-color: rgba(255, 255, 255, 0.1);
            border: 1px solid rgba(255, 255, 255, 0.3);
            border-radius: 6px;
            padding: 0 16px;
            color: white;
            font-size: 16px;
            selection-background-color: rgba(255, 255, 255, 0.3);
        }
        QLineEdit#passwordEdit:focus {
            border: 1px solid rgba(255, 255, 255, 0.6);
            background-color: rgba(255, 255, 255, 0.15);
        }
        QLineEdit#passwordEdit::placeholder {
            color: rgba(255, 255, 255, 0.5);
        }
    )");
    d->contentLayout->addWidget(d->passwordEdit);
    
    // Connect return pressed
    connect(d->passwordEdit, &QLineEdit::returnPressed, this, &LockScreenWindow::onPasswordEntered);
    
    // Error label (initially hidden)
    d->errorLabel = new explorer::ui::Label("", d->contentFrame);
    d->errorLabel->setLabelType(explorer::ui::Label::LabelType::Caption);
    d->errorLabel->setAlignment(Qt::AlignCenter);
    d->errorLabel->setTextColor(QColor(255, 100, 100));
    d->errorLabel->setObjectName("errorLabel");
    d->errorLabel->hide();
    d->contentLayout->addWidget(d->errorLabel);
    
    // Spacer
    d->contentLayout->addSpacing(16);
    
    // Unlock button
    d->unlockButton = new explorer::ui::Button("Unlock", d->contentFrame);
    d->unlockButton->setButtonType(explorer::ui::Button::ButtonType::Primary);
    d->unlockButton->setFixedHeight(44);
    d->unlockButton->setCursor(Qt::PointingHandCursor);
    d->contentLayout->addWidget(d->unlockButton);
    
    connect(d->unlockButton, &QPushButton::clicked, this, &LockScreenWindow::onUnlockClicked);
    
    // Center the content frame in the background
    QHBoxLayout* centerLayout = new QHBoxLayout(d->backgroundWidget);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->addStretch();
    centerLayout->addWidget(d->contentFrame, 0, Qt::AlignCenter);
    centerLayout->addStretch();
    
    // Fade animation for background
    d->fadeAnimation = new QPropertyAnimation(d->backgroundOpacity, "opacity", this);
    d->fadeAnimation->setDuration(300);
    d->fadeAnimation->setStartValue(0.0);
    d->fadeAnimation->setEndValue(0.8);
    d->fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void LockScreenWindow::setupLayerShell() {
    using namespace explorer::layer;
    
    // Create layer surface options for overlay (lock screen)
    LayerSurfaceOptions options;
    options.layer = Layer::Overlay;
    options.namespace_ = "explorer-lock-screen";
    options.description = "Explorer Linux Lock Screen";
    options.keyboardInteractivity = true; // Critical: capture all keyboard input
    options.exclusiveZone = 0; // Full screen, no exclusive zone needed for overlay
    options.anchor = QPoint(0.5, 0.5); // Center anchor
    
    // Get screen geometry for full-screen sizing
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        options.size = screen->geometry().size();
    }
    
    // Create layer surface
    d->layerSurface = explorer::layer::LayerShellManager::instance().createSurface(windowHandle(), options);
    
    if (d->layerSurface) {
        connect(d->layerSurface.get(), &LayerSurface::closed, this, [this]() {
            qInfo() << "Layer surface closed";
            hideLockScreen();
        });
        
        connect(d->layerSurface.get(), &LayerSurface::visibleChanged, this, [this](bool visible) {
            qInfo() << "Layer surface visibility changed:" << visible;
        });
        
        qInfo() << "Layer surface created for lock screen";
    } else {
        qWarning() << "Failed to create layer surface";
    }
}

void LockScreenWindow::setupDBus() {
    // Try to connect to the daemon
    d->daemonInterface = std::make_unique<explorer::ipc::DBusInterface>(
        "org.explorer.Daemon",
        "/org/explorer/Daemon",
        "org.explorer.Daemon",
        QDBusConnection::sessionBus()
    );
    
    if (d->daemonInterface->isValid()) {
        // Connect to daemon signals
        connect(d->daemonInterface->interface(), &QDBusInterface::signal,
            this, [this](const QString& signalName, const QDBusMessage& message) {
                if (signalName == "LockRequested") {
                    QMetaObject::invokeMethod(this, "onLockRequested", Qt::QueuedConnection);
                } else if (signalName == "UnlockRequested") {
                    QMetaObject::invokeMethod(this, "onUnlockRequested", Qt::QueuedConnection);
                }
            });
    }
    
    // Create message bus for pub/sub
    d->messageBus = std::make_unique<explorer::ipc::MessageBus>(this);
}

void LockScreenWindow::setupTimers() {
    // Date/time update timer (every second)
    d->dateTimeTimer = new QTimer(this);
    d->dateTimeTimer->setInterval(1000);
    connect(d->dateTimeTimer, &QTimer::timeout, this, &LockScreenWindow::updateDateTime);
    d->dateTimeTimer->start();
    
    // Error clear timer (single shot, 3 seconds)
    d->errorClearTimer = new QTimer(this);
    d->errorClearTimer->setSingleShot(true);
    d->errorClearTimer->setInterval(3000);
    connect(d->errorClearTimer, &QTimer::timeout, this, &LockScreenWindow::clearErrorMessage);
}

bool LockScreenWindow::validatePassword(const QString& password) const {
    // For MVP: if no password is set, accept any non-empty input
    if (!d->passwordSet) {
        return !password.isEmpty();
    }
    
    // Simple comparison (in real implementation, use proper hashing)
    return password == d->password;
}

void LockScreenWindow::showError(const QString& message) {
    if (d->errorLabel) {
        d->errorLabel->setText(message);
        d->errorLabel->show();
        
        // Start timer to clear error
        if (d->errorClearTimer) {
            d->errorClearTimer->start();
        }
        
        // Shake animation for password field
        QPropertyAnimation* shakeAnim = new QPropertyAnimation(d->passwordEdit, "pos", this);
        shakeAnim->setDuration(400);
        shakeAnim->setEasingCurve(QEasingCurve::OutElastic);
        
        QPoint originalPos = d->passwordEdit->pos();
        shakeAnim->setKeyValueAt(0.0, originalPos);
        shakeAnim->setKeyValueAt(0.1, originalPos + QPoint(-10, 0));
        shakeAnim->setKeyValueAt(0.2, originalPos + QPoint(10, 0));
        shakeAnim->setKeyValueAt(0.3, originalPos + QPoint(-8, 0));
        shakeAnim->setKeyValueAt(0.4, originalPos + QPoint(8, 0));
        shakeAnim->setKeyValueAt(0.5, originalPos + QPoint(-5, 0));
        shakeAnim->setKeyValueAt(0.6, originalPos + QPoint(5, 0));
        shakeAnim->setKeyValueAt(0.7, originalPos + QPoint(-3, 0));
        shakeAnim->setKeyValueAt(0.8, originalPos + QPoint(3, 0));
        shakeAnim->setKeyValueAt(1.0, originalPos);
        
        shakeAnim->start(QAbstractAnimation::DeleteWhenStopped);
        
        // Clear password field
        d->passwordEdit->clear();
        d->passwordEdit->setFocus();
    }
}

void LockScreenWindow::applyStyles() {
    // Apply theme-aware styles
    setStyleSheet(R"(
        QMainWindow {
            background: transparent;
        }
        
        QFrame#contentFrame {
            background-color: rgba(30, 30, 40, 0.95);
            border-radius: 16px;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        QLabel#timeLabel {
            font-size: 72px;
            font-weight: 300;
            letter-spacing: -2px;
        }
        
        QLabel#dateLabel {
            font-size: 16px;
            font-weight: 400;
        }
        
        QLabel#messageLabel {
            font-size: 16px;
            font-weight: 400;
        }
        
        QLabel#errorLabel {
            font-size: 13px;
            font-weight: 500;
        }
    )");
}

void LockScreenWindow::updateTimeDateLabels() {
    if (d->timeLabel) {
        d->timeLabel->setText(currentTimeString());
    }
    if (d->dateLabel) {
        d->dateLabel->setText(currentDateString());
    }
}

void LockScreenWindow::keyPressEvent(QKeyEvent* event) {
    // Handle Escape to clear password field (but not unlock)
    if (event->key() == Qt::Key_Escape) {
        if (d->passwordEdit) {
            d->passwordEdit->clear();
            d->passwordEdit->setFocus();
        }
        event->accept();
        return;
    }
    
    // Let other keys through to password field
    QMainWindow::keyPressEvent(event);
}

void LockScreenWindow::closeEvent(QCloseEvent* event) {
    // Prevent closing via window manager when locked
    if (d->locked) {
        event->ignore();
        return;
    }
    
    QMainWindow::closeEvent(event);
}

void LockScreenWindow::focusInEvent(QFocusEvent* event) {
    // Ensure password field has focus when window gets focus
    if (d->locked && d->passwordEdit) {
        d->passwordEdit->setFocus(Qt::ActiveWindowFocusReason);
    }
    QMainWindow::focusInEvent(event);
}

void LockScreenWindow::focusOutEvent(QFocusEvent* event) {
    // Try to maintain focus when locked
    if (d->locked) {
        // Re-activate window to regain focus
        QTimer::singleShot(100, this, [this]() {
            if (d->locked) {
                activateWindow();
                raise();
                if (d->passwordEdit) {
                    d->passwordEdit->setFocus(Qt::ActiveWindowFocusReason);
                }
            }
        });
    }
    QMainWindow::focusOutEvent(event);
}

void LockScreenWindow::showEvent(QShowEvent* event) {
    if (d->locked && d->passwordEdit) {
        d->passwordEdit->setFocus(Qt::ActiveWindowFocusReason);
    }
    QMainWindow::showEvent(event);
}

void LockScreenWindow::hideEvent(QHideEvent* event) {
    QMainWindow::hideEvent(event);
}

void LockScreenWindow::onLockRequested() {
    qInfo() << "Lock requested via DBus/signal";
    showLockScreen();
}

void LockScreenWindow::onUnlockRequested() {
    qInfo() << "Unlock requested via DBus/signal";
    hideLockScreen();
}

void LockScreenWindow::onPasswordEntered() {
    if (!d->passwordEdit) return;
    
    QString password = d->passwordEdit->text();
    
    if (validatePassword(password)) {
        qInfo() << "Password correct, unlocking";
        hideLockScreen();
    } else {
        qWarning() << "Password incorrect";
        showError("Incorrect password. Please try again.");
        emit authenticationFailed("Incorrect password");
    }
}

void LockScreenWindow::onUnlockClicked() {
    onPasswordEntered();
}

void LockScreenWindow::updateDateTime() {
    updateTimeDateLabels();
}

void LockScreenWindow::clearErrorMessage() {
    if (d->errorLabel) {
        d->errorLabel->setText("");
        d->errorLabel->hide();
    }
}

} // namespace explorer::lockscreen

#include "LockScreenWindow.moc"