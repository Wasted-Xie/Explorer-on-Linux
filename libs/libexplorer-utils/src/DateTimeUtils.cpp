#include "DateTimeUtils.h"
#include <QTimeZone>
#include <QLocale>

namespace explorer::utils {

QDateTime DateTimeUtils::now() {
    return QDateTime::currentDateTime();
}

QDate DateTimeUtils::today() {
    return QDate::currentDate();
}

QTime DateTimeUtils::currentTime() {
    return QTime::currentTime();
}

qint64 DateTimeUtils::currentTimestamp() {
    return QDateTime::currentMSecsSinceEpoch();
}

qint64 DateTimeUtils::currentUnixTime() {
    return QDateTime::currentSecsSinceEpoch();
}

QDateTime DateTimeUtils::fromString(const QString& str, const QString& format) {
    return QDateTime::fromString(str, format);
}

QString DateTimeUtils::toString(const QDateTime& dt, const QString& format) {
    return dt.toString(format);
}

QString DateTimeUtils::toString(const QDate& date, const QString& format) {
    return date.toString(format);
}

QString DateTimeUtils::toString(const QTime& time, const QString& format) {
    return time.toString(format);
}

QString DateTimeUtils::toRelativeString(const QDateTime& dt, const QDateTime& relativeTo) {
    QDateTime now = relativeTo.isValid() ? relativeTo : QDateTime::currentDateTime();
    if (!dt.isValid()) return QString();

    qint64 secs = dt.secsTo(now);
    bool past = secs >= 0;
    secs = qAbs(secs);

    if (secs < 60) {
        return past ? "刚刚" : "片刻后";
    } else if (secs < 3600) {
        int mins = secs / 60;
        return past ? QString("%1分钟前").arg(mins) : QString("%1分钟后").arg(mins);
    } else if (secs < 86400) {
        int hours = secs / 3600;
        return past ? QString("%1小时前").arg(hours) : QString("%1小时后").arg(hours);
    } else if (secs < 604800) {
        int days = secs / 86400;
        return past ? QString("%1天前").arg(days) : QString("%1天后").arg(days);
    } else if (secs < 2592000) {
        int weeks = secs / 604800;
        return past ? QString("%1周前").arg(weeks) : QString("%1周后").arg(weeks);
    } else if (secs < 31536000) {
        int months = secs / 2592000;
        return past ? QString("%1个月前").arg(months) : QString("%1个月后").arg(months);
    } else {
        int years = secs / 31536000;
        return past ? QString("%1年前").arg(years) : QString("%1年后").arg(years);
    }
}

QString DateTimeUtils::toShortRelativeString(const QDateTime& dt, const QDateTime& relativeTo) {
    QDateTime now = relativeTo.isValid() ? relativeTo : QDateTime::currentDateTime();
    if (!dt.isValid()) return QString();

    qint64 secs = dt.secsTo(now);
    bool past = secs >= 0;
    secs = qAbs(secs);

    if (secs < 60) return past ? "刚刚" : "即将";
    if (secs < 3600) return past ? QString("%1m").arg(secs / 60) : QString("%1m").arg(secs / 60);
    if (secs < 86400) return past ? QString("%1h").arg(secs / 3600) : QString("%1h").arg(secs / 3600);
    if (secs < 604800) return past ? QString("%1d").arg(secs / 86400) : QString("%1d").arg(secs / 86400);
    if (secs < 2592000) return past ? QString("%1w").arg(secs / 604800) : QString("%1w").arg(secs / 604800);
    if (secs < 31536000) return past ? QString("%1mo").arg(secs / 2592000) : QString("%1mo").arg(secs / 2592000);
    return past ? QString("%1y").arg(secs / 31536000) : QString("%1y").arg(secs / 31536000);
}

QDateTime DateTimeUtils::startOfDay(const QDateTime& dt) {
    return QDateTime(dt.date(), QTime(0, 0, 0), dt.timeZone());
}

QDateTime DateTimeUtils::endOfDay(const QDateTime& dt) {
    return QDateTime(dt.date(), QTime(23, 59, 59, 999), dt.timeZone());
}

QDateTime DateTimeUtils::startOfWeek(const QDateTime& dt, Qt::DayOfWeek startOfWeek) {
    QDate date = dt.date();
    int daysToSubtract = (date.dayOfWeek() - static_cast<int>(startOfWeek) + 7) % 7;
    return QDateTime(date.addDays(-daysToSubtract), QTime(0, 0, 0), dt.timeZone());
}

QDateTime DateTimeUtils::endOfWeek(const QDateTime& dt, Qt::DayOfWeek startOfWeek) {
    QDateTime start = startOfWeek(dt, startOfWeek);
    return start.addDays(6).addSecs(86399);
}

QDateTime DateTimeUtils::startOfMonth(const QDateTime& dt) {
    return QDateTime(QDate(dt.date().year(), dt.date().month(), 1), QTime(0, 0, 0), dt.timeZone());
}

QDateTime DateTimeUtils::endOfMonth(const QDateTime& dt) {
    QDate date = dt.date();
    return QDateTime(date.addMonths(1).addDays(-1), QTime(23, 59, 59, 999), dt.timeZone());
}

QDateTime DateTimeUtils::startOfYear(const QDateTime& dt) {
    return QDateTime(QDate(dt.date().year(), 1, 1), QTime(0, 0, 0), dt.timeZone());
}

QDateTime DateTimeUtils::endOfYear(const QDateTime& dt) {
    return QDateTime(QDate(dt.date().year(), 12, 31), QTime(23, 59, 59, 999), dt.timeZone());
}

QDateTime DateTimeUtils::addDays(const QDateTime& dt, int days) {
    return dt.addDays(days);
}

QDateTime DateTimeUtils::addMonths(const QDateTime& dt, int months) {
    return dt.addMonths(months);
}

QDateTime DateTimeUtils::addYears(const QDateTime& dt, int years) {
    return dt.addYears(years);
}

QDateTime DateTimeUtils::addHours(const QDateTime& dt, int hours) {
    return dt.addSecs(hours * 3600);
}

QDateTime DateTimeUtils::addMinutes(const QDateTime& dt, int minutes) {
    return dt.addSecs(minutes * 60);
}

QDateTime DateTimeUtils::addSeconds(const QDateTime& dt, int seconds) {
    return dt.addSecs(seconds);
}

QDateTime DateTimeUtils::addMilliseconds(const QDateTime& dt, qint64 ms) {
    return dt.addMSecs(ms);
}

qint64 DateTimeUtils::daysBetween(const QDateTime& dt1, const QDateTime& dt2) {
    return dt1.daysTo(dt2);
}

qint64 DateTimeUtils::hoursBetween(const QDateTime& dt1, const QDateTime& dt2) {
    return dt1.secsTo(dt2) / 3600;
}

qint64 DateTimeUtils::minutesBetween(const QDateTime& dt1, const QDateTime& dt2) {
    return dt1.secsTo(dt2) / 60;
}

qint64 DateTimeUtils::secondsBetween(const QDateTime& dt1, const QDateTime& dt2) {
    return dt1.secsTo(dt2);
}

qint64 DateTimeUtils::millisecondsBetween(const QDateTime& dt1, const QDateTime& dt2) {
    return dt1.msecsTo(dt2);
}

bool DateTimeUtils::isToday(const QDateTime& dt) {
    return dt.date() == QDate::currentDate();
}

bool DateTimeUtils::isYesterday(const QDateTime& dt) {
    return dt.date() == QDate::currentDate().addDays(-1);
}

bool DateTimeUtils::isTomorrow(const QDateTime& dt) {
    return dt.date() == QDate::currentDate().addDays(1);
}

bool DateTimeUtils::isThisWeek(const QDateTime& dt) {
    QDateTime start = startOfWeek(QDateTime::currentDateTime());
    QDateTime end = endOfWeek(QDateTime::currentDateTime());
    return dt >= start && dt <= end;
}

bool DateTimeUtils::isThisMonth(const QDateTime& dt) {
    QDate now = QDate::currentDate();
    return dt.date().year() == now.year() && dt.date().month() == now.month();
}

bool DateTimeUtils::isThisYear(const QDateTime& dt) {
    return dt.date().year() == QDate::currentDate().year();
}

bool DateTimeUtils::isPast(const QDateTime& dt) {
    return dt < QDateTime::currentDateTime();
}

bool DateTimeUtils::isFuture(const QDateTime& dt) {
    return dt > QDateTime::currentDateTime();
}

bool DateTimeUtils::isSameDay(const QDateTime& dt1, const QDateTime& dt2) {
    return dt1.date() == dt2.date();
}

bool DateTimeUtils::isSameWeek(const QDateTime& dt1, const QDateTime& dt2) {
    return startOfWeek(dt1) == startOfWeek(dt2);
}

bool DateTimeUtils::isSameMonth(const QDateTime& dt1, const QDateTime& dt2) {
    return dt1.date().year() == dt2.date().year() && dt1.date().month() == dt2.date().month();
}

bool DateTimeUtils::isSameYear(const QDateTime& dt1, const QDateTime& dt2) {
    return dt1.date().year() == dt2.date().year();
}

QDateTime DateTimeUtils::toTimeZone(const QDateTime& dt, const QString& timeZoneId) {
    QTimeZone tz(timeZoneId.toUtf8());
    if (!tz.isValid()) return dt;
    return dt.toTimeZone(tz);
}

QDateTime DateTimeUtils::toLocalTime(const QDateTime& dt) {
    return dt.toLocalTime();
}

QDateTime DateTimeUtils::toUTC(const QDateTime& dt) {
    return dt.toUTC();
}

QStringList DateTimeUtils::availableTimeZones() {
    return QTimeZone::availableTimeZoneIds();
}

std::optional<QDateTime> DateTimeUtils::parseHttpDate(const QString& str) {
    QDateTime dt = QDateTime::fromString(str, "ddd, dd MMM yyyy hh:mm:ss GMT");
    if (dt.isValid()) return dt;

    dt = QDateTime::fromString(str, "ddd, dd-MMM-yy hh:mm:ss GMT");
    if (dt.isValid()) return dt;

    dt = QDateTime::fromString(str, "ddd MMM d hh:mm:ss yyyy");
    if (dt.isValid()) return dt;

    return std::nullopt;
}

std::optional<QDateTime> DateTimeUtils::parseRfc2822(const QString& str) {
    QDateTime dt = QDateTime::fromString(str, Qt::RFC2822Date);
    if (dt.isValid()) return dt;
    return std::nullopt;
}

std::optional<QDateTime> DateTimeUtils::parseIso8601(const QString& str) {
    QDateTime dt = QDateTime::fromString(str, Qt::ISODate);
    if (dt.isValid()) return dt;

    dt = QDateTime::fromString(str, Qt::ISODateWithMs);
    if (dt.isValid()) return dt;

    return std::nullopt;
}

std::optional<QDateTime> DateTimeUtils::parseUnixTimestamp(const QString& str) {
    bool ok;
    qint64 timestamp = str.toLongLong(&ok);
    if (ok) {
        // 尝试毫秒
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
        if (dt.isValid() && dt.year() > 1970) return dt;

        // 尝试秒
        dt = QDateTime::fromSecsSinceEpoch(timestamp);
        if (dt.isValid() && dt.year() > 1970) return dt;
    }
    return std::nullopt;
}

QString DateTimeUtils::toHttpDate(const QDateTime& dt) {
    return dt.toUTC().toString("ddd, dd MMM yyyy hh:mm:ss 'GMT'");
}

QString DateTimeUtils::toRfc2822(const QDateTime& dt) {
    return dt.toString(Qt::RFC2822Date);
}

QString DateTimeUtils::toIso8601(const QDateTime& dt) {
    return dt.toString(Qt::ISODateWithMs);
}

QString DateTimeUtils::toUnixTimestamp(const QDateTime& dt) {
    return QString::number(dt.toSecsSinceEpoch());
}

int DateTimeUtils::age(const QDate& birthDate, const QDate& referenceDate) {
    QDate ref = referenceDate.isValid() ? referenceDate : QDate::currentDate();
    int age = ref.year() - birthDate.year();
    if (ref.month() < birthDate.month() ||
        (ref.month() == birthDate.month() && ref.day() < birthDate.day())) {
        age--;
    }
    return age;
}

QDateTime DateTimeUtils::nextWeekday(const QDateTime& dt, Qt::DayOfWeek day) {
    QDate date = dt.date();
    int daysAhead = (static_cast<int>(day) - date.dayOfWeek() + 7) % 7;
    if (daysAhead == 0) daysAhead = 7;
    return QDateTime(date.addDays(daysAhead), QTime(0, 0, 0), dt.timeZone());
}

QDateTime DateTimeUtils::previousWeekday(const QDateTime& dt, Qt::DayOfWeek day) {
    QDate date = dt.date();
    int daysBehind = (date.dayOfWeek() - static_cast<int>(day) + 7) % 7;
    if (daysBehind == 0) daysBehind = 7;
    return QDateTime(date.addDays(-daysBehind), QTime(0, 0, 0), dt.timeZone());
}

QDateTime DateTimeUtils::nextOccurrence(const QDateTime& dt, const QTime& time) {
    QDateTime next = QDateTime(dt.date(), time, dt.timeZone());
    if (next <= dt) {
        next = next.addDays(1);
    }
    return next;
}

QDateTime DateTimeUtils::previousOccurrence(const QDateTime& dt, const QTime& time) {
    QDateTime prev = QDateTime(dt.date(), time, dt.timeZone());
    if (prev >= dt) {
        prev = prev.addDays(-1);
    }
    return prev;
}

} // namespace explorer::utils