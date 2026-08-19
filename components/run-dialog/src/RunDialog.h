#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QProcess>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QTimer>

#include <explorer/core/Config.h>
#include <explorer/core/Settings.h>
#include <explorer/ipc/DBusService.h>
#include <explorer/ipc/DBusInterface.h>
#include <explorer/ipc/MessageBus.h>
#include <explorer/layer/LayerShell.h>
#include <explorer/ui/Button.h>
#include <explorer/ui/Label.h>
#include <explorer/utils/ProcessUtils.h>

namespace explorer::components {

class RunDialog : public QDialog {
    Q_OBJECT

public:
    explicit RunDialog(QWidget* parent = nullptr);
    ~RunDialog() override;

    // 显示对话框（带动画）
    void showDialog();
    void hideDialog();

    // 是否可见
    bool isDialogVisible() const;

public slots:
    // IPC/DBus 调用接口
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void toggle();

signals:
    void visibilityChanged(bool visible);
    void commandExecuted(const QString& command);
    void errorOccurred(const QString& message);

private slots:
    void onOkClicked();
    void onCancelClicked();
    void onLineEditReturnPressed();
    void onLineEditTextChanged(const QString& text);
    void onDaemonSignal(const QString& signalName, const QVariant& args);

private:
    void setupUI();
    void setupLayerSurface();
    void setupIPC();
    void loadSettings();
    void saveSettings();
    void executeCommand(const QString& command);
    void centerOnScreen();
    void applyTheme();

    // 事件处理
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    // UI 组件
    QVBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_inputLayout = nullptr;
    QHBoxLayout* m_buttonLayout = nullptr;

    explorer::ui::Label* m_label = nullptr;
    QLineEdit* m_lineEdit = nullptr;
    explorer::ui::Button* m_okButton = nullptr;
    explorer::ui::Button* m_cancelButton = nullptr;

    // Layer Surface
    std::unique_ptr<explorer::layer::LayerSurface> m_layerSurface;

    // IPC
    std::unique_ptr<explorer::ipc::DBusService> m_dbusService;
    std::unique_ptr<explorer::ipc::MessageBus> m_messageBus;
    explorer::ipc::DBusInterface m_daemonInterface;

    // 状态
    bool m_initialized = false;
    QString m_lastCommand;

    // 配置键常量
    static constexpr const char* CONFIG_GROUP = "RunDialog";
    static constexpr const char* KEY_LAST_COMMAND = "lastCommand";
    static constexpr const char* KEY_WINDOW_GEOMETRY = "windowGeometry";
};

} // namespace explorer::components

#include "RunDialog.moc"