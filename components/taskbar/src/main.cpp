#include <QApplication>
#include <QMessageBox>
#include "TaskbarWindow.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Taskbar");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Explorer Linux");
    QCoreApplication::setOrganizationDomain("explorer-linux.org");

    TaskbarWindow w;
    if (!w.initialize()) {
        QMessageBox::critical(nullptr, "任务栏初始化失败", "无法初始化任务栏。");
        return -1;
    }
    w.show();
    return app.exec();
}

#include "main.moc"