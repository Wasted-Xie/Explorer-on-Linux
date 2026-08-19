#pragma once

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QMimeDatabase>
#include <functional>
#include <optional>

namespace explorer::utils {

// 文件工具
class FileUtils {
public:
    // 读取/写入文件
    static bool readFile(const QString& path, QString& content);
    static bool readFile(const QString& path, QByteArray& content);
    static bool writeFile(const QString& path, const QString& content);
    static bool writeFile(const QString& path, const QByteArray& content);
    static bool appendFile(const QString& path, const QString& content);
    static bool appendFile(const QString& path, const QByteArray& content);

    // 复制/移动/删除
    static bool copyFile(const QString& src, const QString& dest, bool overwrite = true);
    static bool moveFile(const QString& src, const QString& dest);
    static bool deleteFile(const QString& path);
    static bool deleteDir(const QString& path, bool recursive = true);

    // 目录操作
    static bool createDir(const QString& path, bool createParents = true);
    static bool dirExists(const QString& path);
    static QStringList listFiles(const QString& path, const QStringList& filters = {}, bool recursive = false);
    static QStringList listDirs(const QString& path, bool recursive = false);
    static qint64 dirSize(const QString& path);
    static int countFiles(const QString& path, bool recursive = true);

    // 文件信息
    static qint64 fileSize(const QString& path);
    static QDateTime lastModified(const QString& path);
    static QString mimeType(const QString& path);
    static QString fileType(const QString& path); // file, dir, symlink, etc.
    static QString permissions(const QString& path);
    static QString owner(const QString& path);

    // 搜索
    static QStringList findFiles(const QString& path, const QString& pattern, bool recursive = true);
    static QStringList findFiles(const QString& path, const QRegularExpression& regex, bool recursive = true);

    // 临时文件
    static QString createTempFile(const QString& prefix = "tmp", const QString& suffix = "");
    static QString createTempDir(const QString& prefix = "tmp");
    static bool removeTempFile(const QString& path);

    // 文件监视
    class FileWatcher {
    public:
        using ChangeCallback = std::function<void(const QString& path)>;
        FileWatcher();
        ~FileWatcher();
        bool addPath(const QString& path);
        void removePath(const QString& path);
        void setCallback(ChangeCallback cb);

    private:
        class Impl;
        std::unique_ptr<Impl> d;
    };

    // 文件锁
    class FileLock {
    public:
        explicit FileLock(const QString& path);
        ~FileLock();
        bool tryLock(int timeoutMs = 0);
        void unlock();
        bool isLocked() const;

    private:
        QString m_lockPath;
        int m_fd = -1;
        bool m_locked = false;
    };
};

} // namespace explorer::utils