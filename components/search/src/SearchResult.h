#pragma once

#include <QString>
#include <QIcon>
#include <QVariant>

namespace explorer::search {

// 搜索结果项类型
enum class ResultType {
    Application,  // 应用程序 (.desktop 文件)
    File,         // 文件
    Folder        // 文件夹
};

// 搜索结果项
struct SearchResult {
    QString id;              // 唯一标识符
    QString name;            // 显示名称
    QString description;     // 描述 (应用: GenericName/Comment, 文件: 路径)
    QString execPath;        // 执行路径 (应用: Exec 命令, 文件/文件夹: 文件路径)
    QIcon icon;              // 图标
    ResultType type;         // 结果类型
    double score = 0.0;      // 匹配分数 (用于排序)
    QVariant userData;       // 用户数据

    // 默认构造
    SearchResult() = default;

    // 构造函数
    SearchResult(const QString& id_, const QString& name_, const QString& description_,
                 const QString& execPath_, const QIcon& icon_, ResultType type_, double score_ = 0.0)
        : id(id_), name(name_), description(description_), execPath(execPath_), icon(icon_), type(type_), score(score_) {}

    // 比较运算符 (按分数排序，分数高的在前)
    bool operator<(const SearchResult& other) const {
        return score > other.score; // 反转，因为优先队列默认是最大堆
    }

    // 检查是否有效
    bool isValid() const {
        return !id.isEmpty() && !name.isEmpty() && !execPath.isEmpty();
    }

    // 获取类型的字符串表示
    static QString typeToString(ResultType type) {
        switch (type) {
            case ResultType::Application: return "Application";
            case ResultType::File: return "File";
            case ResultType::Folder: return "Folder";
        }
        return "Unknown";
    }
};

} // namespace explorer::search