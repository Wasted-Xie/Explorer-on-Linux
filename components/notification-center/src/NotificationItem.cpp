#include "NotificationItem.h"
#include <QIcon>
#include <QDebug>

namespace explorer::notification {

class NotificationItem::Impl {
public:
    QString id;
    QString summary;
    QString body;
    QString iconName;
    QIcon icon;
    QDateTime timestamp;
    int timeout = -1;
    bool expired = false;
};

NotificationItem::NotificationItem(const QString& id,
                                   const QString& summary,
                                   const QString& body,
                                   const QString& iconName,
                                   int timeout,
                                   QObject* parent)
    : QObject(parent), d(std::make_unique<Impl>()) {
    d->id = id;
    d->summary = summary;
    d->body = body;
    d->iconName = iconName;
    d->icon = QIcon::fromTheme(iconName);
    d->timestamp = QDateTime::currentDateTime();
    d->timeout = timeout;
}

NotificationItem::~NotificationItem() = default;

QString NotificationItem::id() const { return d->id; }
QString NotificationItem::summary() const { return d->summary; }
QString NotificationItem::body() const { return d->body; }
QString NotificationItem::iconName() const { return d->iconName; }
QIcon NotificationItem::icon() const { return d->icon; }
QDateTime NotificationItem::timestamp() const { return d->timestamp; }
int NotificationItem::timeout() const { return d->timeout; }
bool NotificationItem::isExpired() const { return d->expired; }

void NotificationItem::setExpired(bool expired) {
    if (d->expired != expired) {
        d->expired = expired;
        emit expiredChanged();
    }
}

QVariantMap NotificationItem::toVariantMap() const {
    QVariantMap map;
    map["id"] = d->id;
    map["summary"] = d->summary;
    map["body"] = d->body;
    map["iconName"] = d->iconName;
    map["timestamp"] = d->timestamp.toString(Qt::ISODate);
    map["timeout"] = d->timeout;
    map["expired"] = d->expired;
    return map;
}

} // namespace explorer::notification