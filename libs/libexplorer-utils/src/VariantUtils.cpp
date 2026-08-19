#include "VariantUtils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

namespace explorer::utils {

bool VariantUtils::canConvert(const QVariant& variant, int typeId) {
    return variant.canConvert(static_cast<QMetaType::Type>(typeId));
}

QVariant VariantUtils::convert(const QVariant& variant, int targetTypeId) {
    QVariant result = variant;
    result.convert(static_cast<QMetaType::Type>(targetTypeId));
    return result;
}

QVariant VariantUtils::fromJson(const QString& json) {
    return fromJson(json.toUtf8());
}

QVariant VariantUtils::fromJson(const QByteArray& json) {
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isNull()) return QVariant();
    return jsonValueToVariant(doc.object());
}

QString VariantUtils::toJson(const QVariant& variant, bool compact) {
    return QString::fromUtf8(toJsonBinary(variant, compact));
}

QByteArray VariantUtils::toJsonBinary(const QVariant& variant, bool compact) {
    QJsonValue jsonValue = variantToJsonValue(variant);
    QJsonDocument doc(jsonValue);
    return compact ? doc.toJson(QJsonDocument::Compact) : doc.toJson(QJsonDocument::Indented);
}

QVariantMap VariantUtils::merge(const QVariantMap& a, const QVariantMap& b) {
    QVariantMap result = a;
    for (auto it = b.constBegin(); it != b.constEnd(); ++it) {
        if (result.contains(it.key()) && result[it.key()].type() == QVariant::Map &&
            it.value().type() == QVariant::Map) {
            result[it.key()] = merge(result[it.key()].toMap(), it.value().toMap());
        } else {
            result[it.key()] = it.value();
        }
    }
    return result;
}

QVariantMap VariantUtils::filter(const QVariantMap& map, const QStringList& keys) {
    QVariantMap result;
    for (const QString& key : keys) {
        if (map.contains(key)) {
            result[key] = map[key];
        }
    }
    return result;
}

QVariantMap VariantUtils::exclude(const QVariantMap& map, const QStringList& keys) {
    QVariantMap result = map;
    for (const QString& key : keys) {
        result.remove(key);
    }
    return result;
}

QVariantMap VariantUtils::pick(const QVariantMap& map, const QStringList& keys) {
    return filter(map, keys);
}

QVariantMap VariantUtils::omit(const QVariantMap& map, const QStringList& keys) {
    return exclude(map, keys);
}

QVariant VariantUtils::getPath(const QVariant& variant, const QString& path, const QVariant& defaultValue) {
    if (path.isEmpty()) return variant;

    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    QVariant current = variant;

    for (const QString& part : parts) {
        if (current.type() == QVariant::Map) {
            QVariantMap map = current.toMap();
            if (map.contains(part)) {
                current = map[part];
            } else {
                return defaultValue;
            }
        } else if (current.type() == QVariant::List) {
            QVariantList list = current.toList();
            bool ok;
            int index = part.toInt(&ok);
            if (ok && index >= 0 && index < list.size()) {
                current = list[index];
            } else {
                return defaultValue;
            }
        } else {
            return defaultValue;
        }
    }
    return current;
}

bool VariantUtils::setPath(QVariant& variant, const QString& path, const QVariant& value) {
    if (path.isEmpty()) {
        variant = value;
        return true;
    }

    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return false;

    // 确保根是 Map
    if (variant.type() != QVariant::Map) {
        variant = QVariantMap();
    }

    QVariantMap* currentMap = &variant.value<QVariantMap>();

    for (int i = 0; i < parts.size() - 1; ++i) {
        const QString& part = parts[i];
        if (!currentMap->contains(part) || (*currentMap)[part].type() != QVariant::Map) {
            (*currentMap)[part] = QVariantMap();
        }
        currentMap = &(*currentMap)[part].value<QVariantMap>();
    }

    (*currentMap)[parts.last()] = value;
    return true;
}

bool VariantUtils::hasPath(const QVariant& variant, const QString& path) {
    return getPath(variant, path).isValid();
}

