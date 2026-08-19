#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QRect>
#include <QSize>
#include <QPoint>
#include <optional>

namespace explorer::utils {

// 系统信息工具
class SystemUtils {
public:
    // 操作系统信息
    static QString osName();
    static QString osVersion();
    static QString osArchitecture();
    static QString kernelVersion();
    static QString distribution();
    static QString distributionVersion();
    static QString desktopEnvironment();

    // 硬件信息
    static QString cpuModel();
    static int cpuCores();
    static int cpuThreads();
    static qint64 totalMemory();
    static qint64 availableMemory();
    static qint64 totalSwap();
    static qint64 availableSwap();

    // 磁盘信息
    struct DiskInfo {
        QString device;
        QString mountPoint;
        QString fsType;
        qint64 totalBytes = 0;
        qint64 freeBytes = 0;
        qint64 availableBytes = 0;
    };
    static QList<DiskInfo> diskInfo();

    // 网络信息
    struct NetworkInterface {
        QString name;
        QString ipAddress;
        QString macAddress;
        bool isUp = false;
        bool isLoopback = false;
    };
    static QList<NetworkInterface> networkInterfaces();

    // 显示信息
    struct ScreenInfo {
        QString name;
        QRect geometry;
        QRect availableGeometry;
        qreal devicePixelRatio = 1.0;
        int refreshRate = 60;
        bool isPrimary = false;
    };
    static QList<ScreenInfo> screens();
    static ScreenInfo primaryScreen();

    // 电源信息
    struct BatteryInfo {
        bool present = false;
        int percentage = 0;
        bool charging = false;
        int timeRemaining = -1; // 秒
    };
    static BatteryInfo batteryInfo();

    // 系统运行时间
    static qint64 uptime(); // 秒
    static QString uptimeString();

    // 用户信息
    static QString currentUser();
    static QString homeDirectory();
    static QString userName();
    static QString userDisplayName();

    // 语言/区域
    static QString systemLanguage();
    static QString systemLocale();
    static QStringList uiLanguages();

    // 时间/时区
    static QString timeZone();
    static bool isDaylightSavings();

    // DPI/缩放
    static qreal screenScaleFactor();
    static int dpi();

    // 启动时间
    static QDateTime bootTime();

    // 内存使用
    struct MemoryUsage {
        qint64 total = 0;
        qint64 used = 0;
        qint64 free = 0;
        qint64 buffers = 0;
        qint64 cached = 0;
        qint64 available = 0;
    };
    static MemoryUsage memoryUsage();

    // CPU 使用率
    struct CpuUsage {
        double user = 0;
        double system = 0;
        double idle = 0;
        double iowait = 0;
    };
    static CpuUsage cpuUsage();
    static QList<CpuUsage> perCpuUsage();
};

} // namespace explorer::utils