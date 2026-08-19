#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariant>
#include <optional>
#include <memory>

#include <explorer/layer/LayerShell.h>
#include <explorer/ui/Label.h>
#include <explorer/ui/Button.h>
#include <explorer/ui/BaseWidget.h>
#include <explorer/core/Config.h>
#include <explorer/core/Registry.h>
#include <explorer/utils/DateTimeUtils.h>
#include <explorer/ipc/DBusInterface.h>
#include <explorer/ipc/MessageBus.h>

namespace explorer::lockscreen {

// Forward declaration for the UI implementation
class LockScreenWindowPrivate;

/**
 * @brief Lock Screen Window - Full-screen overlay for session locking
 * 
 * This window uses LayerShell with Layer::Overlay to create a topmost
 * full-screen surface that captures all input. It provides password
 * entry functionality and communicates with the daemon via DBus.
 */
class LockScreenWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LockScreenWindow(QWidget* parent = nullptr);
    ~LockScreenWindow() override;

    // Initialize the lock screen (create layer surface, connect signals)
    bool initialize();

    // Show the lock screen
    void showLockScreen();

    // Hide the lock screen
    void hideLockScreen();

    // Check if lock screen is currently visible
    bool isLocked() const;

    // Set password from config (for testing/demo)
    void setPassword(const QString& password);

    // Get current time string for display
    QString currentTimeString() const;

    // Get current date string for display
    QString currentDateString() const;

signals:
    // Emitted when lock state changes
    void lockStateChanged(bool locked);

    // Emitted when unlock is requested (successful)
    void unlockRequested();

    // Emitted when authentication fails
    void authenticationFailed(const QString& reason);

    // Emitted for DBus communication
    void lockRequested();
    void unlockSignal();

public slots:
    // Slot for DBus lock request
    void onLockRequested();

    // Slot for DBus unlock request
    void onUnlockRequested();

    // Slot for password entry
    void onPasswordEntered();

    // Slot for unlock button click
    void onUnlockClicked();

    // Slot for updating time display
    void updateDateTime();

    // Slot for clearing error message after delay
    void clearErrorMessage();

protected:
    // Handle key press events (Enter to unlock, Escape to cancel)
    void keyPressEvent(QKeyEvent* event) override;

    // Handle close event (prevent closing via window manager)
    void closeEvent(QCloseEvent* event) override;

    // Handle focus events to maintain keyboard grab
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

    // Handle show/hide events
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    // Setup UI components
    void setupUI();

    // Setup layer shell surface
    void setupLayerShell();

    // Setup DBus communication
    void setupDBus();

    // Setup timers
    void setupTimers();

    // Validate password
    bool validatePassword(const QString& password) const;

    // Show error message temporarily
    void showError(const QString& message);

    // Apply styling
    void applyStyles();

    // Update time/date labels
    void updateTimeDateLabels();

    // Private implementation pointer
    std::unique_ptr<LockScreenWindowPrivate> d;
};

} // namespace explorer::lockscreen