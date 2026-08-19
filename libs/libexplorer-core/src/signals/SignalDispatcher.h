#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QVector>
#include <QMutex>
#include <functional>
#include <memory>
#include <typeindex>

namespace explorer::core {

// 信号分发器 - 支持类型安全的信号/槽连接
class SignalDispatcher : public QObject {
    Q_OBJECT
public:
    static SignalDispatcher& instance();

    SignalDispatcher(const SignalDispatcher&) = delete;
    SignalDispatcher& operator=(const SignalDispatcher&) = delete;

    // 发射信号
    template<typename... Args>
    void emit(const QString& signalName, Args&&... args);

    // 连接信号
    template<typename Func>
    int connect(const QString& signalName, Func&& func);

    // 断开连接
    void disconnect(int connectionId);

    // 断开特定信号的所有连接
    void disconnectAll(const QString& signalName);

    // 检查是否有监听者
    bool hasListeners(const QString& signalName) const;

    // 获取监听者数量
    int listenerCount(const QString& signalName) const;

private:
    SignalDispatcher();
    ~SignalDispatcher() override;

    struct ConnectionBase {
        virtual ~ConnectionBase() = default;
        virtual void invoke(const QVector<QVariant>& args) = 0;
    };

    template<typename... Args>
    struct Connection : ConnectionBase {
        std::function<void(Args...)> func;

        Connection(std::function<void(Args...)> f) : func(std::move(f)) {}

        void invoke(const QVector<QVariant>& args) override {
            if (args.size() != sizeof...(Args)) return;
            invokeImpl(args, std::index_sequence_for<Args...>{});
        }

    private:
        template<std::size_t... I>
        void invokeImpl(const QVector<QVariant>& args, std::index_sequence<I...>) {
            func(args[I].value<Args>()...);
        }
    };

    struct SignalData {
        QVector<std::unique_ptr<ConnectionBase>> connections;
        int nextConnectionId = 1;
    };

    mutable QMutex m_mutex;
    QMap<QString, std::unique_ptr<SignalData>> m_signals;
    QMap<int, std::pair<QString, int>> m_connectionMap; // connectionId -> (signalName, index)
    int m_nextGlobalConnectionId = 1;
};

} // namespace explorer::core