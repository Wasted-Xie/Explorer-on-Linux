#pragma once

#include <QLabel>
#include "BaseWidget.h"

namespace explorer::ui {

class Label : public QLabel {
    Q_OBJECT
public:
    explicit Label(const QString& text = "", QWidget* parent = nullptr);
    explicit Label(QWidget* parent = nullptr);
    ~Label() override;

    // 标签样式
    enum class LabelType {
        Normal,
        Heading1,
        Heading2,
        Heading3,
        Caption,
        Body,
        Overline
    };

    void setLabelType(LabelType type);
    LabelType labelType() const;

    // 对齐方式
    void setAlignment(Qt::Alignment alignment);
    Qt::Alignment alignment() const;

    // 文本颜色覆盖
    void setTextColor(const QColor& color);
    QColor textColor() const;

protected:
    void applyTheme() override;

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::ui

#include "Label.moc"