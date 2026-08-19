#include "FileSystem.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include <QMimeDatabase>
#include <QProcess>
#include <QDebug>

namespace explorer::core {

QString FileSystem::normalizePath(const QString& path) {
    return QDir::cleanPath(path);
}

QString FileSystem::absolutePath(const QString& path) {
    return QFileInfo(path).absoluteFilePath();
}

QString FileSystem::relativePath(const QString& path, const QString& base) {
    return QDir(base).relativeFilePath(path);
}

QString FileSystem::parentPath(const QString& path) {
    return QFileInfo(path).absolutePath();
}

QString FileSystem::fileName(const QString& path) {
    return QFileInfo(path).fileName();
}

QString FileSystem::baseName(const QString& path) {
    return QFileInfo(path).baseName();
}

QString FileSystem::suffix(const QString& path) {
    return QFileInfo(path).suffix();
}

QString FileSystem::completeSuffix(const QString& path) {
    return QFileInfo(path).completeSuffix();
}

QString FileSystem::joinPath(const QString& base, const QString& relative) {
    return QDir(base).filePath(relative);
}

QString FileSystem::joinPaths(const QStringList& parts) {
    QString result;
    for (const QString& part : parts) {
        if (result.isEmpty()) {
            result = part;
        } else {
            result = QDir(result).filePath(part);
        }
    }
    return QDir::cleanPath(result);
}

bool FileSystem::exists(const QString& path) {
    return QFileInfo::exists(path);
}

bool FileSystem::isFile(const QString& path) {
    return QFileInfo(path).isFile();
}

bool FileSystem::isDirectory(const QString& path) {
    return QFileInfo(path).isDir();
}

bool FileSystem::isSymLink(const QString& path) {
    return QFileInfo(path).isSymLink();
}

bool FileSystem::isReadable(const QString& path) {
    return QFileInfo(path).isReadable();
}

bool FileSystem::isWritable(const QString& path) {
    return QFileInfo(path).isWritable();
}

bool FileSystem::isExecutable(const QString& path) {
    return QFileInfo(path).isExecutable();
}

bool FileSystem::isHidden(const QString& path) {
    return QFileInfo(path).isHidden();
}

qint64 FileSystem::size(const QString& path) {
    return QFileInfo(path).size();
}

QDateTime FileSystem::lastModified(const QString& path) {
    return QFileInfo(path).lastModified();
}

QDateTime FileSystem::lastRead(const QString& path) {
    return QFileInfo(path).lastRead();
}

QDateTime FileSystem::created(const QString& path) {
    return QFileInfo(path).birthTime();
}

QString FileSystem::owner(const QString& path) {
    return QFileInfo(path).owner();
}

QString FileSystem::group(const QString& path) {
    return QFileInfo(path).group();
}

QString FileSystem::permissions(const QString& path) {
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

QString FileSystem::mimeType(const QString& path) {
    static QMimeDatabase mimeDb;
    return mimeDb.mimeTypeForFile(path).name();
}

QStringList FileSystem::listDir(const QString& path, QDir::Filters filters) {
    QDir dir(path);
    dir.setFilter(filters);
    return dir.entryList();
}

QStringList FileSystem::listFiles(const QString& path, const QStringList& nameFilters) {
    QDir dir(path);
    if (!nameFilters.isEmpty()) {
        dir.setNameFilters(nameFilters);
    }
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    return dir.entryList();
}

QStringList FileSystem::listDirs(const QString& path) {
    QDir dir(path);
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    return dir.entryList();
}

QStorageInfo FileSystem::storageInfo(const QString& path) {
    return QStorageInfo(path);
}

qint64 FileSystem::freeSpace(const QString& path) {
    return QStorageInfo(path).bytesFree();
}

qint64 FileSystem::totalSpace(const QString& path) {
    return QStorageInfo(path).bytesTotal();
}

QString FileSystem::homePath() {
    return QDir::homePath();
}

QString FileSystem::tempPath() {
    return QDir::tempPath();
}

QString FileSystem::desktopPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

QString FileSystem::documentsPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString FileSystem::downloadsPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString FileSystem::musicPath() {
    return QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
}

QString FileSystem::picturesPath() {
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

QString FileSystem::videosPath() {
    return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
}

QString FileSystem::applicationsPath() {
    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
}

QString FileSystem::configPath() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
}

QString FileSystem::cachePath() {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString FileSystem::dataPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString FileSystem::runtimePath() {
    return QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
}

QString FileSystem::defaultApplication(const QString& mimeType) {
    // 使用 xdg-mime 查询默认应用
    QProcess process;
    process.start("xdg-mime", {"query", "default", mimeType});
    process.waitForFinished(3000);
    QString output = process.readAllStandardOutput().trimmed();
    return output.isEmpty() ? QString() : output;
}

QStringList FileSystem::applicationsForMimeType(const QString& mimeType) {
    // 使用 xdg-mime 查询所有支持该 MIME 类型的应用
    QProcess process;
    process.start("xdg-mime", {"query", "default", mimeType});
    process.waitForFinished(3000);
    QString output = process.readAllStandardOutput().trimmed();
    if (output.isEmpty()) return {};

    // 简单实现：返回单个默认应用
    return {output};
}

} // namespace explorer::core