#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>

namespace explorer::notification {

// 通知项自定义委托
class NotificationDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit NotificationDelegate(QObject* parent = nullptr);
    ~NotificationDelegate() override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    // 设置主题相关颜色
    void setColors(const QColor& background, const QColor& text,
                   const QColor& summaryText, const QColor& timestampText,
                   const QColor& border);

signals:
    // 点击关闭按钮
    void closeClicked(const QModelIndex& index);
    // 点击通知项
    void notificationClicked(const QModelIndex& index);

protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::notification

#include "NotificationDelegate.moc"