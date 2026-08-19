#include "FileOperations.h"
#include <QThread>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
#include <atomic>

namespace explorer::core {

class FileOperations::Impl {
public:
    Impl(FileOperations* owner) : q(owner) {}

    void runCopy(const QString& source, const QString& dest,
                 ProgressCallback progress, FinishedCallback finished) {
        if (busy.load()) return;
        busy.store(true);
        cancelled.store(false);

        QThread* thread = new QThread();
        auto worker = new CopyWorker(source, dest, progress, finished, &cancelled);
        worker->moveToThread(thread);

        connect(thread, &QThread::started, worker, &CopyWorker::run);
        connect(worker, &CopyWorker::progress, q, &FileOperations::progressUpdated);
        connect(worker, &CopyWorker::finished, [=](const FileOperationResult& result) {
            finished(result);
            emit q->operationFinished(result);
            thread->quit();
        });
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(worker, &CopyWorker::finished, worker, &CopyWorker::deleteLater);

        emit q->operationStarted("copy");
        thread->start();
    }

    void runMove(const QString& source, const QString& dest,
                 ProgressCallback progress, FinishedCallback finished) {
        if (busy.load()) return;
        busy.store(true);
        cancelled.store(false);

        QThread* thread = new QThread();
        auto worker = new MoveWorker(source, dest, progress, finished, &cancelled);
        worker->moveToThread(thread);

        connect(thread, &QThread::started, worker, &MoveWorker::run);
        connect(worker, &MoveWorker::progress, q, &FileOperations::progressUpdated);
        connect(worker, &MoveWorker::finished, [=](const FileOperationResult& result) {
            finished(result);
            emit q->operationFinished(result);
            thread->quit();
        });
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(worker, &MoveWorker::finished, worker, &MoveWorker::deleteLater);

        emit q->operationStarted("move");
        thread->start();
    }

    void runRemove(const QStringList& paths,
                   ProgressCallback progress, FinishedCallback finished) {
        if (busy.load()) return;
        busy.store(true);
        cancelled.store(false);

        QThread* thread = new QThread();
        auto worker = new RemoveWorker(paths, progress, finished, &cancelled);
        worker->moveToThread(thread);

        connect(thread, &QThread::started, worker, &RemoveWorker::run);
        connect(worker, &RemoveWorker::progress, q, &FileOperations::progressUpdated);
        connect(worker, &RemoveWorker::finished, [=](const FileOperationResult& result) {
            finished(result);
            emit q->operationFinished(result);
            thread->quit();
        });
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(worker, &RemoveWorker::finished, worker, &RemoveWorker::deleteLater);

        emit q->operationStarted("delete");
        thread->start();
    }

    void cancelOperation() {
        cancelled.store(true);
    }

    bool isBusy() const { return busy.load(); }

    FileOperations* q;
    std::atomic<bool> busy{false};
    std::atomic<bool> cancelled{false};
};

// 基础工作类
class BaseWorker : public QObject {
    Q_OBJECT
public:
    BaseWorker(ProgressCallback progress, FinishedCallback finished, std::atomic<bool>* cancelled)
        : m_progress(std::move(progress)), m_finished(std::move(finished)), m_cancelled(cancelled) {}

    void reportProgress(const FileOperationProgress& progress) {
        if (m_progress) m_progress(progress);
        emit progressUpdated(progress);
    }

    void reportFinished(const FileOperationResult& result) {
        if (m_finished) m_finished(result);
        emit finished(result);
    }

