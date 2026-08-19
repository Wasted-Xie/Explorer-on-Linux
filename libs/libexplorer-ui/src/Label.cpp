#include "Label.h"
#include <QPainter>
#include <QApplication>

namespace explorer::ui {

class Label::Impl {
public:
    Impl() : m_labelType(LabelType::Normal), m_alignment(Qt::AlignLeft | Qt::AlignVCenter) {}

    LabelType m_labelType;
    Qt::Alignment m_alignment;
    QColor m_textColor; // 空表示使用主题颜色
};

Label::Label(const QString& text, QWidget* parent) 
    : QLabel(text, parent), d(std::make_unique<Impl>(this)) {
    setWordWrap(true);
    applyTheme();
}

Label::Label(QWidget* parent) 
    : QLabel(parent), d(std::make_unique<Impl>(this)) {
    setWordWrap(true);
    applyTheme();
}

Label::~Label() = default;

void Label::setLabelType(LabelType type) {
    if (d->m_labelType == type) return;
    d->m_labelType = type;
    updateStyle();
}

Label::LabelType Label::labelType() const {
    return d->m_labelType;
}

void Label::setAlignment(Qt::Alignment alignment) {
    if (d->m_alignment == alignment) return;
    d->m_alignment = alignment;
    setAlignment(alignment); // 调用基类方法
}

Qt::Alignment Label::alignment() const {
    return d->m_alignment;
}

void Label::setTextColor(const QColor& color) {
    if (m_textColor == color) return;
    m_textColor = color;
    update();
}

QColor Label::textColor() const {
    return m_textColor;
}

void Label::applyTheme() {
    updateStyle();
    update();
}

void Label::updateStyle() {
    // 获取主题颜色
    auto& theme = ThemeManager::instance();
    QColor color = m_textColor.isValid() ? m_textColor : theme.color(ThemeManager::ColorRole::WindowText);
    
    // 根据标签类型调整字体
    QFont font = this->font();
    switch (d->m_labelType) {
        case LabelType::Heading1:
            font.setPointSize(24);
            font.setWeight(QFont::Bold);
            break;
        case LabelType::Heading2:
            font.setPointSize(20);
            font.setWeight(QFont::Bold);
            break;
        case LabelType::Heading3:
            font.setPointSize(16);
            font.setWeight(QFont::Bold);
            break;
        case LabelType::Caption:
            font.setPointSize(12);
            font.setWeight(QFont::Normal);
            break;
        case LabelType::Body:
            font.setPointSize(14);
            font.setWeight(QFont::Normal);
            break;
        case LabelType::Overline:
            font.setPointSize(10);
            font.setWeight(QFont::Normal);
            font.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
            break;
        case LabelType::Normal:
        default:
            font.setPointSize(12);
            font.setWeight(QFont::Normal);
            break;
    }
    setFont(font);
    
    // 设置样式表
    QString styleSheet = QString("QLabel { color: %1; }").arg(color.name());
    setStyleSheet(styleSheet);
}

} // namespace explorer::ui

#include "Label.moc"