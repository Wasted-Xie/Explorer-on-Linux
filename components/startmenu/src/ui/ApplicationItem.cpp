#include "ApplicationItem.h"
#include <QPainter>
#include <QMouseEvent>
#include <QStyleOption>
#include <QDebug>
#include <QProcess>

namespace explorer::startmenu {

class ApplicationItem::Impl {
public:
    QString name;
    QString exec;
    QIcon icon;
    bool hovered = false;
    bool pressed = false;
    int iconSize = 24;
    int padding = 8;
};

ApplicationItem::ApplicationItem(const QString& name, const QString& exec, const QIcon& icon, QWidget* parent)
    : BaseWidget(parent), d(std::make_unique<Impl>()) {
    d->name = name;
    d->exec = exec;
    d->icon = icon;
    
    setMouseTracking(true);
    setFixedHeight(44);
    setCursor(Qt::PointingHandCursor);
    
    // 从配置读取图标大小
    auto& config = explorer::core::Config::instance();
    d->iconSize = config.value("startmenu/iconSize", 24).toInt();
    d->padding = config.value("startmenu/itemPadding", 8).toInt();
    
    applyTheme();
}

ApplicationItem::~ApplicationItem() = default;

QString ApplicationItem::name() const {
    return d->name;
}

QString ApplicationItem::exec() const {
    return d->exec;
}

QIcon ApplicationItem::icon() const {
    return d->icon;
}

void ApplicationItem::setName(const QString& name) {
    d->name = name;
    update();
}

void ApplicationItem::setExec(const QString& exec) {
    d->exec = exec;
}

void ApplicationItem::setIcon(const QIcon& icon) {
    d->icon = icon;
    update();
}

void ApplicationItem::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && rect().contains(event->pos())) {
        emit launched(d->exec);
    }
    BaseWidget::mouseReleaseEvent(event);
}

void ApplicationItem::enterEvent(QEnterEvent* event) {
    d->hovered = true;
    update();
    BaseWidget::enterEvent(event);
}

void ApplicationItem::leaveEvent(QEvent* event) {
    d->hovered = false;
    d->pressed = false;
    update();
    BaseWidget::leaveEvent(event);
}

void ApplicationItem::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect rect = this->rect();
    
    // 背景色
    if (d->hovered) {
        painter.fillRect(rect, palette().color(QPalette::Highlight).lighter(180));
    } else {
        painter.fillRect(rect, palette().color(QPalette::Base));
    }
    
    // 绘制图标
    int x = d->padding;
    int y = (height() - d->iconSize) / 2;
    
    if (!d->icon.isNull()) {
        QPixmap pixmap = d->icon.pixmap(d->iconSize, d->iconSize);
        painter.drawPixmap(x, y, pixmap);
    } else {
        // 默认图标占位符
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(x, y, d->iconSize, d->iconSize, Qt::AlignCenter, "📦");
    }
    
    // 绘制名称
    x += d->iconSize + d->padding;
    QRect textRect(x, 0, width() - x - d->padding, height());
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, d->name);
    
    BaseWidget::paintEvent(event);
}

} // namespace explorer::startmenu