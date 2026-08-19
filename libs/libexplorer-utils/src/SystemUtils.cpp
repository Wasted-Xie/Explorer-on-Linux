#include "SystemUtils.h"
#include <QSysInfo>
#include <QScreen>
#include <QGuiApplication>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QDateTime>
#include <QLocale>
#include <QStorageInfo>
#include <unistd.h>
#include <sys/utsname.h>
#include <fstream>
#include <sstream>

namespace explorer::utils {

QString SystemUtils::osName() {
    return QSysInfo::prettyProductName();
}

QString SystemUtils::osVersion() {
    return QSysInfo::productVersion();
}

QString SystemUtils::osArchitecture() {
    return QSysInfo::currentCpuArchitecture();
}

QString SystemUtils::kernelVersion() {
    struct utsname info;
    if (uname(&info) == 0) {
        return QString::fromLatin1(info.release);
    }
    return QString();
}

QString SystemUtils::distribution() {
    // 读取 /etc/os-release
    QFile file("/etc/os-release");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("ID=")) {
                return line.mid(3).trimmed().remove('"');
            }
        }
    }
    return QSysInfo::kernelType(); // fallback
}

QString SystemUtils::distributionVersion() {
    QFile file("/etc/os-release");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("VERSION_ID=")) {
                return line.mid(11).trimmed().remove('"');
            }
        }
    }
    return QString();
}

QString SystemUtils::desktopEnvironment() {
    QString de = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    if (!de.isEmpty()) return de;

    de = qEnvironmentVariable("DESKTOP_SESSION");
    if (!de.isEmpty()) return de;

    return "unknown";
}

QString SystemUtils::cpuModel() {
    QFile file("/proc/cpuinfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("model name")) {
                return line.split(':').last().trimmed();
            }
        }
    }
    return QString();
}

int SystemUtils::cpuCores() {
    return QThread::idealThreadCount();
}

int SystemUtils::cpuThreads() {
    // 读取 /proc/cpuinfo 统计 processor 数量
    QFile file("/proc/cpuinfo");
    int count = 0;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("processor")) {
                count++;
            }
        }
    }
    return count > 0 ? count : cpuCores();
}

qint64 SystemUtils::totalMemory() {
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("MemTotal:")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024; // KB to bytes
                }
            }
        }
    }
    return 0;
}

qint64 SystemUtils::availableMemory() {
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("MemAvailable:")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024;
                }
            }
        }
    }
    // 回退：MemFree + Buffers + Cached
    return freeMemory() + buffersMemory() + cachedMemory();
}

qint64 SystemUtils::totalSwap() {
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("SwapTotal:")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024;
                }
            }
        }
    }
    return 0;
}

qint64 SystemUtils::availableSwap() {
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("SwapFree:")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024;
                }
            }
        }
    }
    return 0;
}

QList<SystemUtils::DiskInfo> SystemUtils::diskInfo() {
    QList<DiskInfo> result;
    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady()) continue;

        DiskInfo info;
        info.device = storage.device();
        info.mountPoint = storage.rootPath();
        info.fsType = storage.fileSystemType();
        info.totalBytes = storage.bytesTotal();
        info.freeBytes = storage.bytesFree();
        info.availableBytes = storage.bytesAvailable();
        result.append(info);
    }
    return result;
}

QList<SystemUtils::NetworkInterface> SystemUtils::networkInterfaces() {
    QList<NetworkInterface> result;

    // 使用 ip addr 命令
    QProcess process;
    process.start("ip", {"-j", "addr", "show"});
    process.waitForFinished(3000);

    if (process.exitCode() == 0) {
        QJsonDocument doc = QJsonDocument::fromJson(process.readAllStandardOutput());
        if (doc.isArray()) {
            for (const QJsonValue& val : doc.array()) {
                QJsonObject obj = val.toObject();
                NetworkInterface iface;
                iface.name = obj["ifname"].toString();
                iface.isUp = obj["flags"].toArray().contains("UP");
                iface.isLoopback = obj["flags"].toArray().contains("LOOPBACK");

                QJsonArray addrInfo = obj["addr_info"].toArray();
                for (const QJsonValue& addrVal : addrInfo) {
                    QJsonObject addrObj = addrVal.toObject();
                    if (addrObj["family"].toString() == "inet") {
                        iface.ipAddress = addrObj["local"].toString();
                    }
                }

                // 获取 MAC 地址
                QFile macFile("/sys/class/net/" + iface.name + "/address");
                if (macFile.open(QIODevice::ReadOnly)) {
                    iface.macAddress = QString::fromUtf8(macFile.readAll()).trimmed().toUpper();
                }

                result.append(iface);
            }
        }
    }

    return result;
}

