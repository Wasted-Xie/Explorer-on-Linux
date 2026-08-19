#include "BaseWidget.h"
#include <QApplication>
#include <QEvent>

namespace explorer::ui {

BaseWidget::BaseWidget(QWidget* parent) : QWidget(parent) {
    // 安装事件过滤器以检测主题变化
    qApp->installEventFilter(this);
}

BaseWidget::~BaseWidget() {
    qApp->removeEventFilter(this);
}

void BaseWidget::applyTheme() {
    // 子类可以重写此方法来应用特定的主题样式
    update();
}

void BaseWidget::setAutoTheme(bool enabled) {
    if (m_autoTheme == enabled) return;
    m_autoTheme = enabled;
    if (enabled) {
        applyTheme();
    }
}

bool BaseWidget::autoTheme() const {
    return m_autoTheme;
}

void BaseWidget::setIcon(const QIcon& icon) {
    if (m_icon == icon) return;
    m_icon = icon;
    update(); // 触发重绘以显示图标
}

QIcon BaseWidget::icon() const {
    return m_icon;
}

void BaseWidget::setPlaceholderText(const QString& text) {
    if (m_placeholderText == text) return;
    m_placeholderText = text;
    update();
}

QString BaseWidget::placeholderText() const {
    return m_placeholderText;
}

bool BaseWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == qApp && event->type() == QEvent::PaletteChange) {
        if (m_autoTheme) {
            applyTheme();
        }
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

} // namespace explorer::ui

#include "BaseWidget.moc"