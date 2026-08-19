#include "SignalDispatcher.h"
#include <QDebug>

namespace explorer::core {

SignalDispatcher& SignalDispatcher::instance() {
    static SignalDispatcher inst;
    return inst;
}

SignalDispatcher::SignalDispatcher() : QObject(nullptr) {}

SignalDispatcher::~SignalDispatcher() {
    QMutexLocker lock(&m_mutex);
    m_signals.clear();
    m_connectionMap.clear();
}

template<typename... Args>
void SignalDispatcher::emit(const QString& signalName, Args&&... args) {
    QVector<QVariant> variantArgs = {QVariant::fromValue(std::forward<Args>(args))...};

    QMutexLocker lock(&m_mutex);
    auto it = m_signals.find(signalName);
    if (it == m_signals.end()) return;

    // 复制连接列表避免在调用时持有锁
    QVector<ConnectionBase*> connections;
    connections.reserve(it->get()->connections.size());
    for (auto& conn : it->get()->connections) {
        if (conn) connections.append(conn.get());
    }

    lock.unlock();

    for (auto* conn : connections) {
        try {
            conn->invoke(variantArgs);
        } catch (const std::exception& e) {
            qWarning() << "SignalDispatcher: exception in slot for" << signalName << ":" << e.what();
        } catch (...) {
            qWarning() << "SignalDispatcher: unknown exception in slot for" << signalName;
        }
    }
}

template<typename Func>
int SignalDispatcher::connect(const QString& signalName, Func&& func) {
    using DecayedFunc = std::decay_t<Func>;
    using ArgsTuple = typename function_traits<DecayedFunc>::args_tuple;

    QMutexLocker lock(&m_mutex);

    auto& signalData = m_signals[signalName];
    if (!signalData) {
        signalData = std::make_unique<SignalData>();
    }

    int connectionId = signalData->nextConnectionId++;
    int globalId = m_nextGlobalConnectionId++;

    // 存储类型擦除的连接
    signalData->connections.append(
        std::make_unique<Connection<typename function_traits<DecayedFunc>::args...>>(
            std::function<void(typename function_traits<DecayedFunc>::args...)>(std::forward<Func>(func))
        )
    );

    m_connectionMap[globalId] = {signalName, connectionId - 1};

    return globalId;
}

void SignalDispatcher::disconnect(int connectionId) {
    QMutexLocker lock(&m_mutex);

    auto it = m_connectionMap.find(connectionId);
    if (it == m_connectionMap.end()) return;

    const QString& signalName = it->first;
    int index = it->second;

    auto signalIt = m_signals.find(signalName);
    if (signalIt != m_signals.end() && index < signalIt->get()->connections.size()) {
        signalIt->get()->connections[index].reset(); // 标记为空，emit 时会跳过
    }

    m_connectionMap.erase(it);
}

void SignalDispatcher::disconnectAll(const QString& signalName) {
    QMutexLocker lock(&m_mutex);

    auto it = m_signals.find(signalName);
    if (it == m_signals.end()) return;

    // 清理 connectionMap 中对应的项
    for (auto mapIt = m_connectionMap.begin(); mapIt != m_connectionMap.end();) {
        if (mapIt->first == signalName) {
            mapIt = m_connectionMap.erase(mapIt);
        } else {
            ++mapIt;
        }
    }

    it->get()->connections.clear();
    it->get()->nextConnectionId = 1;
}

bool SignalDispatcher::hasListeners(const QString& signalName) const {
    QMutexLocker lock(&m_mutex);
    auto it = m_signals.find(signalName);
    if (it == m_signals.end()) return false;

    for (const auto& conn : it->get()->connections) {
        if (conn) return true;
    }
    return false;
}

int SignalDispatcher::listenerCount(const QString& signalName) const {
    QMutexLocker lock(&m_mutex);
    auto it = m_signals.find(signalName);
    if (it == m_signals.end()) return 0;

    int count = 0;
    for (const auto& conn : it->get()->connections) {
        if (conn) ++count;
    }
    return count;
}

// 函数特性辅助
namespace detail {
    template<typename T>
    struct function_traits;

    template<typename R, typename... Args>
    struct function_traits<R(Args...)> {
        using args_tuple = std::tuple<Args...>;
        template<std::size_t I>
        using arg = typename std::tuple_element<I, args_tuple>::type;
        static constexpr std::size_t arity = sizeof...(Args);
    };

    template<typename R, typename... Args>
    struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> {};

    template<typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...)> : function_traits<R(Args...)> {};

    template<typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...) const> : function_traits<R(Args...)> {};

    template<typename Func>
    struct function_traits : function_traits<decltype(&Func::operator())> {};
}

} // namespace explorer::core

// 显式实例化常用的发射签名
namespace explorer::core {
    template void SignalDispatcher::emit<>(const QString&);
    template void SignalDispatcher::emit<int>(const QString&, int);
    template void SignalDispatcher::emit<QString>(const QString&, QString);
    template void SignalDispatcher::emit<int, QString>(const QString&, int, QString);
    template void SignalDispatcher::emit<QString, QString>(const QString&, QString, QString);
    template void SignalDispatcher::emit<bool>(const QString&, bool);
    template void SignalDispatcher::emit<int, int>(const QString&, int, int);
}