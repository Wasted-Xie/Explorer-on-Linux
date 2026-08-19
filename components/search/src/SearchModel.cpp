#include "SearchModel.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QUrl>
#include <algorithm>

namespace explorer::search {

SearchModel::SearchModel(QObject* parent)
    : QAbstractListModel(parent) {
    // 设置防抖动定时器
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(DEBOUNCE_MS);
    connect(&m_debounceTimer, &QTimer::timeout, this, &SearchModel::onDebouncedSearch);
}

SearchModel::~SearchModel() = default;

int SearchModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_results.size();
}

QVariant SearchModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_results.size()) {
        return QVariant();
    }

    const SearchResult& result = m_results[index.row()];

    switch (role) {
        case IdRole: return result.id;
        case NameRole: return result.name;
        case DescriptionRole: return result.description;
        case ExecPathRole: return result.execPath;
        case IconRole: return result.icon;
        case TypeRole: return static_cast<int>(result.type);
        case ScoreRole: return result.score;
        case UserDataRole: return result.userData;
        default: return QVariant();
    }
}

QHash<int, QByteArray> SearchModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[ExecPathRole] = "execPath";
    roles[IconRole] = "icon";
    roles[TypeRole] = "type";
    roles[ScoreRole] = "score";
    roles[UserDataRole] = "userData";
    return roles;
}

void SearchModel::setQuery(const QString& query) {
    if (m_query == query) return;

    m_query = query;
    emit queryChanged(query);

    // 重置防抖动定时器
    m_debounceTimer.start();
}

QString SearchModel::query() const {
    return m_query;
}

void SearchModel::clearResults() {
    beginResetModel();
    m_results.clear();
    endResetModel();
}

void SearchModel::addResult(const SearchResult& result) {
    if (!result.isValid()) return;

    int row = m_results.size();
    beginInsertRows(QModelIndex(), row, row);
    m_results.append(result);
    endInsertRows();
}

void SearchModel::addResults(const QVector<SearchResult>& results) {
    if (results.isEmpty()) return;

    int startRow = m_results.size();
    int endRow = startRow + results.size() - 1;
    beginInsertRows(QModelIndex(), startRow, endRow);
    m_results.append(results);
    endInsertRows();
}

SearchResult SearchModel::resultAt(int row) const {
    if (row >= 0 && row < m_results.size()) {
        return m_results[row];
    }
    return SearchResult();
}

QVector<SearchResult> SearchModel::results() const {
    return m_results;
}

bool SearchModel::launchResult(int row) {
    if (row < 0 || row >= m_results.size()) {
        return false;
    }

    const SearchResult& result = m_results[row];
    bool success = false;

    try {
        if (result.type == ResultType::Application) {
            // 启动应用程序
            QString exec = result.execPath;
            // 处理 Exec 字段中的参数占位符 (%f, %u, %F, %U 等)
            exec.remove(QRegularExpression("%[fFuU]"));
            exec = exec.trimmed();

            success = QProcess::startDetached(exec);
        } else {
            // 打开文件/文件夹
            success = QDesktopServices::openUrl(QUrl::fromLocalFile(result.execPath));
        }
    } catch (...) {
        success = false;
    }

    emit resultLaunched(result.execPath, success);
    return success;
}

void SearchModel::onDebouncedSearch() {
    performSearch();
}

void SearchModel::performSearch() {
    emit searchStarted();

    QVector<SearchResult> results;

    if (m_query.isEmpty()) {
        // 空查询时清空结果
        clearResults();
        emit searchFinished(0);
        return;
    }

    // 搜索应用程序
    searchApplications(m_query, results);

    // 搜索文件
    searchFiles(m_query, results);

    // 按分数排序
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;
              });

    // 限制结果数量
    if (results.size() > MAX_RESULTS) {
        results.resize(MAX_RESULTS);
    }

    // 更新模型
    beginResetModel();
    m_results = std::move(results);
    endResetModel();

    emit searchFinished(m_results.size());
}

void SearchModel::searchApplications(const QString& query, QVector<SearchResult>& results) {
    QStringList desktopPaths = getDesktopFilePaths();
    QString lowerQuery = query.toLower();

    for (const QString& desktopPath : desktopPaths) {
        SearchResult result = parseDesktopFile(desktopPath);
        if (!result.isValid()) continue;

        // 计算匹配分数
        QString nameLower = result.name.toLower();
        QString descLower = result.description.toLower();

        double score = 0.0;
        if (nameLower.contains(lowerQuery)) {
            // 名称匹配权重更高
            score += 100.0;
            if (nameLower.startsWith(lowerQuery)) {
                score += 50.0; // 前缀匹配加分
            }
        }
        if (descLower.contains(lowerQuery)) {
            score += 30.0;
        }

        if (score > 0) {
            result.score = score;
            results.append(result);
        }
    }
}