    bool isCancelled() const { return m_cancelled && m_cancelled->load(); }

signals:
    void progressUpdated(const FileOperationProgress&);
    void finished(const FileOperationResult&);

protected:
    ProgressCallback m_progress;
    FinishedCallback m_finished;
    std::atomic<bool>* m_cancelled;
};

// 复制工作器
class CopyWorker : public BaseWorker {
    Q_OBJECT
public:
    CopyWorker(const QString& source, const QString& dest,
               ProgressCallback progress, FinishedCallback finished, std::atomic<bool>* cancelled)
        : BaseWorker(std::move(progress), std::move(finished), cancelled)
        , m_source(source), m_dest(dest) {}

public slots:
    void run() {
        FileOperationResult result;
        QFileInfo srcInfo(m_source);

        if (!srcInfo.exists()) {
            result.success = false;
            result.error = "源路径不存在: " + m_source;
            reportFinished(result);
            return;
        }

        if (srcInfo.isDir()) {
            result = copyDirectory(m_source, m_dest);
        } else {
            result = copyFile(m_source, m_dest);
        }

        reportFinished(result);
    }

private:
    FileOperationResult copyFile(const QString& src, const QString& dest) {
        FileOperationResult result;
        QFileInfo srcInfo(src);
        qint64 totalSize = srcInfo.size();

        FileOperationProgress progress;
        progress.operation = "copy";
        progress.sourcePath = src;
        progress.destPath = dest;
        progress.totalBytes = totalSize;
        progress.totalFiles = 1;
        progress.currentFile = 1;
        progress.currentFileName = srcInfo.fileName();
        reportProgress(progress);

        QFile srcFile(src);
        QFile destFile(dest);

        if (!srcFile.open(QIODevice::ReadOnly)) {
            result.success = false;
            result.error = "无法打开源文件: " + srcFile.errorString();
            return result;
        }

        if (!destFile.open(QIODevice::WriteOnly)) {
            result.success = false;
            result.error = "无法创建目标文件: " + destFile.errorString();
            return result;
        }

        const qint64 bufferSize = 64 * 1024;
        char buffer[bufferSize];
        qint64 processed = 0;

        while (!isCancelled() && !srcFile.atEnd()) {
            qint64 read = srcFile.read(buffer, bufferSize);
            if (read <= 0) break;

            qint64 written = destFile.write(buffer, read);
            if (written != read) {
                result.success = false;
                result.error = "写入目标文件失败: " + destFile.errorString();
                return result;
            }

            processed += written;
            progress.processedBytes = processed;
            reportProgress(progress);
        }

        srcFile.close();
        destFile.close();

        // 保留权限和时间
        QFile::setPermissions(dest, QFileInfo(src).permissions());
        QFileInfo destInfo(dest);
        QFile::setFileTime(dest, QFileInfo(src).lastModified(), QFile::FileModificationTime);

        result.success = true;
        result.totalBytes = processed;
        result.processedFiles << dest;
        return result;
    }

    FileOperationResult copyDirectory(const QString& src, const QString& dest) {
        FileOperationResult result;
        QDir srcDir(src);

        // 先计算总大小和文件数
        qint64 totalSize = 0;
        int totalFiles = 0;
        calculateDirStats(src, totalSize, totalFiles);

        FileOperationProgress progress;
        progress.operation = "copy";
        progress.sourcePath = src;
        progress.destPath = dest;
        progress.totalBytes = totalSize;
        progress.totalFiles = totalFiles;
        reportProgress(progress);

        if (!QDir().mkpath(dest)) {
            result.success = false;
            result.error = "无法创建目标目录: " + dest;
            return result;
        }

        qint64 processedBytes = 0;
        int processedFiles = 0;
        bool success = copyDirRecursive(src, dest, progress, processedBytes, processedFiles, result);

        result.success = success;
        result.totalBytes = processedBytes;
        return result;
    }

    void calculateDirStats(const QString& path, qint64& totalSize, int& totalFiles) {
        QDir dir(path);
        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
        for (const QFileInfo& info : entries) {
            if (info.isDir()) {
                calculateDirStats(info.absoluteFilePath(), totalSize, totalFiles);
            } else {
                totalSize += info.size();
                totalFiles++;
            }
        }
        totalFiles++; // 目录本身
    }

    bool copyDirRecursive(const QString& src, const QString& dest,
                          FileOperationProgress& progress,
                          qint64& processedBytes, int& processedFiles,
                          FileOperationResult& result) {
        QDir srcDir(src);
        QFileInfoList entries = srcDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

        for (const QFileInfo& info : entries) {
            if (isCancelled()) return false;

            QString srcPath = info.absoluteFilePath();
            QString destPath = dest + "/" + info.fileName();

            progress.currentFileName = info.fileName();
            progress.currentFile = ++processedFiles;
            reportProgress(progress);

            if (info.isDir()) {
                if (!QDir().mkpath(destPath)) {
                    result.error = "无法创建目录: " + destPath;
                    return false;
                }
                if (!copyDirRecursive(srcPath, destPath, progress, processedBytes, processedFiles, result)) {
                    return false;
                }
            } else {
                QFile srcFile(srcPath);
                QFile destFile(destPath);

                if (!srcFile.open(QIODevice::ReadOnly) || !destFile.open(QIODevice::WriteOnly)) {
                    result.error = "无法打开文件: " + srcPath;
                    return false;
                }

                const qint64 bufferSize = 64 * 1024;
                char buffer[bufferSize];
                while (!isCancelled() && !srcFile.atEnd()) {
                    qint64 read = srcFile.read(buffer, bufferSize);
                    if (read <= 0) break;
                    qint64 written = destFile.write(buffer, read);
                    if (written != read) {
                        result.error = "写入失败: " + destPath;
                        return false;
                    }
                    processedBytes += written;
                    progress.processedBytes = processedBytes;
                    reportProgress(progress);
                }

                srcFile.close();
                destFile.close();

                QFile::setPermissions(destPath, info.permissions());
                QFile::setFileTime(destPath, info.lastModified(), QFile::FileModificationTime);
            }

            result.processedFiles << destPath;
        }
        return true;
    }

