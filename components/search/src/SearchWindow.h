#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QListView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QSettings>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QScreen>
#include <QApplication>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

#include <explorer/core/Config.h>
#include <explorer/core/SignalDispatcher.h>
#include <explorer/ipc/DBusService.h>
#include <explorer/ipc/MessageBus.h>
#include <explorer/layer/LayerShell.h>
#include <explorer/ui/Button.h>
#include <explorer/ui/Label.h>
// LineEdit and ListView not yet implemented in libexplorer-ui, use Qt equivalents

#include "SearchModel.h"
#include "SearchDelegate.h"

namespace explorer::search {

// 搜索窗口 - 主界面
class SearchWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit SearchWindow(QWidget* parent = nullptr);
    ~SearchWindow() override;

    // 显示/隐藏窗口
    void showWindow();
    void hideWindow();
    void toggleWindow();

    // IPC/DBus 调用接口
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void toggle();

    // 是否可见
    bool isWindowVisible() const;

signals:
    void visibilityChanged(bool visible);
    void searchPerformed(const QString& query);
    void resultLaunched(const QString& execPath, bool success);
    void errorOccurred(const QString& message);

private slots:
    void onLineEditTextChanged(const QString& text);
    void onLineEditReturnPressed();
    void onClearButtonClicked();
    void onResultClicked(const QModelIndex& index);
    void onResultActivated(const QModelIndex& index);
    void onModelSearchFinished(int count);
    void onDaemonSignal(const QString& signalName, const QVariant& args);
    void onLayerSurfaceClosed();

    // 键盘快捷键
    void onEscapePressed();

private:
    void setupUI();
    void setupLayerSurface();
    void setupIPC();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void applyTheme();
    void positionWindow();
    void focusSearchInput();
    void clearSearch();

    // 事件处理
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    // UI 组件
    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;

    // 搜索输入区域
    QWidget* m_inputWidget = nullptr;
    QHBoxLayout* m_inputLayout = nullptr;
    QLineEdit* m_lineEdit = nullptr;
    explorer::ui::Button* m_clearButton = nullptr;
    explorer::ui::Label* m_searchIconLabel = nullptr;

    // 结果列表
    QListView* m_listView = nullptr;
    SearchModel* m_model = nullptr;
    SearchDelegate* m_delegate = nullptr;

    // Layer Surface
    std::unique_ptr<explorer::layer::LayerSurface> m_layerSurface;

    // IPC
    std::unique_ptr<explorer::ipc::DBusService> m_dbusService;
    std::unique_ptr<explorer::ipc::MessageBus> m_messageBus;
    explorer::ipc::DBusInterface m_daemonInterface;

    // 状态
    bool m_initialized = false;
    bool m_isVisible = false;
    QString m_lastQuery;

    // 配置键常量
    static constexpr const char* CONFIG_GROUP = "Search";
    static constexpr const char* KEY_LAST_QUERY = "lastQuery";
    static constexpr const char* KEY_WINDOW_GEOMETRY = "windowGeometry";
    static constexpr const char* KEY_WINDOW_WIDTH_FACTOR = "windowWidthFactor";
    static constexpr const char* KEY_WINDOW_HEIGHT = "windowHeight";
};

} // namespace explorer::search