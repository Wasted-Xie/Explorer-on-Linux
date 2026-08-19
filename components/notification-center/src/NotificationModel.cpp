#include "NotificationModel.h"
#include <QDebug>

namespace explorer::notification {

class NotificationModel::Impl {
public:
    QList<QPointer<NotificationItem>> items;
    int maxNotifications = 20;
};

NotificationModel::NotificationModel(QObject* parent)
    : QAbstractListModel(parent), d(std::make_unique<Impl>()) {}

NotificationModel::~NotificationModel() = default;

int NotificationModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return d->items.size();
}

QVariant NotificationModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= d->items.size())
        return QVariant();

    NotificationItem* item = d->items[index.row()];
    if (!item) return QVariant();

    switch (role) {
    case IdRole:
        return item->id();
    case SummaryRole:
        return item->summary();
    case BodyRole:
        return item->body();
    case IconNameRole:
        return item->iconName();
    case IconRole:
        return item->icon();
    case TimestampRole:
        return item->timestamp();
    case TimeoutRole:
        return item->timeout();
    case ExpiredRole:
        return item->isExpired();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> NotificationModel::roleNames() const {
    return {
        {IdRole, "id"},
        {SummaryRole, "summary"},
        {BodyRole, "body"},
        {IconNameRole, "iconName"},
        {IconRole, "icon"},
        {TimestampRole, "timestamp"},
        {TimeoutRole, "timeout"},
        {ExpiredRole, "expired"}
    };
}

void NotificationModel::addNotification(NotificationItem* item) {
    if (!item) return;

    // 检查是否已存在
    for (int i = 0; i < d->items.size(); ++i) {
        if (d->items[i] && d->items[i]->id() == item->id()) {
            // 已存在，更新现有项
            d->items[i] = item;
            emit dataChanged(index(i), index(i));
            return;
        }
    }

    // 检查是否超过最大数量
    if (d->items.size() >= d->maxNotifications) {
        // 移除最旧的通知
        beginRemoveRows(QModelIndex(), 0, 0);
        d->items.removeFirst();
        endRemoveRows();
    }

    // 添加新通知到末尾（最新的在最后）
    int row = d->items.size();
    beginInsertRows(QModelIndex(), row, row);
    d->items.append(item);
    endInsertRows();

    emit notificationAdded(item);
    emit countChanged(d->items.size());
}

void NotificationModel::removeNotification(const QString& id) {
    for (int i = 0; i < d->items.size(); ++i) {
        if (d->items[i] && d->items[i]->id() == id) {
            beginRemoveRows(QModelIndex(), i, i);
            d->items.removeAt(i);
            endRemoveRows();
            emit notificationRemoved(id);
            emit countChanged(d->items.size());
            return;
        }
    }
}

void NotificationModel::clearAll() {
    if (d->items.isEmpty()) return;
    beginRemoveRows(QModelIndex(), 0, d->items.size() - 1);
    d->items.clear();
    endRemoveRows();
    emit countChanged(0);
}

NotificationItem* NotificationModel::itemAt(int index) const {
    if (index < 0 || index >= d->items.size()) return nullptr;
    return d->items[index];
}

NotificationItem* NotificationModel::findById(const QString& id) const {
    for (auto* item : d->items) {
        if (item && item->id() == id) return item;
    }
    return nullptr;
}

int NotificationModel::count() const {
    return d->items.size();
}

void NotificationModel::setMaxNotifications(int max) {
    if (max < 1) max = 1;
    if (max != d->maxNotifications) {
        d->maxNotifications = max;
        // 如果当前数量超过最大值，移除旧的
        while (d->items.size() > d->maxNotifications) {
            beginRemoveRows(QModelIndex(), 0, 0);
            d->items.removeFirst();
            endRemoveRows();
        }
        emit countChanged(d->items.size());
    }
}

int NotificationModel::maxNotifications() const {
    return d->maxNotifications;
}

} // namespace explorer::notification