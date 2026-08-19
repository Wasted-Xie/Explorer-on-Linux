#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QPointer>
#include "NotificationItem.h"

namespace explorer::notification {

// 通知列表模型
class NotificationModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        SummaryRole,
        BodyRole,
        IconNameRole,
        IconRole,
        TimestampRole,
        TimeoutRole,
        ExpiredRole
    };

    explicit NotificationModel(QObject* parent = nullptr);
    ~NotificationModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 添加通知
    void addNotification(NotificationItem* item);
    // 移除通知
    void removeNotification(const QString& id);
    // 清空所有通知
    void clearAll();
    // 获取指定索引的通知项
    NotificationItem* itemAt(int index) const;
    // 通过 ID 查找通知项
    NotificationItem* findById(const QString& id) const;
    // 获取通知数量
    int count() const;

    // 设置最大通知数量
    void setMaxNotifications(int max);
    int maxNotifications() const;

signals:
    void notificationAdded(NotificationItem* item);
    void notificationRemoved(const QString& id);
    void countChanged(int count);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::notification

#include "NotificationModel.moc"