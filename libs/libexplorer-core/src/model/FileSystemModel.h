#pragma once

#include <QObject>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QSize>
#include <QDebug>
#include <memory>
#include <vector>
#include <algorithm>

namespace explorer::core {

// 文件系统模型项
struct FileSystemItem {
    QString id;              // 文件绝对路径
    QString name;            // 文件名
    QString filePath;        // 完整路径
    bool isDirectory = false;
    bool isSymLink = false;
    qint64 size = 0;
    QDateTime modifiedTime;
    QString mimeType;
    QString icon;            // 图标名称或路径
    
    FileSystemItem() = default;
    explicit FileSystemItem(const QFileInfo& info);
};

// 文件系统模型 - 继承自QAbstractItemModel以兼容Qt视图
class FileSystemModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit FileSystemModel(QObject* parent = nullptr);
    ~FileSystemModel() override;

    // QAbstractItemModel 接口实现
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 设置根路径
    void setRootPath(const QString& path);
    QString rootPath() const;

    // 获取路径对应的模型索引
    QModelIndex indexForPath(const QString& path) const;
    QString filePath(const QModelIndex& index) const;
    bool isDir(const QModelIndex& index) const;

    // 刷新
    void refresh();
    void refresh(const QString& path);

    // 文件操作（简化版）
    bool mkdir(const QString& path, const QString& name);
    bool remove(const QString& path);
    bool rename(const QString& path, const QString& newName);

signals:
    void rootPathChanged(const QString& path);
    void directoryLoaded(const QString& path);
    void fileError(const QString& path, const QString& error);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::core

#include "FileSystemModel.moc"