QList<SystemUtils::ScreenInfo> SystemUtils::screens() {
    QList<ScreenInfo> result;
    QGuiApplication* app = QGuiApplication::instance();
    if (!app) return result;

    const auto screenList = app->screens();
    for (QScreen* screen : screenList) {
        ScreenInfo info;
        info.name = screen->name();
        info.geometry = screen->geometry();
        info.availableGeometry = screen->availableGeometry();
        info.devicePixelRatio = screen->devicePixelRatio();
        info.refreshRate = screen->refreshRate();
        info.isPrimary = (screen == app->primaryScreen());
        result.append(info);
    }
    return result;
}

SystemUtils::ScreenInfo SystemUtils::primaryScreen() {
    QGuiApplication* app = QGuiApplication::instance();
    if (app && app->primaryScreen()) {
        QScreen* screen = app->primaryScreen();
        ScreenInfo info;
        info.name = screen->name();
        info.geometry = screen->geometry();
        info.availableGeometry = screen->availableGeometry();
        info.devicePixelRatio = screen->devicePixelRatio();
        info.refreshRate = screen->refreshRate();
        info.isPrimary = true;
        return info;
    }
    return ScreenInfo();
}

SystemUtils::BatteryInfo SystemUtils::batteryInfo() {
    BatteryInfo info;

    // 读取 /sys/class/power_supply/BAT*
    QDir dir("/sys/class/power_supply");
    QStringList batteries = dir.entryList({"BAT*"}, QDir::Dirs);
    if (batteries.isEmpty()) {
        batteries = dir.entryList({"battery*"}, QDir::Dirs);
    }

    if (!batteries.isEmpty()) {
        info.present = true;
        QString batPath = "/sys/class/power_supply/" + batteries.first();

        QFile capFile(batPath + "/capacity");
        if (capFile.open(QIODevice::ReadOnly)) {
            info.percentage = QString::fromUtf8(capFile.readAll()).trimmed().toInt();
        }

        QFile statusFile(batPath + "/status");
        if (statusFile.open(QIODevice::ReadOnly)) {
            QString status = QString::fromUtf8(statusFile.readAll()).trimmed();
            info.charging = (status == "Charging" || status == "Full");
        }

        // 估算剩余时间（需要当前功率和能量）
        QFile energyNow(batPath + "/energy_now");
        QFile powerNow(batPath + "/power_now");
        if (energyNow.open(QIODevice::ReadOnly) && powerNow.open(QIODevice::ReadOnly)) {
            qint64 energy = QString::fromUtf8(energyNow.readAll()).trimmed().toLongLong();
            qint64 power = QString::fromUtf8(powerNow.readAll()).trimmed().toLongLong();
            if (power > 0) {
                info.timeRemaining = energy / power * 3600; // 微瓦时 -> 秒
            }
        }
    }

    return info;
}

qint64 SystemUtils::uptime() {
    QFile file("/proc/uptime");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line = in.readLine();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (!parts.isEmpty()) {
            return static_cast<qint64>(parts[0].toDouble());
        }
    }
    return 0;
}

QString SystemUtils::uptimeString() {
    qint64 secs = uptime();
    qint64 days = secs / 86400;
    secs %= 86400;
    qint64 hours = secs / 3600;
    secs %= 3600;
    qint64 mins = secs / 60;
    secs %= 60;

    QStringList parts;
    if (days > 0) parts << QString("%1天").arg(days);
    if (hours > 0) parts << QString("%1小时").arg(hours);
    if (mins > 0) parts << QString("%1分").arg(mins);
    parts << QString("%1秒").arg(secs);
    return parts.join(" ");
}

QString SystemUtils::currentUser() {
    return qgetenv("USER");
}

