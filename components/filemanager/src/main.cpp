#include <QApplication>
#include <QCommandLineParser>
#include "FileManagerWindow.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("File Manager");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Explorer Linux");
    QCoreApplication::setOrganizationDomain("explorer-linux.org");

    QCommandLineParser parser;
    parser.setApplicationDescription("Explorer Linux File Manager");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption startPathOption(QStringList() << "p" << "path",
                                       "Start at specified path",
                                       "path");
    parser.addOption(startPathOption);

    parser.process(app);

    QString startPath;
    if (parser.isSet(startPathOption)) {
        startPath = parser.value(startPathOption);
    } else {
        startPath = QStandardPaths::homeLocation();
    }

    FileManagerWindow window;
    if (!startPath.isEmpty()) {
        // 如果指定了起始路径，我们需要修改窗口以使用该路径
        // 为了简化，这里我们只创建一个带有指定路径的选项卡
        // 实际实现中可能需要修改addTab方法或创建一个特殊的构造函数
        window.addTab(startPath);
        // 移除默认的主目录选项卡
        if (window.tabWidget()->count() > 1) {
            window.tabWidget()->removeTab(0);
        }
    }

    window.show();
    return app.exec();
}

#include "main.moc"