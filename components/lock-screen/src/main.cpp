#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QTimer>
#include <QScreen>
#include <QDir>

#include "LockScreenWindow.h"
#include <explorer/core/Config.h>
#include <explorer/ipc/DBusInterface.h>
#include <explorer/ipc/MessageBus.h>
#include <explorer/layer/LayerShellManager.h>

using namespace explorer::lockscreen;

int main(int argc, char* argv[]) {
    // Enable high DPI scaling
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    QApplication app(argc, argv);
    app.setApplicationName("explorer-lock-screen");
    app.setApplicationDisplayName("Explorer Lock Screen");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Explorer Linux");
    app.setOrganizationDomain("explorer-linux.org");
    
    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Explorer Linux Lock Screen Component");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Options
    QCommandLineOption showOption({"s", "show"}, "Show lock screen immediately");
    parser.addOption(showOption);
    
    QCommandLineOption testOption({"t", "test"}, "Run in test mode (no daemon connection)");
    parser.addOption(testOption);
    
    QCommandLineOption passwordOption({"p", "password"}, "Set password for testing", "password");
    parser.addOption(passwordOption);
    
    QCommandLineOption daemonOption({"d", "daemon"}, "Connect to daemon and register");
    parser.addOption(daemonOption);
    
    parser.process(app);
    
    // Initialize config
    auto& config = explorer::core::Config::instance();
    
    // Initialize layer shell manager
    if (!explorer::layer::LayerShellManager::instance().init()) {
        qWarning() << "Failed to initialize LayerShell manager. "
                   << "Lock screen requires Wayland with layer-shell protocol.";
        qWarning() << "Running in fallback mode (may not capture input properly).";
    }
    
    // Create lock screen window
    LockScreenWindow lockScreen;
    
    // Set password if provided via command line
    if (parser.isSet(passwordOption)) {
        lockScreen.setPassword(parser.value(passwordOption));
    }
    
    // Initialize the lock screen
    if (!lockScreen.initialize()) {
        qCritical() << "Failed to initialize lock screen";
        return 1;
    }
    
    // Show immediately if requested
    if (parser.isSet(showOption)) {
        lockScreen.showLockScreen();
    }
    
    // Connect to daemon if requested
    if (parser.isSet(daemonOption) || !parser.isSet(testOption)) {
        // Try to register with daemon
        explorer::ipc::DBusInterface daemonInterface(
            "org.explorer.Daemon",
            "/org/explorer/Daemon",
            "org.explorer.Daemon",
            QDBusConnection::sessionBus()
        );
        
        if (daemonInterface.isValid()) {
            qInfo() << "Connected to daemon, registering lock screen...";
            
            QDBusReply<void> reply = daemonInterface.call("RegisterLockScreen");
            if (reply.isValid()) {
                qInfo() << "Successfully registered with daemon";
            } else {
                qWarning() << "Failed to register with daemon:" << reply.error().message();
            }
            
            // Listen for lock/unlock signals from daemon
            QObject::connect(daemonInterface.interface(), &QDBusInterface::signal,
                &lockScreen, [&lockScreen](const QString& signalName, const QDBusMessage&) {
                    if (signalName == "LockRequested") {
                        QMetaObject::invokeMethod(&lockScreen, "onLockRequested", Qt::QueuedConnection);
                    } else if (signalName == "UnlockRequested") {
                        QMetaObject::invokeMethod(&lockScreen, "onUnlockRequested", Qt::QueuedConnection);
                    }
                });
        } else {
            qWarning() << "Daemon not available on session bus";
        }
    }
    
    // Setup message bus for pub/sub
    explorer::ipc::MessageBus messageBus;
    if (messageBus.init(QDBusConnection::sessionBus())) {
        // Subscribe to lock/unlock messages
        messageBus.subscribe("org.explorer.LockScreen.Lock", 
            [&lockScreen](const QString&, const QVariant&) {
                QMetaObject::invokeMethod(&lockScreen, "onLockRequested", Qt::QueuedConnection);
            });
        
        messageBus.subscribe("org.explorer.LockScreen.Unlock", 
            [&lockScreen](const QString&, const QVariant&) {
                QMetaObject::invokeMethod(&lockScreen, "onUnlockRequested", Qt::QueuedConnection);
            });
        
        qInfo() << "Message bus initialized and subscribed to lock/unlock topics";
    }
    
    // Handle application activation (for testing)
    QObject::connect(&app, &QApplication::applicationStateChanged, [&lockScreen](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && !lockScreen.isLocked()) {
            // App became active, could trigger lock if needed
        }
    });
    
    // Print startup info
    qInfo() << "Explorer Lock Screen started";
    qInfo() << "Version:" << app.applicationVersion();
    qInfo() << "Locked:" << lockScreen.isLocked();
    qInfo() << "Layer shell available:" << explorer::layer::LayerShellManager::instance().isAvailable();
    
    if (parser.isSet(testOption)) {
        qInfo() << "Running in test mode";
        qInfo() << "Press Win+L or call LockScreen.Lock via DBus to activate";
        qInfo() << "Or use --show to display immediately";
    }
    
    // Run application
    int result = app.exec();
    
    // Cleanup
    explorer::layer::LayerShellManager::instance().shutdown();
    
    return result;
}