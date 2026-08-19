#include "NotificationDelegate.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QMouseEvent>
#include <QApplication>
#include <QStyle>
#include <QRect>
#include <QIcon>
#include <QDateTime>
#include <QDebug>
#include <explorer/utils/DateTimeUtils.h>

namespace explorer::notification {

struct NotificationDelegate::Impl {
    QColor backgroundColor = Qt::white;
    QColor textColor = Qt::black;
    QColor summaryColor = Qt::black;
    QColor timestampColor = Qt::gray;
    QColor borderColor = QColor(200, 200, 200);
    int padding = 12;
    int iconSize = 48;
    int spacing = 8;
    int closeButtonSize = 24;
    int maxBodyLines = 3;
};

NotificationDelegate::NotificationDelegate(QObject* parent)
    : QStyledItemDelegate(parent), d(std::make_unique<Impl>()) {}

NotificationDelegate::~NotificationDelegate() = default;

void NotificationDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
    if (!index.isValid()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    QRect rect = option.rect;
    const int padding = d->padding;

    // 绘制背景
    QColor bgColor = d->backgroundColor;
    if (option.state & QStyle::State_Selected) {
        bgColor = bgColor.lighter(110);
    } else if (option.state & QStyle::State_MouseOver) {
        bgColor = bgColor.lighter(105);
    }
    painter->fillRect(rect, bgColor);

    // 绘制边框
    painter->setPen(QPen(d->borderColor, 1));
    painter->drawRect(rect.adjusted(0, 0, -1, -1));

    // 获取数据
    QString summary = index.data(NotificationModel::SummaryRole).toString();
    QString body = index.data(NotificationModel::BodyRole).toString();
    QIcon icon = qvariant_cast<QIcon>(index.data(NotificationModel::IconRole));
    QDateTime timestamp = index.data(NotificationModel::TimestampRole).toDateTime();
    bool expired = index.data(NotificationModel::ExpiredRole).toBool();

    // 图标区域
    QRect iconRect(padding, padding, d->iconSize, d->iconSize);
    if (!icon.isNull()) {
        QPixmap pixmap = icon.pixmap(d->iconSize, d->iconSize);
        painter->drawPixmap(iconRect, pixmap);
    } else {
        // 绘制默认图标占位符
        painter->setPen(QPen(d->timestampColor, 1, Qt::DashLine));
        painter->drawRect(iconRect);
    }

    // 文本区域
    int textLeft = iconRect.right() + d->spacing;
    int textRight = rect.right() - padding - d->closeButtonSize - d->spacing;
    int textWidth = textRight - textLeft;

    if (textWidth > 0) {
        // Summary (粗体)
        QFont summaryFont = option.font;
        summaryFont.setBold(true);
        summaryFont.setPointSizeF(summaryFont.pointSizeF() * 1.0);
        painter->setFont(summaryFont);
        painter->setPen(d->summaryColor);

        QRect summaryRect(textLeft, padding, textWidth, 0);
        summaryRect.setHeight(painter->fontMetrics().height());
        QString elidedSummary = painter->fontMetrics().elidedText(summary, Qt::ElideRight, textWidth);
        painter->drawText(summaryRect, Qt::AlignLeft | Qt::AlignVCenter, elidedSummary);

        // Timestamp
        QFont timestampFont = option.font;
        timestampFont.setPointSizeF(timestampFont.pointSizeF() * 0.8);
        painter->setFont(timestampFont);
        painter->setPen(d->timestampColor);

        QString timeStr = explorer::utils::DateTimeUtils::toShortRelativeString(timestamp);
        QRect timeRect(textLeft, summaryRect.bottom() + 2, textWidth, painter->fontMetrics().height());
        QString elidedTime = painter->fontMetrics().elidedText(timeStr, Qt::ElideRight, textWidth);
        painter->drawText(timeRect, Qt::AlignLeft | Qt::AlignVCenter, elidedTime);

        // Body
        if (!body.isEmpty()) {
            QFont bodyFont = option.font;
            bodyFont.setPointSizeF(bodyFont.pointSizeF() * 0.9);
            painter->setFont(bodyFont);
            painter->setPen(d->textColor);

            QRect bodyRect(textLeft, timeRect.bottom() + 4, textWidth, 0);
            int lineHeight = painter->fontMetrics().height();
            int maxBodyHeight = lineHeight * d->maxBodyLines;
            
            // 使用 QTextOption 处理换行
            QTextOption textOption(Qt::AlignLeft | Qt::AlignTop);
            textOption.setWrapMode(QTextOption::WordWrap);
            
            QString elidedBody = painter->fontMetrics().elidedText(body, Qt::ElideRight, textWidth * d->maxBodyLines);
            painter->drawText(bodyRect, textOption, elidedBody);
        }
    }

    // 关闭按钮区域
    QRect closeRect(rect.right() - padding - d->closeButtonSize,
                    rect.top() + padding,
                    d->closeButtonSize, d->closeButtonSize);
    
    // 只有鼠标悬停时显示关闭按钮
    if (option.state & QStyle::State_MouseOver) {
        painter->setPen(QPen(d->timestampColor, 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(closeRect);
        
        // 绘制 X
        QPen crossPen(d->timestampColor, 2);
        painter->setPen(crossPen);
        int crossPadding = 6;
        QRect crossRect = closeRect.adjusted(crossPadding, crossPadding, -crossPadding, -crossPadding);
        painter->drawLine(crossRect.topLeft(), crossRect.bottomRight());
        painter->drawLine(crossRect.topRight(), crossRect.bottomLeft());
    }

    painter->restore();
}

QSize NotificationDelegate::sizeHint(const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const {
    Q_UNUSED(index)
    int height = d->padding * 2 + d->iconSize;
    // 根据内容调整高度
    QFontMetrics fm(option.font);
    height = qMax(height, d->padding * 2 + fm.height() * 4);
    return QSize(option.rect.width(), height);
}

void NotificationDelegate::setColors(const QColor& background, const QColor& text,
                                     const QColor& summaryText, const QColor& timestampText,
                                     const QColor& border) {
    d->backgroundColor = background;
    d->textColor = text;
    d->summaryColor = summaryText;
    d->timestampColor = timestampText;
    d->borderColor = border;
}

bool NotificationDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index) {
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QRect closeRect(option.rect.right() - d->padding - d->closeButtonSize,
                        option.rect.top() + d->padding,
                        d->closeButtonSize, d->closeButtonSize);
        
        if (closeRect.contains(mouseEvent->pos())) {
            emit closeClicked(index);
            return true;
        } else {
            emit notificationClicked(index);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

} // namespace explorer::notification