#pragma once

#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QString>
#include <optional>

namespace explorer::utils {

// 日期时间工具
class DateTimeUtils {
public:
    // 当前时间
    static QDateTime now();
    static QDate today();
    static QTime currentTime();
    static qint64 currentTimestamp(); // 毫秒
    static qint64 currentUnixTime();  // 秒

    // 解析/格式化
    static QDateTime fromString(const QString& str, const QString& format = Qt::ISODate);
    static QString toString(const QDateTime& dt, const QString& format = Qt::ISODate);
    static QString toString(const QDate& date, const QString& format = Qt::ISODate);
    static QString toString(const QTime& time, const QString& format = Qt::ISODate);

    // 相对时间格式化
    static QString toRelativeString(const QDateTime& dt, const QDateTime& relativeTo = QDateTime());
    static QString toShortRelativeString(const QDateTime& dt, const QDateTime& relativeTo = QDateTime());

    // 时间计算
    static QDateTime startOfDay(const QDateTime& dt);
    static QDateTime endOfDay(const QDateTime& dt);
    static QDateTime startOfWeek(const QDateTime& dt, Qt::DayOfWeek startOfWeek = Qt::Monday);
    static QDateTime endOfWeek(const QDateTime& dt, Qt::DayOfWeek startOfWeek = Qt::Monday);
    static QDateTime startOfMonth(const QDateTime& dt);
    static QDateTime endOfMonth(const QDateTime& dt);
    static QDateTime startOfYear(const QDateTime& dt);
    static QDateTime endOfYear(const QDateTime& dt);

    // 添加/减去时间
    static QDateTime addDays(const QDateTime& dt, int days);
    static QDateTime addMonths(const QDateTime& dt, int months);
    static QDateTime addYears(const QDateTime& dt, int years);
    static QDateTime addHours(const QDateTime& dt, int hours);
    static QDateTime addMinutes(const QDateTime& dt, int minutes);
    static QDateTime addSeconds(const QDateTime& dt, int seconds);
    static QDateTime addMilliseconds(const QDateTime& dt, qint64 ms);

    // 差值
    static qint64 daysBetween(const QDateTime& dt1, const QDateTime& dt2);
    static qint64 hoursBetween(const QDateTime& dt1, const QDateTime& dt2);
    static qint64 minutesBetween(const QDateTime& dt1, const QDateTime& dt2);
    static qint64 secondsBetween(const QDateTime& dt1, const QDateTime& dt2);
    static qint64 millisecondsBetween(const QDateTime& dt1, const QDateTime& dt2);

    // 判断
    static bool isToday(const QDateTime& dt);
    static bool isYesterday(const QDateTime& dt);
    static bool isTomorrow(const QDateTime& dt);
    static bool isThisWeek(const QDateTime& dt);
    static bool isThisMonth(const QDateTime& dt);
    static bool isThisYear(const QDateTime& dt);
    static bool isPast(const QDateTime& dt);
    static bool isFuture(const QDateTime& dt);
    static bool isSameDay(const QDateTime& dt1, const QDateTime& dt2);
    static bool isSameWeek(const QDateTime& dt1, const QDateTime& dt2);
    static bool isSameMonth(const QDateTime& dt1, const QDateTime& dt2);
    static bool isSameYear(const QDateTime& dt1, const QDateTime& dt2);

    // 时区
    static QDateTime toTimeZone(const QDateTime& dt, const QString& timeZoneId);
    static QDateTime toLocalTime(const QDateTime& dt);
    static QDateTime toUTC(const QDateTime& dt);
    static QStringList availableTimeZones();

    // 解析常见格式
    static std::optional<QDateTime> parseHttpDate(const QString& str);
    static std::optional<QDateTime> parseRfc2822(const QString& str);
    static std::optional<QDateTime> parseIso8601(const QString& str);
    static std::optional<QDateTime> parseUnixTimestamp(const QString& str);

    // 格式化为常见格式
    static QString toHttpDate(const QDateTime& dt);
    static QString toRfc2822(const QDateTime& dt);
    static QString toIso8601(const QDateTime& dt);
    static QString toUnixTimestamp(const QDateTime& dt);

    // 年龄计算
    static int age(const QDate& birthDate, const QDate& referenceDate = QDate());

    // 下一个/上一个
    static QDateTime nextWeekday(const QDateTime& dt, Qt::DayOfWeek day);
    static QDateTime previousWeekday(const QDateTime& dt, Qt::DayOfWeek day);
    static QDateTime nextOccurrence(const QDateTime& dt, const QTime& time);
    static QDateTime previousOccurrence(const QDateTime& dt, const QTime& time);
};

} // namespace explorer::utils