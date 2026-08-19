#include "FileSystemModel.h"
#include <QDir>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QDebug>
#include <algorithm>

namespace explorer::core {

FileSystemItem::FileSystemItem(const QFileInfo& info)
    : id(info.absoluteFilePath())
    , name(info.fileName())
    , filePath(info.absoluteFilePath())
    , isDirectory(info.isDir())
    , isSymLink(info.isSymLink())
    , size(info.size())
    , modifiedTime(info.lastModified()) {
    // MIME 类型
    static QMimeDatabase mimeDb;
    mimeType = mimeDb.mimeTypeForFile(info).name();

    // 图标
    static QFileIconProvider iconProvider;
    icon = iconProvider.icon(info).name();
}

class FileSystemModel::Impl {
public:
    Impl(FileSystemModel* owner) : q(owner) {
        rootItem = std::make_unique<FileSystemItem>();
        rootItem->id = "";
        rootItem->name = "根目录";
        rootItem->filePath = "";
        rootItem->isDirectory = true;
        
        // 设置图标提供者
        iconProvider = std::make_unique<QFileIconProvider>();
        
        // 初始时不加载任何目录
    }
    
    // 获取项
    FileSystemItem* getItem(const QModelIndex& index) const {
        if (!index.isValid()) {
            return rootItem.get();
        }
        return static_cast<FileSystemItem*>(index.internalPointer());
    }
    
    // 根据路径查找项的行号
    int rowForPath(const QString& path, const QString& parentPath) const {
        QDir dir(parentPath);
        QStringList entries = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        QString fileName = QFileInfo(path).fileName();
        
        // 排序方式与Qt默认一致：先目录后文件，然后按名称排序
        std::sort(entries.begin(), entries.end(), [](const QString& a, const QString& b) {
            QFileInfo infoA(a);
            QFileInfo infoB(b);
            bool isDirA = infoA.isDir();
            bool isDirB = infoB.isDir();
            if (isDirA != isDirB) return isDirA; // 目录在前
            return a.localeAwareCompare(b) < 0;  // 然后按名称排序
        });
        
        auto it = std::find(entries.begin(), entries.end(), fileName);
        if (it != entries.end()) {
            return std::distance(entries.begin(), it);
        }
        return -1; // 未找到
    }
    
    FileSystemModel* q;
    std::unique_ptr<FileSystemItem> rootItem;
    QString m_rootPath;
    std::unique_ptr<QFileIconProvider> iconProvider;
};

FileSystemModel::FileSystemModel(QObject* parent) : QAbstractItemModel(parent), d(std::make_unique<Impl>(this)) {}
FileSystemModel::~FileSystemModel() = default;

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    FileSystemItem* parentItem = d->getItem(parent);

    if (!parentItem || !parentItem->isDirectory)
        return QModelIndex();

    QDir dir(parentItem->filePath);
    if (!dir.exists())
        return QModelIndex();

    QStringList entries = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    
    // 排序方式：目录在前，文件在后；同类按名称排序
    std::sort(entries.begin(), entries.end(), [](const QString& a, const QString& b) {
        QFileInfo infoA(dir.absoluteFilePath(a));
        QFileInfo infoB(dir.absoluteFilePath(b));
        bool isDirA = infoA.isDir();
        bool isDirB = infoB.isDir();
        if (isDirA != isDirB) return isDirA; // 目录在前
        return a.localeAwareCompare(b) < 0;  // 然后按名称排序
    });

    if (row < 0 || row >= entries.size())
        return QModelIndex();

    QString fileName = entries[row];
    QString absolutePath = dir.absoluteFilePath(fileName);
    QFileInfo info(absolutePath);
    
    // 创建新项
    auto* childItem = new FileSystemItem(info);
    // 将项的所有权交给模型（通过internalPointer管理）
    // 注意：这里存在内存管理问题，实际实现中需要更好的内存管理
    // 为了简单起见，我们这里假设项会被适当管理
    
    return createIndex(row, column, childItem);
}

QModelIndex FileSystemModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();

    FileSystemItem* childItem = d->getItem(index);
    if (!childItem || childItem->filePath.isEmpty())
        return QModelIndex();

    QString parentPath = QFileInfo(childItem->filePath).absolutePath();
    if (parentPath == d->m_rootPath && d->m_rootPath.isEmpty()) {
        // 根目录的父级是无效索引
        return QModelIndex();
    }

    // 查找父项在其父目录中的行号
    QDir dir(parentPath);
    QString grandParentPath = dir.absolutePath("..");
    if (grandParentPath == parentPath) {
        // 已经是文件系统根目录
        return QModelIndex();
    }
    
    QString parentFileName = QFileInfo(parentPath).fileName();
    int parentRow = d->rowForPath(parentPath, grandParentPath);
    
    if (parentRow < 0)
        return QModelIndex();

    // 创建父项
    QFileInfo parentInfo(parentPath);
    FileSystemItem* parentItem = new FileSystemItem(parentInfo);
    
    return createIndex(parentRow, 0, parentItem);
}

