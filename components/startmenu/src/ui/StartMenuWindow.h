#pragma once

#include <QWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPoint>
#include <QSize>
#include "libs/libexplorer-layer/src/LayerShell.h"
#include "libs/libexplorer-ui/src/BaseWidget.h"
#include "ApplicationItem.h"

namespace explorer::startmenu {

class StartMenuWindow : public QWindow {
    Q_OBJECT
public:
    explicit StartMenuWindow(QWindow* parent = nullptr);
    ~StartMenuWindow() override;

    // 显示在指定位置
    void showAt(const QPoint& globalPos);
    void hide();

    // 设置应用列表
    void setApplications(const QList<QPair<QString, QString>>& apps); // name, exec

    // 是否可见
    bool isVisible() const;

signals:
    void applicationLaunched(const QString& exec);
    void closed();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUI();
    void createLayerSurface();
    void positionAt(const QPoint& globalPos);

    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::startmenu

#include "StartMenuWindow.moc"