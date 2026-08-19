#pragma once

#include <QObject>
#include <QString>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QMap>
#include <functional>
#include <QSet>

namespace explorer::ipc {

// 消息总线 - 简化的 DBus 消息发布/订阅
class MessageBus : public QObject {
    Q_OBJECT
public:
    explicit MessageBus(QObject* parent = nullptr);
    ~MessageBus() override;

    // 初始化
    bool init(const QDBusConnection& connection = QDBusConnection::sessionBus());

    // 发布消息
    void publish(const QString& topic, const QVariant& message = {});
    void publish(const QString& topic, const QString& message);
    void publish(const QString& topic, int message);
    void publish(const QString& topic, bool message);

    // 订阅主题
    using MessageHandler = std::function<void(const QString& topic, const QVariant& message)>;
    int subscribe(const QString& topic, MessageHandler handler);
    void unsubscribe(int subscriptionId);
    void unsubscribeAll(const QString& topic);

    // 直接发送 DBus 消息
    bool sendMessage(const QDBusMessage& message);
    QDBusMessage receiveMessage(int timeoutMs = 1000);

signals:
    void messagePublished(const QString& topic, const QVariant& message);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

// 服务监视器 - 监视 DBus 服务的出现和消失
class ServiceWatcher : public QObject {
    Q_OBJECT
public:
    explicit ServiceWatcher(QObject* parent = nullptr);
    ~ServiceWatcher() override;

    // 添加监视的服务
    bool watchService(const QString& serviceName);
    void unwatchService(const QString& serviceName);
    void unwatchAll();

    // 检查服务是否可用
    bool isServiceAvailable(const QString& serviceName) const;

signals:
    void serviceAppeared(const QString& serviceName);
    void serviceVanished(const QString& serviceName);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::ipc

#include "MessageBus.moc"
#include "ServiceWatcher.moc"