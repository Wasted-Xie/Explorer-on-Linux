#pragma once

#include <QObject>
#include <QString>
#include <QIcon>
#include <QDateTime>
#include <QVariant>

namespace explorer::notification {

// 通知项数据结构
class NotificationItem : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString summary READ summary CONSTANT)
    Q_PROPERTY(QString body READ body CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QIcon icon READ icon CONSTANT)
    Q_PROPERTY(QDateTime timestamp READ timestamp CONSTANT)
    Q_PROPERTY(int timeout READ timeout CONSTANT)
    Q_PROPERTY(bool isExpired READ isExpired NOTIFY expiredChanged)

public:
    explicit NotificationItem(const QString& id,
                              const QString& summary,
                              const QString& body,
                              const QString& iconName,
                              int timeout,
                              QObject* parent = nullptr);
    ~NotificationItem() override;

    QString id() const;
    QString summary() const;
    QString body() const;
    QString iconName() const;
    QIcon icon() const;
    QDateTime timestamp() const;
    int timeout() const; // 超时时间（秒），<=0 表示不自动过期
    bool isExpired() const;

    // 标记为已过期
    void setExpired(bool expired);

    // 转换为 QVariantMap 用于 DBus 传输
    QVariantMap toVariantMap() const;

signals:
    void expiredChanged();

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::notification

#include "NotificationItem.moc"