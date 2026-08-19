#pragma once

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QChar>
#include <algorithm>
#include <cctype>

namespace explorer::utils {

// 字符串工具
class StringUtils {
public:
    // 大小写转换
    static QString toLower(const QString& str);
    static QString toUpper(const QString& str);
    static QString toTitleCase(const QString& str);
    static QString toCamelCase(const QString& str);
    static QString toSnakeCase(const QString& str);
    static QString toKebabCase(const QString& str);

    // 修剪
    static QString trim(const QString& str);
    static QString trimLeft(const QString& str);
    static QString trimRight(const QString& str);

    // 截断
    static QString truncate(const QString& str, int maxLength, const QString& suffix = "...");
    static QString truncateWords(const QString& str, int maxWords, const QString& suffix = "...");

    // 填充
    static QString padLeft(const QString& str, int length, QChar padChar = ' ');
    static QString padRight(const QString& str, int length, QChar padChar = ' ');

    // 替换
    static QString replaceAll(const QString& str, const QString& from, const QString& to);
    static QString replaceFirst(const QString& str, const QString& from, const QString& to);
    static QString replaceLast(const QString& str, const QString& from, const QString& to);

    // 分割/连接
    static QStringList split(const QString& str, const QString& separator, Qt::SplitBehavior behavior = Qt::KeepEmptyParts);
    static QString join(const QStringList& list, const QString& separator);
    static QString join(const QVector<QString>& list, const QString& separator);

    // 匹配
    static bool startsWith(const QString& str, const QString& prefix, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    static bool endsWith(const QString& str, const QString& suffix, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    static bool contains(const QString& str, const QString& substr, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    static bool matches(const QString& str, const QRegularExpression& regex);
    static QStringList matchAll(const QString& str, const QRegularExpression& regex);

    // 编码/解码
    static QString htmlEscape(const QString& str);
    static QString htmlUnescape(const QString& str);
    static QString urlEncode(const QString& str);
    static QString urlDecode(const QString& str);
    static QString base64Encode(const QString& str);
    static QString base64Decode(const QString& str);

    // 路径相关
    static QString normalizePath(const QString& path);
    static QString getExtension(const QString& path);
    static QString getBaseName(const QString& path);
    static QString getDirName(const QString& path);
    static QString joinPath(const QString& base, const QString& relative);

    // 格式化
    static QString formatBytes(qint64 bytes, int precision = 2);
    static QString formatDuration(qint64 milliseconds);
    static QString formatNumber(qint64 number, const QString& thousandSep = ",");
    static QString formatPercent(double value, int precision = 1);

    // 验证
    static bool isEmpty(const QString& str);
    static bool isBlank(const QString& str);
    static bool isNumeric(const QString& str);
    static bool isEmail(const QString& str);
    static bool isUrl(const QString& str);
    static bool isIpAddress(const QString& str);
    static bool isUuid(const QString& str);

    // 生成
    static QString generateUuid();
    static QString generateRandomString(int length, const QString& charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    static QString slugify(const QString& str);

    // 相似度
    static double similarity(const QString& a, const QString& b);
    static int levenshteinDistance(const QString& a, const QString& b);
};

} // namespace explorer::utils