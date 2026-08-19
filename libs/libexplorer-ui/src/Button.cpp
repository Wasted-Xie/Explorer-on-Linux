#include "Button.h"
#include <QPainter>
#include <QStyleOptionButton>
#include <QApplication>

namespace explorer::ui {

class Button::Impl {
public:
    Impl() : m_buttonType(ButtonType::Normal), m_loading(false) {}

    ButtonType m_buttonType;
    bool m_loading;
    QPixmap m_loadingPixmap; // 用于加载动画
    int m_loadingAngle = 0;
};

Button::Button(const QString& text, QWidget* parent) 
    : QPushButton(text, parent), d(std::make_unique<Impl>(this)) {
    setCursor(Qt::PointingHandCursor);
    applyTheme();
}

Button::Button(const QIcon& icon, const QString& text, QWidget* parent)
    : QPushButton(icon, text, parent), d(std::make_unique<Impl>(this)) {
    setCursor(Qt::PointingHandCursor);
    applyTheme();
}

Button::~Button() = default;

void Button::setButtonType(ButtonType type) {
    if (d->m_buttonType == type) return;
    d->m_buttonType = type;
    updateStyle();
}

Button::ButtonType Button::buttonType() const {
    return d->m_buttonType;
}

void Button::setLoading(bool loading) {
    if (d->m_loading == loading) return;
    d->m_loading = loading;
    if (loading) {
        // 开始加载动画
        startTimer(50); // 每50ms更新一次
        setEnabled(false);
    } else {
        // 停止加载动画
        killTimer(timerId);
        setEnabled(true);
    }
    update();
}

bool Button::isLoading() const {
    return d->m_loading;
}

void Button::applyTheme() {
    updateStyle();
    update();
}

void Button::updateStyle() {
    QString styleSheet;
    
    // 获取主题颜色
    auto& theme = ThemeManager::instance();
    QColor bg = theme.color(ThemeManager::ColorRole::Button);
    QColor fg = theme.color(ThemeManager::ColorRole::ButtonText);
    QColor hover = bg.lighter(110);
    QColor pressed = bg.darker(110);
    
    // 根据按钮类型调整颜色
    switch (d->m_buttonType) {
        case ButtonType::Primary:
            bg = theme.color(ThemeManager::ColorRole::Highlight);
            fg = theme.color(ThemeManager::ColorRole::HighlightedText);
            hover = bg.lighter(110);
            pressed = bg.darker(110);
            break;
        case ButtonType::Success:
            bg = QColor(76, 175, 80); // 绿色
            fg = Qt::white;
            hover = bg.lighter(110);
            pressed = bg.darker(110);
            break;
        case ButtonType::Danger:
            bg = QColor(244, 67, 54); // 红色
            fg = Qt::white;
            hover = bg.lighter(110);
            pressed = bg.darker(110);
            break;
        case ButtonType::Warning:
            bg = QColor(255, 152, 0); // 橙色
            fg = Qt::white;
            hover = bg.lighter(110);
            pressed = bg.darker(110);
            break;
        case ButtonType::Info:
            bg = QColor(33, 150, 243); // 蓝色
            fg = Qt::white;
            hover = bg.lighter(110);
            pressed = bg.darker(110);
            break;
        case ButtonType::Link:
            bg = Qt::transparent;
            fg = theme.color(ThemeManager::ColorRole::Link);
            hover = fg.lighter(120);
            pressed = fg.darker(120);
            setStyleSheet("QButton { border: none; text-decoration: underline; }");
            return;
        case ButtonType::Secondary:
            bg = theme.color(ThemeManager::ColorRole::Mid);
            fg = theme.color(ThemeManager::ColorRole::WindowText);
            hover = bg.lighter(110);
            pressed = bg.darker(110);
            break;
        case ButtonType::Normal:
        default:
            // 使用默认主题颜色
            break;
    }
    
    styleSheet = QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: none;"
        "   padding: 6px 12px;"
        "   border-radius: 4px;"
        "   font-weight: normal;"
        "}"
        "QPushButton:hover {"
        "   background-color: %3;"
        "}"
        "QPushButton:pressed {"
        "   background-color: %4;"
        "}"
        "QPushButton:disabled {"
        "   background-color: %5;"
        "   color: %6;"
        "}"
    ).arg(bg.name(), fg.name(), hover.name(), pressed.name(), 
        bg.lighter(120).name(), fg.darker(120).name());
    
    setStyleSheet(styleSheet);
}

void Button::timerEvent(QTimerEvent* event) {
    if (d->m_loading) {
        d->m_loadingAngle = (d->m_loadingAngle + 10) % 360;
        update();
    }
    QWidget::timerEvent(event);
}

void Button::paintEvent(QPaintEvent* event) {
    QPushButton::paintEvent(event);
    
    if (d->m_loading) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 绘制加载旋转器
        int size = qMin(width(), height()) / 3;
        int centerX = width() / 2;
        int centerY = height() / 2;
        
        QPen pen(palette().color(QPalette::Text), 2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        
        painter.drawArc(QRect(centerX - size/2, centerY - size/2, size, size),
                       d->m_loadingAngle * 16, 270 * 16); // 270度弧
    }
}

} // namespace explorer::ui

#include "Button.moc"