    QString m_source, m_dest;
};

// 移动工作器
class MoveWorker : public BaseWorker {
    Q_OBJECT
public:
    MoveWorker(const QString& source, const QString& dest,
               ProgressCallback progress, FinishedCallback finished, std::atomic<bool>* cancelled)
        : BaseWorker(std::move(progress), std::move(finished), cancelled)
        , m_source(source), m_dest(dest) {}

public slots:
    void run() {
        FileOperationResult result;

        // 尝试直接重命名（同文件系统）
        if (QFile::rename(m_source, m_dest)) {
            result.success = true;
            result.processedFiles << m_dest;
            QFileInfo info(m_dest);
            result.totalBytes = info.size();
            reportFinished(result);
            return;
        }

        // 跨文件系统：复制后删除
        FileOperationProgress progress;
        progress.operation = "move";
        progress.sourcePath = m_source;
        progress.destPath = m_dest;
        reportProgress(progress);

        // 复制
        CopyWorker copyWorker(m_source, m_dest, m_progress, [this, &result](const FileOperationResult& copyResult) {
            if (!copyResult.success) {
                result = copyResult;
                reportFinished(result);
                return;
            }

            // 删除源
            RemoveWorker removeWorker({m_source}, m_progress, [this, &result](const FileOperationResult& removeResult) {
                result = copyResult;
                if (!removeResult.success) {
                    result.error = "复制成功但删除源失败: " + removeResult.error;
                }
                reportFinished(result);
            }, m_cancelled);
            removeWorker.run();
        }, m_cancelled);
        copyWorker.run();
    }

private:
    QString m_source, m_dest;
};

// 删除工作器
class RemoveWorker : public BaseWorker {
    Q_OBJECT
public:
    RemoveWorker(const QStringList& paths,
                 ProgressCallback progress, FinishedCallback finished, std::atomic<bool>* cancelled)
        : BaseWorker(std::move(progress), std::move(finished), cancelled)
        , m_paths(paths) {}

public slots:
    void run() {
        FileOperationResult result;
        int totalFiles = m_paths.size();
        int processedFiles = 0;

        for (const QString& path : m_paths) {
            if (isCancelled()) break;

            QFileInfo info(path);
            FileOperationProgress progress;
            progress.operation = "delete";
            progress.sourcePath = path;
            progress.totalFiles = totalFiles;
            progress.currentFile = ++processedFiles;
            progress.currentFileName = info.fileName();
            reportProgress(progress);

            bool success = false;
            if (info.isDir()) {
                QDir dir(path);
                success = dir.removeRecursively();
            } else {
                success = QFile::remove(path);
            }

            if (!success) {
                result.success = false;
                result.error = "删除失败: " + path;
                reportFinished(result);
                return;
            }

            result.processedFiles << path;
        }

        result.success = true;
        reportFinished(result);
    }

private:
    QStringList m_paths;
};

FileOperations::FileOperations(QObject* parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}
FileOperations::~FileOperations() = default;

void FileOperations::copy(const QString& source, const QString& dest,
                          ProgressCallback progress, FinishedCallback finished) {
    d->runCopy(source, dest, std::move(progress), std::move(finished));
}

void FileOperations::move(const QString& source, const QString& dest,
                          ProgressCallback progress, FinishedCallback finished) {
    d->runMove(source, dest, std::move(progress), std::move(finished));
}

void FileOperations::remove(const QStringList& paths,
                            ProgressCallback progress, FinishedCallback finished) {
    d->runRemove(paths, std::move(progress), std::move(finished));
}

void FileOperations::mkdir(const QString& path, bool createParents, FinishedCallback finished) {
    QDir dir;
    bool success = createParents ? dir.mkpath(path) : dir.mkdir(path);

    FileOperationResult result;
    result.success = success;
    if (!success) {
        result.error = "创建目录失败: " + path;
    }
    if (finished) finished(result);
    emit operationFinished(result);
}

void FileOperations::rename(const QString& source, const QString& newName, FinishedCallback finished) {
    QFileInfo info(source);
    QString dest = info.absolutePath() + "/" + newName;

    FileOperationResult result;
    result.success = QFile::rename(source, dest);
    if (!result.success) {
        result.error = "重命名失败: " + source;
    } else {
        result.processedFiles << dest;
    }
    if (finished) finished(result);
    emit operationFinished(result);
}

void FileOperations::cancel() {
    d->cancelOperation();
}

bool FileOperations::isBusy() const {
    return d->isBusy();
}

qint64 FileOperations::directorySize(const QString& path) {
    qint64 size = 0;
    QDir dir(path);
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            size += directorySize(info.absoluteFilePath());
        } else {
            size += info.size();
        }
    }
    return size;
}

int FileOperations::countFiles(const QString& path) {
    int count = 0;
    QDir dir(path);
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            count += countFiles(info.absoluteFilePath());
        } else {
            count++;
        }
    }
    return count;
}

} // namespace explorer::core

#include "FileOperations.moc"