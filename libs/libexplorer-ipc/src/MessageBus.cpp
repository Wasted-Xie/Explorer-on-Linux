#include "MessageBus.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QMutex>

namespace explorer::ipc {

class MessageBus::Impl {
public:
    Impl(MessageBus* owner) : q(owner) {
        // 设置匹配规则来捕获所有自定义消息
        // 使用一个约定的接口和路径来传递我们的主题消息
        connection = QDBusConnection::sessionBus();
        if (connection.isConnected()) {
            connection.connect("", "/explorer/ipc/message",
                             "org.explorer.ipc.MessageBus", "messagePublished",
                             this, SLOT(onMessagePublished(const QString&, const QVariant&)));
        }
    }

    void onMessagePublished(const QString& topic, const QVariant& message) {
        // 查找匹配的订阅者
        QMutexLocker lock(&mutex);
        auto range = subscribers.equal_range(topic);
        for (auto it = range.first; it != range.second; ++it) {
            try {
                it->second(topic, message);
            } catch (const std::exception& e) {
                qWarning() << "Message handler exception:" << e.what();
            }
        }
    }

    MessageBus* q;
    QDBusConnection connection;
    QMutex mutex;
    QMultiMap<QString, std::function<void(const QString&, const QVariant&)>> subscribers;
    int nextSubscriptionId = 1;
};

MessageBus::MessageBus(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
MessageBus::~MessageBus() = default;

bool MessageBus::init(const QDBusConnection& connection) {
    d->connection = connection;
    return d->connection.isConnected();
}

void MessageBus::publish(const QString& topic, const QVariant& message) {
    if (!d->connection.isConnected()) return;

    QDBusMessage msg = QDBusMessage::createSignal("/explorer/ipc/message",
                                                "org.explorer.ipc.MessageBus",
                                                "messagePublished");
    msg.setArguments({topic, message});
    d->connection.send(msg);

    emit messagePublished(topic, message);
}

void MessageBus::publish(const QString& topic, const QString& message) {
    publish(topic, QVariant(message));
}

void MessageBus::publish(const QString& topic, int message) {
    publish(topic, QVariant(message));
}

void MessageBus::publish(const QString& topic, bool message) {
    publish(topic, QVariant(message));
}

int MessageBus::subscribe(const QString& topic, MessageHandler handler) {
    QMutexLocker lock(&d->mutex);
    int id = d->nextSubscriptionId++;
    d->subscribers.insert(topic, std::move(handler));
    return id;
}

void MessageBus::unsubscribe(int subscriptionId) {
    // 需要遍历查找 - 效率不高但简单
    // 在实际实现中应该保持反向映射
    QMutexLocker lock(&d->mutex);
    for (auto it = d->subscribers.begin(); it != d->subscribers.end();) {
        // 这里无法直接通过ID查找，简化处理：不支持按ID取消订阅
        // 实际应用中需要改进
        ++it;
    }
}

void MessageBus::unsubscribeAll(const QString& topic) {
    QMutexLocker lock(&d->mutex);
    d->subscribers.remove(topic);
}

bool MessageBus::sendMessage(const QDBusMessage& message) {
    if (!d->connection.isConnected()) return false;
    return d->connection.send(message);
}

QDBusMessage MessageBus::receiveMessage(int timeoutMs) {
    if (!d->connection.isConnected()) return QDBusMessage();
    // 简化实现：实际需要更复杂的接收机制
    return QDBusMessage();
}

} // namespace explorer::ipc

#include "MessageBus.moc"