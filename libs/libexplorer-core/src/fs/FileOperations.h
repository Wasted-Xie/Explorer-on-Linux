#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <functional>
#include <memory>

namespace explorer::core {

// 文件操作进度信息
struct FileOperationProgress {
    QString operation;        // copy, move, delete, mkdir
    QString sourcePath;
    QString destPath;
    qint64 totalBytes = 0;
    qint64 processedBytes = 0;
    int currentFile = 0;
    int totalFiles = 0;
    QString currentFileName;
    bool cancelled = false;
    QString error;
};

// 文件操作结果
struct FileOperationResult {
    bool success = false;
    QString error;
    QStringList processedFiles;
    qint64 totalBytes = 0;
};

// 文件操作类 - 支持异步操作和进度回调
class FileOperations : public QObject {
    Q_OBJECT
public:
    explicit FileOperations(QObject* parent = nullptr);
    ~FileOperations() override;

    using ProgressCallback = std::function<void(const FileOperationProgress&)>;
    using FinishedCallback = std::function<void(const FileOperationResult&)>;

    // 复制文件/目录
    void copy(const QString& source, const QString& dest,
              ProgressCallback progress = {}, FinishedCallback finished = {});

    // 移动文件/目录
    void move(const QString& source, const QString& dest,
              ProgressCallback progress = {}, FinishedCallback finished = {});

    // 删除文件/目录
    void remove(const QStringList& paths,
                ProgressCallback progress = {}, FinishedCallback finished = {});

    // 创建目录
    void mkdir(const QString& path, bool createParents = true,
               FinishedCallback finished = {});

    // 重命名
    void rename(const QString& source, const QString& newName,
                FinishedCallback finished = {});

    // 取消当前操作
    void cancel();

    // 检查是否有正在进行的操作
    bool isBusy() const;

    // 获取目录大小（用于预估）
    static qint64 directorySize(const QString& path);
    static int countFiles(const QString& path);

signals:
    void progressUpdated(const FileOperationProgress& progress);
    void operationFinished(const FileOperationResult& result);
    void operationStarted(const QString& operation);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::core