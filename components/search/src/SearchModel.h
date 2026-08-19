#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QTimer>
#include <QString>
#include <QIcon>
#include <QMimeDatabase>

#include "SearchResult.h"
#include <explorer/utils/FileUtils.h>

namespace explorer::search {

// 搜索模型 - 管理搜索结果列表
class SearchModel : public QAbstractListModel {
    Q_OBJECT

public:
    // 角色定义
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        ExecPathRole,
        IconRole,
        TypeRole,
        ScoreRole,
        UserDataRole
    };

    explicit SearchModel(QObject* parent = nullptr);
    ~SearchModel() override;

    // QAbstractItemModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 搜索相关
    void setQuery(const QString& query);
    QString query() const;

    // 结果操作
    void clearResults();
    void addResult(const SearchResult& result);
    void addResults(const QVector<SearchResult>& results);
    SearchResult resultAt(int row) const;
    QVector<SearchResult> results() const;

    // 启动选中的项目
    bool launchResult(int row);

signals:
    void queryChanged(const QString& query);
    void searchStarted();
    void searchFinished(int resultCount);
    void resultLaunched(const QString& execPath, bool success);

private slots:
    void onDebouncedSearch();

private:
    void performSearch();
    void searchApplications(const QString& query, QVector<SearchResult>& results);
    void searchFiles(const QString& query, QVector<SearchResult>& results);
    QStringList getDesktopFilePaths() const;
    SearchResult parseDesktopFile(const QString& filePath) const;
    QIcon getIconForFile(const QString& filePath) const;
    double calculateScore(const QString& query, const QString& text) const;

    QString m_query;
    QVector<SearchResult> m_results;
    QTimer m_debounceTimer;
    QMimeDatabase m_mimeDb;
    static constexpr int DEBOUNCE_MS = 300;
    static constexpr int MAX_RESULTS = 50;
    static constexpr int MAX_FILE_DEPTH = 3;
};

} // namespace explorer::search