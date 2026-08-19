#include "FileUtils.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QWaitCondition>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

namespace explorer::utils {

bool FileUtils::readFile(const QString& path, QString& content) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    content = QString::fromUtf8(file.readAll());
    return true;
}

bool FileUtils::readFile(const QString& path, QByteArray& content) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    content = file.readAll();
    return true;
}

bool FileUtils::writeFile(const QString& path, const QString& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    file.write(content.toUtf8());
    return true;
}

bool FileUtils::writeFile(const QString& path, const QByteArray& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(content);
    return true;
}

bool FileUtils::appendFile(const QString& path, const QString& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) return false;
    file.write(content.toUtf8());
    return true;
}

bool FileUtils::appendFile(const QString& path, const QByteArray& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) return false;
    file.write(content);
    return true;
}

bool FileUtils::copyFile(const QString& src, const QString& dest, bool overwrite) {
    QFileInfo srcInfo(src);
    if (!srcInfo.exists() || !srcInfo.isFile()) return false;

    if (QFile::exists(dest)) {
        if (!overwrite) return false;
        QFile::remove(dest);
    }

    return QFile::copy(src, dest);
}

bool FileUtils::moveFile(const QString& src, const QString& dest) {
    if (QFile::rename(src, dest)) return true;

    // 跨文件系统：复制后删除
    if (copyFile(src, dest)) {
        return QFile::remove(src);
    }
    return false;
}

bool FileUtils::deleteFile(const QString& path) {
    return QFile::remove(path);
}

bool FileUtils::deleteDir(const QString& path, bool recursive) {
    QDir dir(path);
    if (!dir.exists()) return true;

    if (recursive) {
        return dir.removeRecursively();
    } else {
        return dir.removeRecursively(); // Qt 没有非递归删除目录的方法
    }
}

bool FileUtils::createDir(const QString& path, bool createParents) {
    QDir dir;
    return createParents ? dir.mkpath(path) : dir.mkdir(path);
}

bool FileUtils::dirExists(const QString& path) {
    return QDir(path).exists();
}

QStringList FileUtils::listFiles(const QString& path, const QStringList& filters, bool recursive) {
    QDir dir(path);
    if (!dir.exists()) return {};

    QDir::Filters filter = QDir::Files | QDir::NoDotAndDotDot;
    if (!filters.isEmpty()) {
        dir.setNameFilters(filters);
    }

    QStringList result = dir.entryList(filter);

    if (recursive) {
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& subdir : subdirs) {
            QString subPath = dir.filePath(subdir);
            QStringList subFiles = listFiles(subPath, filters, true);
            for (QString& file : subFiles) {
                file = subdir + "/" + file;
            }
            result += subFiles;
        }
    }

    return result;
}

QStringList FileUtils::listDirs(const QString& path, bool recursive) {
    QDir dir(path);
    if (!dir.exists()) return {};

    QStringList result = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (recursive) {
        for (const QString& subdir : result) {
            QString subPath = dir.filePath(subdir);
            QStringList subDirs = listDirs(subPath, true);
            for (QString& d : subDirs) {
                d = subdir + "/" + d;
            }
            result += subDirs;
        }
    }

    return result;
}

qint64 FileUtils::dirSize(const QString& path) {
    qint64 size = 0;
    QDir dir(path);
    if (!dir.exists()) return 0;

    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            size += dirSize(info.absoluteFilePath());
        } else {
            size += info.size();
        }
    }
    return size;
}

int FileUtils::countFiles(const QString& path, bool recursive) {
    int count = 0;
    QDir dir(path);
    if (!dir.exists()) return 0;

    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            if (recursive) {
                count += countFiles(info.absoluteFilePath(), true);
            }
        } else {
            count++;
        }
    }
    return count;
}

qint64 FileUtils::fileSize(const QString& path) {
    return QFileInfo(path).size();
}

QDateTime FileUtils::lastModified(const QString& path) {
    return QFileInfo(path).lastModified();
}

QString FileUtils::mimeType(const QString& path) {
    static QMimeDatabase mimeDb;
    return mimeDb.mimeTypeForFile(path).name();
}

QString FileUtils::fileType(const QString& path) {
    QFileInfo info(path);
    if (!info.exists()) return "none";
    if (info.isFile()) return "file";
    if (info.isDir()) return "dir";
    if (info.isSymLink()) return "symlink";
    return "unknown";
}