int FileSystemModel::rowCount(const QModelIndex& parent) const {
    FileSystemItem* parentItem = d->getItem(parent);
    if (!parentItem || !parentItem->isDirectory)
        return 0;

    QDir dir(parentItem->filePath);
    if (!dir.exists())
        return 0;

    return dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System).size();
}

int FileSystemModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return 4; // 名称, 大小, 类型, 修改日期
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    FileSystemItem* item = d->getItem(index);
    if (!item)
        return QVariant();

    switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
                case 0: // 名称
                    return item->name.isEmpty() ? item->filePath : item->name;
                case 1: // 大小
                    if (item->isDirectory)
                        return QString("<DIR>");
                    return QString::number(item->size);
                case 2: // 类型
                    if (item->isDirectory)
                        return QString("文件夹");
                    return item->mimeType;
                case 3: // 修改日期
                    return item->modifiedTime.toString("yyyy-MM-dd hh:mm");
                default:
                    return QVariant();
            }
        case Qt::DecorationRole:
            if (index.column() == 0) { // 只在名称列显示图标
                return QIcon::fromTheme(item->icon);
            }
            break;
        case Qt::SizeHintRole:
            return QSize(-1, 20); // 默认行高
        case Qt::TextAlignmentRole:
            if (index.column() == 1) { // 大小列右对齐
                return int(Qt::AlignRight | Qt::AlignVCenter);
            }
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        default:
            break;
    }
    
    return QVariant();
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return QString("名称");
            case 1: return QString("大小");
            case 2: return QString("类型");
            case 3: return QString("修改日期");
            default: return QVariant();
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

void FileSystemModel::setRootPath(const QString& path) {
    beginResetModel();
    d->m_rootPath = QDir::cleanPath(path);
    endResetModel();
    
    emit rootPathChanged(d->m_rootPath);
}

QString FileSystemModel::rootPath() const {
    return d->m_rootPath;
}

QModelIndex FileSystemModel::indexForPath(const QString& path) const {
    if (path.isEmpty() || !QDir::exists(path))
        return QModelIndex();
    
    QFileInfo info(path);
    if (!info.exists())
        return QModelIndex();
    
    // 查找父目录
    QString parentPath = info.absolutePath();
    QString fileName = info.fileName();
    
    QModelIndex parentIndex = indexForPath(parentPath);
    if (!parentIndex.isValid() && !parentPath.isEmpty()) {
        // 尝试获取根目录索引
        parentIndex = index(0, 0, QModelIndex());
        if (!parentIndex.isValid())
            return QModelIndex();
    }
    
    // 在父目录中查找此文件的行号
    int row = d->rowForPath(path, parentPath);
    if (row < 0)
        return QModelIndex();
        
    return index(row, 0, parentIndex);
}

QString FileSystemModel::filePath(const QModelIndex& index) const {
    if (!index.isValid())
        return QString();
        
    FileSystemItem* item = d->getItem(index);
    return item ? item->filePath : QString();
}

bool FileSystemModel::isDir(const QModelIndex& index) const {
    if (!index.isValid())
        return false;
        
    FileSystemItem* item = d->getItem(index);
    return item && item->isDirectory;
}

void FileSystemModel::refresh() {
    if (!d->m_rootPath.isEmpty()) {
        setRootPath(d->m_rootPath);
    }
}

void FileSystemModel::refresh(const QString& path) {
    setRootPath(path);
}

bool FileSystemModel::mkdir(const QString& path, const QString& name) {
    QDir dir(path);
    if (!dir.exists())
        return false;
        
    if (dir.mkdir(name)) {
        // 刷新父目录
        QString parentPath = QDir::cleanPath(path);
        beginResetModel();
        endResetModel();
        return true;
    }
    return false;
}

bool FileSystemModel::remove(const QString& path) {
    QFileInfo info(path);
    if (!info.exists())
        return false;
        
    bool success = false;
    if (info.isDir()) {
        QDir dir(path);
        success = dir.removeRecursively();
    } else {
        success = QFile::remove(path);
    }
    
    if (success) {
        // 刷新父目录
        QString parentPath = info.absolutePath();
        beginResetModel();
        endResetModel();
    }
    return success;
}

bool FileSystemModel::rename(const QString& path, const QString& newName) {
    QFileInfo info(path);
    if (!info.exists())
        return false;
        
    QString newPath = info.absolutePath() + "/" + newName;
    bool success = QFile::rename(path, newPath);
    
    if (success) {
        // 刷新涉及的目录
        QString parentPath = info.absolutePath();
        beginResetModel();
        endResetModel();
    }
    return success;
}

} // namespace explorer::core

#include "FileSystemModel.moc"