void VariantUtils::removePath(QVariant& variant, const QString& path) {
    if (path.isEmpty() || variant.type() != QVariant::Map) return;

    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;

    QVariantMap* currentMap = &variant.value<QVariantMap>();

    for (int i = 0; i < parts.size() - 1; ++i) {
        const QString& part = parts[i];
        if (!currentMap->contains(part) || (*currentMap)[part].type() != QVariant::Map) {
            return; // 路径不存在
        }
        currentMap = &(*currentMap)[part].value<QVariantMap>();
    }

    currentMap->remove(parts.last());
}

int VariantUtils::compare(const QVariant& a, const QVariant& b) {
    if (a.type() != b.type()) {
        return a.type() < b.type() ? -1 : 1;
    }

    switch (a.typeId()) {
        case QMetaType::Bool: return a.toBool() == b.toBool() ? 0 : (a.toBool() ? 1 : -1);
        case QMetaType::Int: return a.toInt() - b.toInt();
        case QMetaType::LongLong: return a.toLongLong() < b.toLongLong() ? -1 : (a.toLongLong() > b.toLongLong() ? 1 : 0);
        case QMetaType::Double: return a.toDouble() < b.toDouble() ? -1 : (a.toDouble() > b.toDouble() ? 1 : 0);
        case QMetaType::QString: return a.toString().compare(b.toString());
        case QMetaType::QDateTime: return a.toDateTime() < b.toDateTime() ? -1 : (a.toDateTime() > b.toDateTime() ? 1 : 0);
        case QMetaType::QDate: return a.toDate() < b.toDate() ? -1 : (a.toDate() > b.toDate() ? 1 : 0);
        case QMetaType::QTime: return a.toTime() < b.toTime() ? -1 : (a.toTime() > b.toTime() ? 1 : 0);
        default: return 0;
    }
}

bool VariantUtils::equals(const QVariant& a, const QVariant& b) {
    return compare(a, b) == 0;
}

QVariant VariantUtils::deepClone(const QVariant& variant) {
    // QVariant 隐式共享，直接返回即可
    return variant;
}

bool VariantUtils::isNull(const QVariant& variant) {
    return !variant.isValid() || variant.isNull();
}

bool VariantUtils::isEmpty(const QVariant& variant) {
    if (!variant.isValid() || variant.isNull()) return true;

    switch (variant.typeId()) {
        case QMetaType::QString: return variant.toString().isEmpty();
        case QMetaType::QVariantList: return variant.toList().isEmpty();
        case QMetaType::QVariantMap: return variant.toMap().isEmpty();
        case QMetaType::QByteArray: return variant.toByteArray().isEmpty();
        case QMetaType::QUrl: return variant.toUrl().isEmpty();
        default: return false;
    }
}

bool VariantUtils::isScalar(const QVariant& variant) {
    if (!variant.isValid()) return false;
    int type = variant.typeId();
    return type != QMetaType::QVariantList && type != QMetaType::QVariantMap;
}

bool VariantUtils::isContainer(const QVariant& variant) {
    if (!variant.isValid()) return false;
    int type = variant.typeId();
    return type == QMetaType::QVariantList || type == QMetaType::QVariantMap;
}

