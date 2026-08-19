#pragma once

#include <QString>
#include <QStringList>
#include <QProcess>
#include <QVariant>
#include <functional>
#include <optional>

namespace explorer::utils {

// 进程工具
class ProcessUtils {
public:
    // 同步执行
    struct Result {
        int exitCode = -1;
        QString stdout;
        QString stderr;
        bool timedOut = false;
        bool crashed = false;
    };

    static Result execute(const QString& program, const QStringList& arguments = {},
                          int timeoutMs = 30000, const QString& workingDir = {});
    static Result execute(const QString& command, int timeoutMs = 30000);

    // 异步执行
    class AsyncProcess : public QObject {
        Q_OBJECT
    public:
        using OutputCallback = std::function<void(const QString&)>;
        using FinishedCallback = std::function<void(const Result&)>;

        explicit AsyncProcess(QObject* parent = nullptr);
        ~AsyncProcess() override;

        void start(const QString& program, const QStringList& arguments = {},
                   const QString& workingDir = {});
        void start(const QString& command);
        void terminate();
        void kill();

        void setStdoutCallback(OutputCallback cb);
        void setStderrCallback(OutputCallback cb);
        void setFinishedCallback(FinishedCallback cb);

        bool isRunning() const;
        qint64 processId() const;

    signals:
        void stdoutReceived(const QString& data);
        void stderrReceived(const QString& data);
        void finished(const Result& result);
        void errorOccurred(QProcess::ProcessError error);

    private:
        class Impl;
        std::unique_ptr<Impl> d;
    };

    // 查找可执行文件
    static QString findExecutable(const QString& name);
    static QStringList findExecutables(const QString& name);

    // 进程信息
    static QStringList runningProcesses();
    static bool isProcessRunning(const QString& name);
    static qint64 getProcessId(const QString& name);
    static void killProcess(qint64 pid, int signal = SIGTERM);
    static void killProcessByName(const QString& name, int signal = SIGTERM);

    // 环境变量
    static QString getEnv(const QString& name, const QString& defaultValue = {});
    static void setEnv(const QString& name, const QString& value);
    static QStringList getEnvironment();
};

} // namespace explorer::utils