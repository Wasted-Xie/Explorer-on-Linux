#include "StringUtils.h"
#include <QRegularExpression>
#include <QUrl>
#include <QByteArray>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>

namespace explorer::utils {

QString StringUtils::toLower(const QString& str) {
    return str.toLower();
}

QString StringUtils::toUpper(const QString& str) {
    return str.toUpper();
}

QString StringUtils::toTitleCase(const QString& str) {
    QStringList words = str.split(QRegularExpression(R"(\s+|_|-)"), Qt::SkipEmptyParts);
    for (QString& word : words) {
        if (!word.isEmpty()) {
            word[0] = word[0].toUpper();
            if (word.length() > 1) {
                word = word[0] + word.mid(1).toLower();
            }
        }
    }
    return words.join(" ");
}

QString StringUtils::toCamelCase(const QString& str) {
    QStringList words = str.split(QRegularExpression(R"(\s+|_|-)"), Qt::SkipEmptyParts);
    if (words.isEmpty()) return QString();

    QString result = words.first().toLower();
    for (int i = 1; i < words.size(); ++i) {
        QString word = words[i].toLower();
        if (!word.isEmpty()) {
            word[0] = word[0].toUpper();
            result += word;
        }
    }
    return result;
}

QString StringUtils::toSnakeCase(const QString& str) {
    QString result = str;
    // 处理驼峰命名
    result.replace(QRegularExpression(R"(([a-z0-9])([A-Z]))"), "\\1_\\2");
    // 替换空格和连字符
    result.replace(QRegularExpression(R"([\s-]+)"), "_");
    return result.toLower();
}

QString StringUtils::toKebabCase(const QString& str) {
    return toSnakeCase(str).replace('_', '-');
}

QString StringUtils::trim(const QString& str) {
    return str.trimmed();
}

QString StringUtils::trimLeft(const QString& str) {
    int i = 0;
    while (i < str.length() && str[i].isSpace()) ++i;
    return str.mid(i);
}

QString StringUtils::trimRight(const QString& str) {
    int i = str.length() - 1;
    while (i >= 0 && str[i].isSpace()) --i;
    return str.left(i + 1);
}

QString StringUtils::truncate(const QString& str, int maxLength, const QString& suffix) {
    if (str.length() <= maxLength) return str;
    if (maxLength <= suffix.length()) return suffix.left(maxLength);
    return str.left(maxLength - suffix.length()) + suffix;
}

QString StringUtils::truncateWords(const QString& str, int maxWords, const QString& suffix) {
    QStringList words = str.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
    if (words.size() <= maxWords) return str;
    return words.mid(0, maxWords).join(" ") + suffix;
}

QString StringUtils::padLeft(const QString& str, int length, QChar padChar) {
    if (str.length() >= length) return str;
    return QString(length - str.length(), padChar) + str;
}

QString StringUtils::padRight(const QString& str, int length, QChar padChar) {
    if (str.length() >= length) return str;
    return str + QString(length - str.length(), padChar);
}

QString StringUtils::replaceAll(const QString& str, const QString& from, const QString& to) {
    if (from.isEmpty()) return str;
    QString result = str;
    result.replace(from, to);
    return result;
}

QString StringUtils::replaceFirst(const QString& str, const QString& from, const QString& to) {
    if (from.isEmpty()) return str;
    QString result = str;
    int index = result.indexOf(from);
    if (index >= 0) {
        result.replace(index, from.length(), to);
    }
    return result;
}

QString StringUtils::replaceLast(const QString& str, const QString& from, const QString& to) {
    if (from.isEmpty()) return str;
    QString result = str;
    int index = result.lastIndexOf(from);
    if (index >= 0) {
        result.replace(index, from.length(), to);
    }
    return result;
}

QStringList StringUtils::split(const QString& str, const QString& separator, Qt::SplitBehavior behavior) {
    return str.split(separator, behavior);
}

QString StringUtils::join(const QStringList& list, const QString& separator) {
    return list.join(separator);
}

QString StringUtils::join(const QVector<QString>& list, const QString& separator) {
    QStringList qlist;
    qlist.reserve(list.size());
    for (const QString& s : list) qlist << s;
    return qlist.join(separator);
}

bool StringUtils::startsWith(const QString& str, const QString& prefix, Qt::CaseSensitivity cs) {
    return str.startsWith(prefix, cs);
}

bool StringUtils::endsWith(const QString& str, const QString& suffix, Qt::CaseSensitivity cs) {
    return str.endsWith(suffix, cs);
}

bool StringUtils::contains(const QString& str, const QString& substr, Qt::CaseSensitivity cs) {
    return str.contains(substr, cs);
}

bool StringUtils::matches(const QString& str, const QRegularExpression& regex) {
    return regex.match(str).hasMatch();
}

QStringList StringUtils::matchAll(const QString& str, const QRegularExpression& regex) {
    QStringList results;
    QRegularExpressionMatchIterator it = regex.globalMatch(str);
    while (it.hasNext()) {
        results << it.next().captured(0);
    }
    return results;
}

QString StringUtils::htmlEscape(const QString& str) {
    QString result = str;
    result.replace("&", "&")
          .replace("<", "<")
          .replace(">", ">")
          .replace("\"", """)
          .replace("'", "'");
    return result;
}

QString StringUtils::htmlUnescape(const QString& str) {
    QString result = str;
    result.replace("&", "&")
          .replace("<", "<")
          .replace(">", ">")
          .replace(""", "\"")
          .replace("'", "'")
          .replace("&nbsp;", " ");
    return result;
}

QString StringUtils::urlEncode(const QString& str) {
    return QUrl::toPercentEncoding(str);
}

QString StringUtils::urlDecode(const QString& str) {
    return QUrl::fromPercentEncoding(str.toUtf8());
}

QString StringUtils::base64Encode(const QString& str) {
    return QString::fromUtf8(str.toUtf8().toBase64());
}

QString StringUtils::base64Decode(const QString& str) {
    return QString::fromUtf8(QByteArray::fromBase64(str.toUtf8()));
}

QString StringUtils::normalizePath(const QString& path) {
    return QDir::cleanPath(path);
}

QString StringUtils::getExtension(const QString& path) {
    QFileInfo info(path);
    return info.suffix();
}

QString StringUtils::getBaseName(const QString& path) {
    QFileInfo info(path);
    return info.baseName();
}

QString StringUtils::getDirName(const QString& path) {
    QFileInfo info(path);
    return info.absolutePath();
}

QString StringUtils::joinPath(const QString& base, const QString& relative) {
    return QDir(base).filePath(relative);
}

QString StringUtils::formatBytes(qint64 bytes, int precision) {
    static const QStringList units = {"B", "KB", "MB", "GB", "TB", "PB"};
    if (bytes < 1024) return QString::number(bytes) + " B";

    double value = bytes;
    int unitIndex = 0;
    while (value >= 1024 && unitIndex < units.size() - 1) {
        value /= 1024;
        unitIndex++;
    }
    return QString::number(value, 'f', precision) + " " + units[unitIndex];
}

QString StringUtils::formatDuration(qint64 milliseconds) {
    if (milliseconds < 1000) {
        return QString::number(milliseconds) + "ms";
    }

    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    qint64 days = hours / 24;

    if (days > 0) {
        return QString("%1天 %2小时").arg(days).arg(hours % 24);
    } else if (hours > 0) {
        return QString("%1小时 %2分").arg(hours).arg(minutes % 60);
    } else if (minutes > 0) {
        return QString("%1分 %2秒").arg(minutes).arg(seconds % 60);
    } else {
        return QString("%1秒").arg(seconds);
    }
}

QString StringUtils::formatNumber(qint64 number, const QString& thousandSep) {
    QString str = QString::number(number);
    int len = str.length();
    if (len <= 3) return str;

    QString result;
    int pos = 0;
    for (int i = len - 1; i >= 0; --i) {
        result.prepend(str[i]);
        pos++;
        if (pos == 3 && i > 0) {
            result.prepend(thousandSep);
            pos = 0;
        }
    }
    return result;
}

QString StringUtils::formatPercent(double value, int precision) {
    return QString::number(value * 100, 'f', precision) + "%";
}

bool StringUtils::isEmpty(const QString& str) {
    return str.isEmpty();
}

bool StringUtils::isBlank(const QString& str) {
    return str.trimmed().isEmpty();
}

bool StringUtils::isNumeric(const QString& str) {
    bool ok = false;
    str.toDouble(&ok);
    return ok;
}

bool StringUtils::isEmail(const QString& str) {
    static QRegularExpression emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return emailRegex.match(str).hasMatch();
}

bool StringUtils::isUrl(const QString& str) {
    QUrl url(str);
    return url.isValid() && !url.scheme().isEmpty() && !url.host().isEmpty();
}

bool StringUtils::isIpAddress(const QString& str) {
    static QRegularExpression ipv4Regex(R"(^(\d{1,3}\.){3}\d{1,3}$)");
    static QRegularExpression ipv6Regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");
    return ipv4Regex.match(str).hasMatch() || ipv6Regex.match(str).hasMatch();
}

bool StringUtils::isUuid(const QString& str) {
    static QRegularExpression uuidRegex(R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)");
    return uuidRegex.match(str).hasMatch();
}

QString StringUtils::generateUuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString StringUtils::generateRandomString(int length, const QString& charset) {
    QString result;
    result.reserve(length);
    int charsetSize = charset.length();
    for (int i = 0; i < length; ++i) {
        result += charset[QRandomGenerator::global()->bounded(charsetSize)];
    }
    return result;
}

QString StringUtils::slugify(const QString& str) {
    QString result = str.toLower();
    result = result.normalize(QString::NormalizationForm_KD);
    result.remove(QRegularExpression(R"([^\w\s-])"));
    result.replace(QRegularExpression(R"([\s-]+)"), "-");
    result = result.trimmed();
    result = result.trimmed().remove(QRegularExpression(R"(^-+|-+$)"));
    return result;
}

double StringUtils::similarity(const QString& a, const QString& b) {
    if (a == b) return 1.0;
    if (a.isEmpty() || b.isEmpty()) return 0.0;

    int dist = levenshteinDistance(a, b);
    int maxLen = qMax(a.length(), b.length());
    return 1.0 - static_cast<double>(dist) / maxLen;
}

int StringUtils::levenshteinDistance(const QString& a, const QString& b) {
    int m = a.length();
    int n = b.length();

    if (m == 0) return n;
    if (n == 0) return m;

    QVector<int> prev(n + 1), curr(n + 1);
    for (int j = 0; j <= n; ++j) prev[j] = j;

    for (int i = 1; i <= m; ++i) {
        curr[0] = i;
        for (int j = 1; j <= n; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = qMin({
                prev[j] + 1,      // deletion
                curr[j - 1] + 1,  // insertion
                prev[j - 1] + cost // substitution
            });
        }
        prev = curr;
    }
    return prev[n];
}

} // namespace explorer::utils