QString VariantUtils::toString(const QVariant& variant, const QString& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

int VariantUtils::toInt(const QVariant& variant, int defaultValue) {
    return getOrDefault(variant, defaultValue);
}

qint64 VariantUtils::toLongLong(const QVariant& variant, qint64 defaultValue) {
    return getOrDefault(variant, defaultValue);
}

double VariantUtils::toDouble(const QVariant& variant, double defaultValue) {
    return getOrDefault(variant, defaultValue);
}

bool VariantUtils::toBool(const QVariant& variant, bool defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QColor VariantUtils::toColor(const QVariant& variant, const QColor& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QSize VariantUtils::toSize(const QVariant& variant, const QSize& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QPoint VariantUtils::toPoint(const QVariant& variant, const QPoint& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QRect VariantUtils::toRect(const QVariant& variant, const QRect& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QUrl VariantUtils::toUrl(const QVariant& variant, const QUrl& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QUuid VariantUtils::toUuid(const QVariant& variant, const QUuid& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QDateTime VariantUtils::toDateTime(const QVariant& variant, const QDateTime& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QByteArray VariantUtils::toByteArray(const QVariant& variant, const QByteArray& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QVariantList VariantUtils::toList(const QVariant& variant, const QVariantList& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

QVariantMap VariantUtils::toMap(const QVariant& variant, const QVariantMap& defaultValue) {
    return getOrDefault(variant, defaultValue);
}

// 内部辅助函数
QJsonValue VariantUtils::variantToJsonValue(const QVariant& variant) {
    if (!variant.isValid() || variant.isNull()) return QJsonValue::Null;

    switch (variant.typeId()) {
        case QMetaType::Bool: return variant.toBool();
        case QMetaType::Int: return variant.toInt();
        case QMetaType::LongLong: return qint64(variant.toLongLong());
        case QMetaType::Double: return variant.toDouble();
        case QMetaType::QString: return variant.toString();
        case QMetaType::QVariantList: {
            QJsonArray arr;
            for (const QVariant& v : variant.toList()) {
                arr.append(variantToJsonValue(v));
            }
            return arr;
        }
        case QMetaType::QVariantMap: {
            QJsonObject obj;
            for (auto it = variant.toMap().constBegin(); it != variant.toMap().constEnd(); ++it) {
                obj[it.key()] = variantToJsonValue(it.value());
            }
            return obj;
        }
        case QMetaType::QDateTime: return variant.toDateTime().toString(Qt::ISODateWithMs);
        case QMetaType::QDate: return variant.toDate().toString(Qt::ISODate);
        case QMetaType::QTime: return variant.toTime().toString(Qt::ISODate);
        case QMetaType::QUrl: return variant.toUrl().toString();
        case QMetaType::QUuid: return variant.value<QUuid>().toString(QUuid::WithoutBraces);
        case QMetaType::QColor: return variant.value<QColor>().name(QColor::HexArgb);
        case QMetaType::QSize: {
            QSize s = variant.value<QSize>();
            return QJsonObject{{"width", s.width()}, {"height", s.height()}};
        }
        case QMetaType::QPoint: {
            QPoint p = variant.value<QPoint>();
            return QJsonObject{{"x", p.x()}, {"y", p.y()}};
        }
        case QMetaType::QRect: {
            QRect r = variant.value<QRect>();
            return QJsonObject{{"x", r.x()}, {"y", r.y()}, {"width", r.width()}, {"height", r.height()}};
        }
        case QMetaType::QByteArray: return QString::fromUtf8(variant.toByteArray().toBase64());
        default: return variant.toString();
    }
}

QVariant VariantUtils::jsonValueToVariant(const QJsonValue& value) {
    if (value.isNull()) return QVariant();
    if (value.isBool()) return value.toBool();
    if (value.isDouble()) return value.toDouble();
    if (value.isString()) return value.toString();
    if (value.isArray()) {
        QVariantList list;
        for (const QJsonValue& v : value.toArray()) {
            list.append(jsonValueToVariant(v));
        }
        return list;
    }
    if (value.isObject()) {
        QJsonObject obj = value.toObject();
        // 检查特殊类型
        if (obj.contains("width") && obj.contains("height") && obj.size() == 2) {
            return QSize(obj["width"].toInt(), obj["height"].toInt());
        }
        if (obj.contains("x") && obj.contains("y") && obj.size() == 2) {
            return QPoint(obj["x"].toInt(), obj["y"].toInt());
        }
        if (obj.contains("x") && obj.contains("y") && obj.contains("width") && obj.contains("height") && obj.size() == 4) {
            return QRect(obj["x"].toInt(), obj["y"].toInt(), obj["width"].toInt(), obj["height"].toInt());
        }
        QVariantMap map;
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            map[it.key()] = jsonValueToVariant(it.value());
        }
        return map;
    }
    return QVariant();
}

} // namespace explorer::utils