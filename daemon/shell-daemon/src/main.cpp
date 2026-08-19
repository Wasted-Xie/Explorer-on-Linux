#include <QCoreApplication>
#include <QDebug>
#include "Daemon.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    explorer::daemon::Daemon daemon;
    if (!daemon.init(argc, argv)) {
        qCritical() << "Failed to initialize daemon";
        return 1;
    }

    qInfo() << "Explorer Linux Daemon started";
    return daemon.run();
}

#include "main.moc"