QString FileUtils::permissions(const QString& path) {
    QFileInfo info(path);
    QFile::Permissions perms = info.permissions();
    QString result;
    result += (perms & QFile::ReadOwner) ? "r" : "-";
    result += (perms & QFile::WriteOwner) ? "w" : "-";
    result += (perms & QFile::ExeOwner) ? "x" : "-";
    result += (perms & QFile::ReadGroup) ? "r" : "-";
    result += (perms & QFile::WriteGroup) ? "w" : "-";
    result += (perms & QFile::ExeGroup) ? "x" : "-";
    result += (perms & QFile::ReadOther) ? "r" : "-";
    result += (perms & QFile::WriteOther) ? "w" : "-";
    result += (perms & QFile::ExeOther) ? "x" : "-";
    return result;
}

QString FileUtils::owner(const QString& path) {
    return QFileInfo(path).owner();
}

QStringList FileUtils::findFiles(const QString& path, const QString& pattern, bool recursive) {
    QDir dir(path);
    if (!dir.exists()) return {};

    QStringList result;
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);

    for (const QFileInfo& entry : entries) {
        if (regex.match(entry.fileName()).hasMatch()) {
            result << entry.absoluteFilePath();
        }
        if (recursive && entry.isDir()) {
            QStringList subResults = findFiles(entry.absoluteFilePath(), pattern, true);
            result += subResults;
        }
    }
    return result;
}

QStringList FileUtils::findFiles(const QString& path, const QRegularExpression& regex, bool recursive) {
    QDir dir(path);
    if (!dir.exists()) return {};

    QStringList result;
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (const QFileInfo& entry : entries) {
        if (regex.match(entry.fileName()).hasMatch()) {
            result << entry.absoluteFilePath();
        }
        if (recursive && entry.isDir()) {
            QStringList subResults = findFiles(entry.absoluteFilePath(), regex, true);
            result += subResults;
        }
    }
    return result;
}

QString FileUtils::createTempFile(const QString& prefix, const QString& suffix) {
    QTemporaryFile file;
    file.setFileTemplate(QDir::tempPath() + "/" + prefix + "_XXXXXX" + suffix);
    if (file.open()) {
        QString path = file.fileName();
        file.close();
        return path;
    }
    return QString();
}

QString FileUtils::createTempDir(const QString& prefix) {
    QTemporaryDir dir;
    dir.setPath(QDir::tempPath() + "/" + prefix + "_XXXXXX");
    if (dir.isValid()) {
        return dir.path();
    }
    return QString();
}

bool FileUtils::removeTempFile(const QString& path) {
    return QFile::remove(path);
}

// FileWatcher 实现
class FileUtils::FileWatcher::Impl {
public:
    Impl() {
        connect(&watcher, &QFileSystemWatcher::fileChanged, this, &Impl::onFileChanged);
        connect(&watcher, &QFileSystemWatcher::directoryChanged, this, &Impl::onDirChanged);
    }

    void onFileChanged(const QString& path) {
        if (callback) callback(path);
    }

    void onDirChanged(const QString& path) {
        if (callback) callback(path);
    }

    QFileSystemWatcher watcher;
    ChangeCallback callback;
};

FileUtils::FileWatcher::FileWatcher() : d(std::make_unique<Impl>()) {}
FileUtils::FileWatcher::~FileWatcher() = default;

bool FileUtils::FileWatcher::addPath(const QString& path) {
    return d->watcher.addPath(path);
}

void FileUtils::FileWatcher::removePath(const QString& path) {
    d->watcher.removePath(path);
}

void FileUtils::FileWatcher::setCallback(ChangeCallback cb) {
    d->callback = std::move(cb);
}

// FileLock 实现
FileUtils::FileLock::FileLock(const QString& path) : m_lockPath(path + ".lock") {}

FileUtils::FileLock::~FileLock() {
    unlock();
}

bool FileUtils::FileLock::tryLock(int timeoutMs) {
    if (m_locked) return true;

    m_fd = open(m_lockPath.toUtf8().constData(), O_CREAT | O_RDWR, 0644);
    if (m_fd < 0) return false;

    int flags = LOCK_EX;
    if (timeoutMs == 0) {
        flags |= LOCK_NB;
    }

    // 简单实现：非阻塞尝试
    if (flock(m_fd, flags) == 0) {
        m_locked = true;
        return true;
    }

    close(m_fd);
    m_fd = -1;
    return false;
}

void FileUtils::FileLock::unlock() {
    if (m_locked && m_fd >= 0) {
        flock(m_fd, LOCK_UN);
        close(m_fd);
        m_fd = -1;
        m_locked = false;
        QFile::remove(m_lockPath);
    }
}

bool FileUtils::FileLock::isLocked() const {
    return m_locked;
}

} // namespace explorer::utils