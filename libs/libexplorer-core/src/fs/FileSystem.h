#pragma once

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include <QMimeDatabase>
#include <functional>
#include <optional>

namespace explorer::core {

// 文件系统工具类 - 静态工具函数
class FileSystem {
public:
    // 路径操作
    static QString normalizePath(const QString& path);
    static QString absolutePath(const QString& path);
    static QString relativePath(const QString& path, const QString& base);
    static QString parentPath(const QString& path);
    static QString fileName(const QString& path);
    static QString baseName(const QString& path);
    static QString suffix(const QString& path);
    static QString completeSuffix(const QString& path);

    // 路径拼接
    static QString joinPath(const QString& base, const QString& relative);
    static QString joinPaths(const QStringList& parts);

    // 文件/目录检查
    static bool exists(const QString& path);
    static bool isFile(const QString& path);
    static bool isDirectory(const QString& path);
    static bool isSymLink(const QString& path);
    static bool isReadable(const QString& path);
    static bool isWritable(const QString& path);
    static bool isExecutable(const QString& path);
    static bool isHidden(const QString& path);

    // 文件信息
    static qint64 size(const QString& path);
    static QDateTime lastModified(const QString& path);
    static QDateTime lastRead(const QString& path);
    static QDateTime created(const QString& path);
    static QString owner(const QString& path);
    static QString group(const QString& path);
    static QString permissions(const QString& path);
    static QString mimeType(const QString& path);

    // 目录操作
    static QStringList listDir(const QString& path, QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot);
    static QStringList listFiles(const QString& path, const QStringList& nameFilters = {});
    static QStringList listDirs(const QString& path);

    // 存储信息
    static QStorageInfo storageInfo(const QString& path);
    static qint64 freeSpace(const QString& path);
    static qint64 totalSpace(const QString& path);

    // 特殊路径
    static QString homePath();
    static QString tempPath();
    static QString desktopPath();
    static QString documentsPath();
    static QString downloadsPath();
    static QString musicPath();
    static QString picturesPath();
    static QString videosPath();
    static QString applicationsPath();
    static QString configPath();
    static QString cachePath();
    static QString dataPath();
    static QString runtimePath();

    // 文件关联
    static QString defaultApplication(const QString& mimeType);
    static QStringList applicationsForMimeType(const QString& mimeType);
};

} // namespace explorer::core