QString SystemUtils::homeDirectory() {
    return QDir::homePath();
}

QString SystemUtils::userName() {
    return qgetenv("USER");
}

QString SystemUtils::userDisplayName() {
    // 从 /etc/passwd 获取
    QFile file("/etc/passwd");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString user = currentUser();
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith(user + ":")) {
                QStringList parts = line.split(':');
                if (parts.size() >= 5) {
                    QString gecos = parts[4];
                    QStringList gecosParts = gecos.split(',');
                    return gecosParts.first();
                }
            }
        }
    }
    return userName();
}

QString SystemUtils::systemLanguage() {
    return QLocale::system().name();
}

QString SystemUtils::systemLocale() {
    return QLocale::system().name();
}

QStringList SystemUtils::uiLanguages() {
    return QLocale::system().uiLanguages();
}

QString SystemUtils::timeZone() {
    return QTimeZone::systemTimeZoneId();
}

bool SystemUtils::isDaylightSavings() {
    return QDateTime::currentDateTime().isDaylightTime();
}

qreal SystemUtils::screenScaleFactor() {
    QGuiApplication* app = QGuiApplication::instance();
    if (app && app->primaryScreen()) {
        return app->primaryScreen()->devicePixelRatio();
    }
    return 1.0;
}

int SystemUtils::dpi() {
    QGuiApplication* app = QGuiApplication::instance();
    if (app && app->primaryScreen()) {
        return app->primaryScreen()->logicalDotsPerInch();
    }
    return 96;
}

QDateTime SystemUtils::bootTime() {
    qint64 uptimeSecs = uptime();
    return QDateTime::currentDateTime().addSecs(-uptimeSecs);
}

SystemUtils::MemoryUsage SystemUtils::memoryUsage() {
    MemoryUsage usage;
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() < 2) continue;

            qint64 value = parts[1].toLongLong() * 1024; // KB to bytes

            if (line.startsWith("MemTotal:")) usage.total = value;
            else if (line.startsWith("MemFree:")) usage.free = value;
            else if (line.startsWith("Buffers:")) usage.buffers = value;
            else if (line.startsWith("Cached:")) usage.cached = value;
            else if (line.startsWith("MemAvailable:")) usage.available = value;
        }
    }
    usage.used = usage.total - usage.free - usage.buffers - usage.cached;
    return usage;
}

SystemUtils::CpuUsage SystemUtils::cpuUsage() {
    CpuUsage usage;
    QFile file("/proc/stat");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line = in.readLine(); // 第一行是总计
        if (line.startsWith("cpu ")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 8) {
                // user, nice, system, idle, iowait, irq, softirq, steal
                qint64 user = parts[1].toLongLong();
                qint64 nice = parts[2].toLongLong();
                qint64 system = parts[3].toLongLong();
                qint64 idle = parts[4].toLongLong();
                qint64 iowait = parts[5].toLongLong();

                qint64 total = user + nice + system + idle + iowait;
                if (total > 0) {
                    usage.user = (user + nice) * 100.0 / total;
                    usage.system = system * 100.0 / total;
                    usage.idle = idle * 100.0 / total;
                    usage.iowait = iowait * 100.0 / total;
                }
            }
        }
    }
    return usage;
}

QList<SystemUtils::CpuUsage> SystemUtils::perCpuUsage() {
    QList<CpuUsage> result;
    QFile file("/proc/stat");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("cpu") && line.length() > 3 && line[3].isDigit()) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 8) {
                    CpuUsage usage;
                    qint64 user = parts[1].toLongLong();
                    qint64 nice = parts[2].toLongLong();
                    qint64 system = parts[3].toLongLong();
                    qint64 idle = parts[4].toLongLong();
                    qint64 iowait = parts[5].toLongLong();

                    qint64 total = user + nice + system + idle + iowait;
                    if (total > 0) {
                        usage.user = (user + nice) * 100.0 / total;
                        usage.system = system * 100.0 / total;
                        usage.idle = idle * 100.0 / total;
                        usage.iowait = iowait * 100.0 / total;
                        result.append(usage);
                    }
                }
            }
        }
    }
    return result;
}

} // namespace explorer::utils