void SearchModel::searchFiles(const QString& query, QVector<SearchResult>& results) {
    QString homePath = QDir::homePath();
    QStringList searchPaths = {
        homePath,
        QDir(homePath).filePath("Desktop"),
        QDir(homePath).filePath("Documents"),
        QDir(homePath).filePath("Downloads"),
        QDir(homePath).filePath("Pictures"),
        QDir(homePath).filePath("Music"),
        QDir(homePath).filePath("Videos")
    };

    QString lowerQuery = query.toLower();
    QRegularExpression regex(QRegularExpression::wildcardToRegularExpression("*" + query + "*"),
                             QRegularExpression::CaseInsensitiveOption);

    for (const QString& searchPath : searchPaths) {
        QDir dir(searchPath);
        if (!dir.exists()) continue;

        QStringList files = explorer::utils::FileUtils::listFiles(searchPath, {}, true);

        for (const QString& filePath : files) {
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.fileName();
            QString fileNameLower = fileName.toLower();

            double score = 0.0;
            if (fileNameLower.contains(lowerQuery)) {
                score += 50.0;
                if (fileNameLower.startsWith(lowerQuery)) {
                    score += 30.0;
                }
            }

            // 目录稍微加权
            if (fileInfo.isDir()) {
                score += 5.0;
            }

            if (score > 0) {
                SearchResult result;
                result.id = "file://" + filePath;
                result.name = fileName;
                result.description = filePath;
                result.execPath = filePath;
                result.icon = getIconForFile(filePath);
                result.type = fileInfo.isDir() ? ResultType::Folder : ResultType::File;
                result.score = score;
                results.append(result);
            }
        }
    }
}

QStringList SearchModel::getDesktopFilePaths() const {
    QStringList paths;
    QStringList dirs = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        QDir::homePath() + "/.local/share/applications"
    };

    for (const QString& dirPath : dirs) {
        QDir dir(dirPath);
        if (dir.exists()) {
            QStringList filters = {"*.desktop"};
            QStringList files = dir.entryList(filters, QDir::Files);
            for (const QString& file : files) {
                paths.append(dir.filePath(file));
            }
        }
    }

    return paths;
}

SearchResult SearchModel::parseDesktopFile(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return SearchResult();
    }

    QString name, exec, genericName, comment, iconName;
    bool noDisplay = false;
    bool terminal = false;

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();

        if (line.startsWith("Name=") && !line.startsWith("Name[")) {
            name = line.mid(5);
        } else if (line.startsWith("Exec=")) {
            exec = line.mid(5);
        } else if (line.startsWith("GenericName=") && !line.startsWith("GenericName[")) {
            genericName = line.mid(12);
        } else if (line.startsWith("Comment=") && !line.startsWith("Comment[")) {
            comment = line.mid(8);
        } else if (line.startsWith("Icon=")) {
            iconName = line.mid(5);
        } else if (line.startsWith("NoDisplay=")) {
            noDisplay = line.mid(10).toLower() == "true";
        } else if (line.startsWith("Terminal=")) {
            terminal = line.mid(9).toLower() == "true";
        }
    }

    if (name.isEmpty() || exec.isEmpty() || noDisplay) {
        return SearchResult();
    }

    // 构建描述
    QString description;
    if (!genericName.isEmpty()) {
        description = genericName;
        if (!comment.isEmpty()) {
            description += " - " + comment;
        }
    } else if (!comment.isEmpty()) {
        description = comment;
    }

    // 获取图标
    QIcon icon;
    if (!iconName.isEmpty()) {
        icon = QIcon::fromTheme(iconName);
    }
    if (icon.isNull()) {
        icon = QIcon::fromTheme("application-x-executable");
    }

    SearchResult result;
    result.id = "app://" + filePath;
    result.name = name;
    result.description = description;
    result.execPath = exec;
    result.icon = icon;
    result.type = ResultType::Application;
    result.score = 0.0;

    return result;
}

QIcon SearchModel::getIconForFile(const QString& filePath) const {
    QFileInfo fileInfo(filePath);

    if (fileInfo.isDir()) {
        return QIcon::fromTheme("folder");
    }

    QString mimeType = m_mimeDb.mimeTypeForFile(filePath).name();
    QString iconName = QIcon::themeIconNameForMime(mimeType);

    if (iconName.isEmpty()) {
        // 根据扩展名猜测图标
        QString suffix = fileInfo.suffix().toLower();
        if (suffix == "txt" || suffix == "md" || suffix == "log") {
            iconName = "text-x-generic";
        } else if (suffix == "pdf") {
            iconName = "application-pdf";
        } else if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "gif") {
            iconName = "image-x-generic";
        } else if (suffix == "mp3" || suffix == "wav" || suffix == "ogg" || suffix == "flac") {
            iconName = "audio-x-generic";
        } else if (suffix == "mp4" || suffix == "mkv" || suffix == "avi" || suffix == "mov") {
            iconName = "video-x-generic";
        } else if (suffix == "zip" || suffix == "tar" || suffix == "gz" || suffix == "7z") {
            iconName = "package-x-generic";
        } else {
            iconName = "text-x-generic";
        }
    }

    QIcon icon = QIcon::fromTheme(iconName);
    if (icon.isNull()) {
        icon = QIcon::fromTheme("text-x-generic");
    }
    return icon;
}

double SearchModel::calculateScore(const QString& query, const QString& text) const {
    QString q = query.toLower();
    QString t = text.toLower();

    if (t == q) return 1000.0; // 完全匹配
    if (t.startsWith(q)) return 500.0; // 前缀匹配
    if (t.contains(q)) return 100.0; // 包含匹配

    return 0.0;
}

} // namespace explorer::search

#include "SearchModel.moc"