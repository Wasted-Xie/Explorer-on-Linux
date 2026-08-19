#pragma once

#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QColor>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QUrl>
#include <QUuid>
#include <QDateTime>
#include <QByteArray>
#include <optional>
#include <type_traits>

namespace explorer::utils {

// Variant 实用工具
class VariantUtils {
public:
    // 类型安全获取
    template<typename T>
    static std::optional<T> get(const QVariant& variant);

    template<typename T>
    static T getOrDefault(const QVariant& variant, const T& defaultValue);

    template<typename T>
    static T getOrDefault(const QVariantMap& map, const QString& key, const T& defaultValue);

    // 类型转换
    static bool canConvert(const QVariant& variant, int typeId);
    static QVariant convert(const QVariant& variant, int targetTypeId);

    // JSON 互转
    static QVariant fromJson(const QString& json);
    static QVariant fromJson(const QByteArray& json);
    static QString toJson(const QVariant& variant, bool compact = true);
    static QByteArray toJsonBinary(const QVariant& variant, bool compact = true);

    // QVariantMap 工具
    static QVariantMap merge(const QVariantMap& a, const QVariantMap& b);
    static QVariantMap filter(const QVariantMap& map, const QStringList& keys);
    static QVariantMap exclude(const QVariantMap& map, const QStringList& keys);
    static QVariantMap pick(const QVariantMap& map, const QStringList& keys);
    static QVariantMap omit(const QVariantMap& map, const QStringList& keys);

    // 嵌套访问
    static QVariant getPath(const QVariant& variant, const QString& path, const QVariant& defaultValue = {});
    static bool setPath(QVariant& variant, const QString& path, const QVariant& value);
    static bool hasPath(const QVariant& variant, const QString& path);
    static void removePath(QVariant& variant, const QString& path);

    // 比较
    static int compare(const QVariant& a, const QVariant& b);
    static bool equals(const QVariant& a, const QVariant& b);

    // 克隆
    static QVariant deepClone(const QVariant& variant);

    // 类型检查
    static bool isNull(const QVariant& variant);
    static bool isEmpty(const QVariant& variant);
    static bool isScalar(const QVariant& variant);
    static bool isContainer(const QVariant& variant);

    // 常用类型快捷方法
    static QString toString(const QVariant& variant, const QString& defaultValue = {});
    static int toInt(const QVariant& variant, int defaultValue = 0);
    static qint64 toLongLong(const QVariant& variant, qint64 defaultValue = 0);
    static double toDouble(const QVariant& variant, double defaultValue = 0.0);
    static bool toBool(const QVariant& variant, bool defaultValue = false);
    static QColor toColor(const QVariant& variant, const QColor& defaultValue = {});
    static QSize toSize(const QVariant& variant, const QSize& defaultValue = {});
    static QPoint toPoint(const QVariant& variant, const QPoint& defaultValue = {});
    static QRect toRect(const QVariant& variant, const QRect& defaultValue = {});
    static QUrl toUrl(const QVariant& variant, const QUrl& defaultValue = {});
    static QUuid toUuid(const QVariant& variant, const QUuid& defaultValue = {});
    static QDateTime toDateTime(const QVariant& variant, const QDateTime& defaultValue = {});
    static QByteArray toByteArray(const QVariant& variant, const QByteArray& defaultValue = {});
    static QVariantList toList(const QVariant& variant, const QVariantList& defaultValue = {});
    static QVariantMap toMap(const QVariant& variant, const QVariantMap& defaultValue = {});
};

// 模板实现
template<typename T>
std::optional<T> VariantUtils::get(const QVariant& variant) {
    if (!variant.isValid() || variant.isNull()) return std::nullopt;

    if constexpr (std::is_same_v<T, QString>) {
        return variant.toString();
    } else if constexpr (std::is_same_v<T, int>) {
        bool ok; int v = variant.toInt(&ok); return ok ? std::optional(v) : std::nullopt;
    } else if constexpr (std::is_same_v<T, qint64>) {
        bool ok; qint64 v = variant.toLongLong(&ok); return ok ? std::optional(v) : std::nullopt;
    } else if constexpr (std::is_same_v<T, double>) {
        bool ok; double v = variant.toDouble(&ok); return ok ? std::optional(v) : std::nullopt;
    } else if constexpr (std::is_same_v<T, bool>) {
        return variant.toBool();
    } else if constexpr (std::is_same_v<T, QColor>) {
        return variant.value<QColor>();
    } else if constexpr (std::is_same_v<T, QSize>) {
        return variant.value<QSize>();
    } else if constexpr (std::is_same_v<T, QPoint>) {
        return variant.value<QPoint>();
    } else if constexpr (std::is_same_v<T, QRect>) {
        return variant.value<QRect>();
    } else if constexpr (std::is_same_v<T, QUrl>) {
        return variant.value<QUrl>();
    } else if constexpr (std::is_same_v<T, QUuid>) {
        return variant.value<QUuid>();
    } else if constexpr (std::is_same_v<T, QDateTime>) {
        return variant.toDateTime();
    } else if constexpr (std::is_same_v<T, QByteArray>) {
        return variant.toByteArray();
    } else if constexpr (std::is_same_v<T, QVariantList>) {
        return variant.toList();
    } else if constexpr (std::is_same_v<T, QVariantMap>) {
        return variant.toMap();
    } else {
        return variant.value<T>();
    }
}

template<typename T>
T VariantUtils::getOrDefault(const QVariant& variant, const T& defaultValue) {
    auto opt = get<T>(variant);
    return opt.value_or(defaultValue);
}

template<typename T>
T VariantUtils::getOrDefault(const QVariantMap& map, const QString& key, const T& defaultValue) {
    auto it = map.find(key);
    if (it == map.end()) return defaultValue;
    return getOrDefault(*it, defaultValue);
}

} // namespace explorer::utils