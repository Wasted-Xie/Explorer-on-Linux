#pragma once

#include <QPushButton>
#include "BaseWidget.h"

namespace explorer::ui {

class Button : public QPushButton {
    Q_OBJECT
public:
    explicit Button(const QString& text = "", QWidget* parent = nullptr);
    explicit Button(const QIcon& icon, const QString& text = "", QWidget* parent = nullptr);
    ~Button() override;

    // 按钮样式
    enum class ButtonType {
        Normal,
        Primary,
        Secondary,
        Success,
        Danger,
        Warning,
        Info,
        Link
    };

    void setButtonType(ButtonType type);
    ButtonType buttonType() const;

    // 加载状态
    void setLoading(bool loading);
    bool isLoading() const;

protected:
    void applyTheme() override;

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::ui

#include "Button.moc"