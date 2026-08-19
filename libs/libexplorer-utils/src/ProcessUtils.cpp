#include "ProcessUtils.h"
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QRegularExpression>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

namespace explorer::utils {

ProcessUtils::Result ProcessUtils::execute(const QString& program, const QStringList& arguments,
                                           int timeoutMs, const QString& workingDir) {
    QProcess process;
    if (!workingDir.isEmpty()) {
        process.setWorkingDirectory(workingDir);
    }

    process.start(program, arguments);
    if (!process.waitForStarted(5000)) {
        Result result;
        result.exitCode = -1;
        result.stderr = "Failed to start process: " + program;
        result.crashed = true;
        return result;
    }

    bool finished = process.waitForFinished(timeoutMs);
    Result result;
    result.exitCode = process.exitCode();
    result.stdout = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    result.stderr = QString::fromUtf8(process.readAllStandardError()).trimmed();
    result.timedOut = !finished;
    result.crashed = (process.error() == QProcess::Crashed);

    if (!finished) {
        process.kill();
        process.waitForFinished(1000);
    }

    return result;
}

ProcessUtils::Result ProcessUtils::execute(const QString& command, int timeoutMs) {
    // 简单的 shell 命令解析
    QStringList parts = command.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        Result result;
        result.exitCode = -1;
        result.stderr = "Empty command";
        return result;
    }
    return execute(parts.first(), parts.mid(1), timeoutMs);
}

// AsyncProcess 实现
class ProcessUtils::AsyncProcess::Impl {
public:
    Impl(AsyncProcess* owner) : q(owner) {
        connect(&process, &QProcess::readyReadStandardOutput, this, &Impl::onStdout);
        connect(&process, &QProcess::readyReadStandardError, this, &Impl::onStderr);
        connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &Impl::onFinished);
        connect(&process, &QProcess::errorOccurred, this, &Impl::onError);
    }

    void onStdout() {
        QString data = QString::fromUtf8(process.readAllStandardOutput());
        if (stdoutCallback) stdoutCallback(data);
        emit q->stdoutReceived(data);
    }

    void onStderr() {
        QString data = QString::fromUtf8(process.readAllStandardError());
        if (stderrCallback) stderrCallback(data);
        emit q->stderrReceived(data);
    }

    void onFinished(int exitCode, QProcess::ExitStatus status) {
        Result result;
        result.exitCode = exitCode;
        result.stdout = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        result.stderr = QString::fromUtf8(process.readAllStandardError()).trimmed();
        result.crashed = (status == QProcess::CrashExit);

        if (finishedCallback) finishedCallback(result);
        emit q->finished(result);
    }

    void onError(QProcess::ProcessError error) {
        emit q->errorOccurred(error);
    }

    QProcess process;
    AsyncProcess* q;
    OutputCallback stdoutCallback;
    OutputCallback stderrCallback;
    FinishedCallback finishedCallback;
};

ProcessUtils::AsyncProcess::AsyncProcess(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
ProcessUtils::AsyncProcess::~AsyncProcess() = default;

void ProcessUtils::AsyncProcess::start(const QString& program, const QStringList& arguments, const QString& workingDir) {
    if (!workingDir.isEmpty()) {
        d->process.setWorkingDirectory(workingDir);
    }
    d->process.start(program, arguments);
}

void ProcessUtils::AsyncProcess::start(const QString& command) {
    QStringList parts = command.split(' ', Qt::SkipEmptyParts);
    if (!parts.isEmpty()) {
        d->process.start(parts.first(), parts.mid(1));
    }
}

void ProcessUtils::AsyncProcess::terminate() {
    d->process.terminate();
}

void ProcessUtils::AsyncProcess::kill() {
    d->process.kill();
}

void ProcessUtils::AsyncProcess::setStdoutCallback(OutputCallback cb) {
    d->stdoutCallback = std::move(cb);
}

void ProcessUtils::AsyncProcess::setStderrCallback(OutputCallback cb) {
    d->stderrCallback = std::move(cb);
}

void ProcessUtils::AsyncProcess::setFinishedCallback(FinishedCallback cb) {
    d->finishedCallback = std::move(cb);
}

bool ProcessUtils::AsyncProcess::isRunning() const {
    return d->process.state() != QProcess::NotRunning;
}

qint64 ProcessUtils::AsyncProcess::processId() const {
    return d->process.processId();
}

QString ProcessUtils::findExecutable(const QString& name) {
    QString path = QStandardPaths::findExecutable(name);
    if (!path.isEmpty()) return path;

    // 检查常见路径
    QStringList searchPaths = {
        "/usr/bin", "/usr/local/bin", "/bin", "/sbin", "/usr/sbin",
        QCoreApplication::applicationDirPath()
    };

    for (const QString& dir : searchPaths) {
        QString fullPath = dir + "/" + name;
        if (QFile::exists(fullPath) && QFileInfo(fullPath).isExecutable()) {
            return fullPath;
        }
    }
    return QString();
}

QStringList ProcessUtils::findExecutables(const QString& name) {
    QStringList results;
    QString path = findExecutable(name);
    if (!path.isEmpty()) results << path;

    // 搜索 PATH
    QString pathEnv = qEnvironmentVariable("PATH");
    for (const QString& dir : pathEnv.split(':')) {
        QString fullPath = dir + "/" + name;
        if (QFile::exists(fullPath) && QFileInfo(fullPath).isExecutable() && !results.contains(fullPath)) {
            results << fullPath;
        }
    }
    return results;
}

QStringList ProcessUtils::runningProcesses() {
    // 使用 ps 命令获取进程列表
    Result result = execute("ps", {"-eo", "comm"}, 5000);
    if (result.exitCode == 0) {
        return result.stdout.split('\n', Qt::SkipEmptyParts);
    }
    return {};
}

bool ProcessUtils::isProcessRunning(const QString& name) {
    Result result = execute("pgrep", {"-x", name}, 5000);
    return result.exitCode == 0 && !result.stdout.trimmed().isEmpty();
}

qint64 ProcessUtils::getProcessId(const QString& name) {
    Result result = execute("pgrep", {"-x", name}, 5000);
    if (result.exitCode == 0) {
        QString pidStr = result.stdout.trimmed().split('\n').first();
        return pidStr.toLongLong();
    }
    return -1;
}

void ProcessUtils::killProcess(qint64 pid, int signal) {
    if (pid > 0) {
        ::kill(static_cast<pid_t>(pid), signal);
    }
}

void ProcessUtils::killProcessByName(const QString& name, int signal) {
    execute("pkill", {"-x", name, QString::number(signal)}, 5000);
}

QString ProcessUtils::getEnv(const QString& name, const QString& defaultValue) {
    return qEnvironmentVariable(name.toUtf8().constData(), defaultValue.toUtf8().constData());
}

void ProcessUtils::setEnv(const QString& name, const QString& value) {
    qputenv(name.toUtf8().constData(), value.toUtf8().constData());
}

QStringList ProcessUtils::getEnvironment() {
    QStringList result;
    for (const QByteArray& var : QProcess::systemEnvironment()) {
        result << QString::fromUtf8(var);
    }
    return result;
}

} // namespace explorer::utils