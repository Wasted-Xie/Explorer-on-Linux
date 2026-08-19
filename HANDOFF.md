# Explorer on Linux 项目交接文档（Handoff）

本文档旨在向后续开发者或维护者交代项目的完整框架、核心接口、库与组件的设计、构建方式以及后续工作方向。请结合源代码以及 `docs/` 目录下的设计说明一起阅读。

## 目录

1. [项目概况](#1-项目概况)
2. [核心库接口](#2-核心库接口)
   - 2.1 libexplorer-core
   - 2.2 libexplorer-utils
   - 2.3 libexplorer-ipc
   - 2.4 libexplorer-hotkeys
   - 2.5 libexplorer-layer
   - 2.6 libexplorer-ui
3. [守护进程架构](#3-守护进程架构)
4. [示例组件：文件管理器](#4-示例组件文件管理器)
5. [WindowManager 服务](#5-windowmanager-服务)
6. [核心组件详解](#6-核心组件详解)
7. [构建系统与依赖](#7-构建系统与依赖)
8. [部署与运行](#8-部署与运行)
9. [后续工作建议](#9-后续工作建议)
10. [已知问题与限制](#10-已知问题与限制)

---

## 1. 项目概况

Explorer Linux 是一个以 **Windows Explorer 交互范式** 为目标的 Linux 桌面环境原型。项目采用 **模块化、进程隔离** 的架构：

- 每个功能组件（文件管理器、任务栏、开始菜单等）均为独立可执行进程。
- 进程间通信采用 **DBus**（`org.explorer.*` 接口）。
- 与 Wayland 合成器（推荐 KWin）通过 **layer-shell** 协议进行表面管理。
- 核心功能以无 UI 依赖的 C++ 库形式提供，便于单元测试与复用。
- UI 层基于 Qt6 Widgets（可未来迁移至 QML），统一使用主题管理器实现 Win11 风格。

项目当前处于 **核心架构完备** 状态：核心库、工具库、IPC、热键、layer-shell、UI 基础组件均已实现；守护进程、WindowManager 服务已完备；以文件管理器为例的示例组件已能进行基本的目录浏览、文件操作、窗口管理集成等操作。

---

## 2. 核心库接口

以下逐库列出主要公开类与函数，供后续组件直接使用。所有头文件均位于对应库的 `src/` 目录，CMake 目标已导出对应的 `*-config.cmake`。

### 2.1 libexplorer-core

**核心功能**：配置、信号分发、注册表、模型抽象、文件系统抽象、文件操作。

#### 2.1.1 配置系统 (`Config`, `Settings`)

- `Config::instance()`：单例访问入口。
- `void Config::setValue(const QString& key, const QVariant& value);` / `QVariant Config::value(const QString& key, const QVariant& def = {}) const;`
- `bool Config::contains(const QString& key) const;`、`void Config::remove(const QString& key);`
- `Settings Settings::childGroup(const QString& name) const;`：获取子组设置。
- `Settings` 提供模板获取/设置：`T Settings::get(const QString& key, const T& def = T{}) const;`，`void Settings::set(const QString& key, const T& val);`。
- 内部使用 `QSettings`（INI 格式）存储于 `$XDG_CONFIG_HOME/explorer-linux/config.ini`。

#### 2.1.2 信号分发 (`SignalDispatcher`)

- `SignalDispatcher& SignalDispatcher::instance();`
- 发送：`template<typename... Args> void emit(const QString& signalName, Args&&... args);`（支持任意可通过 `QVariant` 构造的参数类型）。
- 连接：`template<typename Func> int connect(const QString& signalName, Func&& func);` 返回连接 ID，使用 `void disconnect(int id);` 断开。
- 内部采用类型擦除的 `std::function` 存储，线程安全（内部 `QMutex`），可跨线程发送。
- 已显式实例化常用签名（`void()`, `int`, `QString`, `int,QString`, `QString,QString`, `bool`, `int,int`）以避免链接错误。

#### 2.1.3 注册表 (`Registry`, `RegistryWatcher`)

- `Registry& Registry::instance();`
- `void Registry::setValue(const QString& path, const QVariant& value);`（路径使用 `/` 分割，如 `"desktop/icon-size"`）。
- `QVariant Registry::value(const QString& path, const QVariant& def = {}) const;`
- `bool Registry::contains(const QString& path) const;`、`QStringList Registry::subKeys(const QString& path) const;`、`QStringList Registry::valueNames(const QString& path) const;`
- `void Registry::remove(const QString& path);`、`void Registry::removeValue(const QString& path, const QString& name);`
- 监视：`int Registry::watch(const QString& path, const std::function<void(const QString&, const QString&, const QVariant&)>& cb);` 返回 watch ID，`void Registry::unwatch(int id);`。
- 内部以树形节点存储，支持路径创建与删除。
- `RegistryWatcher` 封装了 `QFileSystemWatcher`，可监视注册表导出的 JSON 文件（若需要文件备份），但当前实现主要用于内存注册表的变化通知（通过 `valueChanged` 信号）。

#### 2.1.4 数据模型 (`ExplorerModel`, `FileSystemModel`)

- `ExplorerModel`：抽象树形模型，提供 `std::shared_ptr<ModelItem> rootItem();`、`std::shared_ptr<ModelItem> findItem(const QString& id) const;`、`bool addItem(std::shared_ptr<ModelItem> parent, std::shared_ptr<ModelItem> item);`、`bool removeItem(const QString& id);`、`void clear();`。内部维护 ID->项映射，支持分类根节点（桌面、此电脑、网络、控制面板、应用程序等）。
- `FileSystemModel`：继承自 `QAbstractItemModel`，直接可用于 `QTreeView`/`QListView`。
  - `void setRootPath(const QString& path);`、`QString rootPath() const;`
  - `QModelIndex indexForPath(const QString& path) const;`、`QString filePath(const QModelIndex& index) const;`、`bool isDir(const QModelIndex& index) const;`
  - `void refresh();`、`void refresh(const QString& path);`
  - 基本文件操作：`bool mkdir(const QString& path, const QString& name);`、`bool remove(const QString& path);`、`bool rename(const QString& path, const QString& newName);`（内部调用 `QFile`/`QDir`，操作成功后自动刷新受影响目录）。
  - 数据角色：`Qt::DisplayRole`（名称、大小为 `"<DIR>"`、类型、修改日期）、`Qt::DecorationRole`（文件图标）、`Qt::TextAlignmentRole`（大小右对齐）、`Qt::SizeHintRole`（固定行高）。
  - 排序：目录在前，文件在后；同类按 `localeAwareCompare` 排序。

#### 2.1.5 文件系统工具 (`FileSystem`)

全静态类，提供路径操作、文件信息、特殊路径查询等：

- `static QString normalizePath(const QString& p);`、`static QString absolutePath(const QString& p);`、`static QString fileName(const QString& p);`、`static QString suffix(const QString& p);`
- `static bool exists(const QString& p);`、`static bool isFile(const QString& p);`、`static bool isDirectory(const QString& p);`、`static bool isReadable(const QString& p);`、`static bool isWritable(const QString& p);`、`static bool isExecutable(const QString& p);`、`static bool isHidden(const QString& p);`
- `static qint64 size(const QString& p);`、`static QDateTime lastModified(const QString& p);`、`static QString mimeType(const QString& p);`（内部 `QMimeDatabase`）。
- `static QStringList listDir(const QString& p, QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot);`、`static QStringList listFiles(const QString& p, const QStringList& nameFilters = {});`、`static QStringList listDirs(const QString& p);`
- 特殊路径：`static QString homePath();`、`static QString tempPath();`、`static QString desktopPath();`、`static QString documentsPath();`、`static QString downloadsPath();`、`static QString musicPath();`、`static QString picturesPath();`、`static QString videosPath();`、`static QString applicationsPath();`、`static QString configPath();`、`static QString cachePath();`、`static QString dataPath();`、`static QString runtimePath();`
- 存储信息：`static QStorageInfo storageInfo(const QString& p);`、`static qint64 freeSpace(const QString& p);`、`static qint64 totalSpace(const QString& p);`
- 默认程序关联：`static QString defaultApplication(const QString& mimeType);`（调用 `xdg-mime query default <mime-type>`），`static QStringList applicationsForMimeType(const QString& mimeType);`。

#### 2.1.6 文件操作 (`FileOperations`)

支持异步进度报告的文件复制、移动、删除、目录创建、重命名。

- `using ProgressCallback = std::function<void(const FileOperationProgress&)>;`
- `using FinishedCallback = std::function<void(const FileOperationResult&)>;`
- `void FileOperations::copy(const QString& src, const QString& dest, ProgressCallback prog = {}, FinishedCallback fin = {});`
- `void FileOperations::move(const QString& src, const QString& dest, ProgressCallback prog = {}, FinishedCallback fin = {});`
- `void FileOperations::remove(const QStringList& paths, ProgressCallback prog = {}, FinishedCallback fin = {});`
- `void FileOperations::mkdir(const QString& path, bool createParents = true, FinishedCallback fin = {});`（内部调用 `QDir::mkpath`）。
- `void FileOperations::rename(const QString& src, const QString& newName, FinishedCallback fin = {});`（内部 `QFile::rename`）。
- `void FileOperations::cancel();` 取消当前正在进行的操作。
- `bool FileOperations::isBusy();` 判断是否有操作进行中。
- 进度结构 `FileOperationProgress` 包含：操作类型、源路径、目标路径、总字节数、已处理字节数、当前文件索引/总文件数、当前文件名、是否被取消、错误信息。
- 结果结构 `FileOperationResult` 包含：成功标志、错误信息、已处理文件列表、已处理字节数。

所有操作在独立工作线程中执行（`QThread`），通过信号`progressUpdated(const FileOperationProgress&)`和`operationFinished(const FileOperationResult&)`向主线程报告。

### 2.2 libexplorer-utils

工具库，提供常用的字符串、文件、进程、系统、日期时间和变量实用函数，全部为**静态方法**，无状态，线程安全。

#### 2.2.1 字符串工具 (`StringUtils`)

- 大小写转换：`toLower`, `toUpper`, `toTitleCase`, `toCamelCase`, `toSnakeCase`, `toKebabCase`.
- 修剪：`trim`, `trimLeft`, `trimRight`.
- 截断：`truncate(int maxLen, const QString& suffix = "...")`, `truncateWords(int maxWords, const QString& suffix = "...")`.
- 填充：`padLeft(int len, QChar ch = ' ')`, `padRight(int len, QChar ch = ' ')`.
- 替换：`replaceAll(const QString& from, const QString& to)`, `replaceFirst`, `replaceLast`.
- 分割/连接：`split(const QString& str, const QString& sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts)`, `join(const QStringList& list, const QString& sep)`.
- 判断：`startsWith`, `endsWith`, `contains`, `matches(const QRegularExpression&)`, `matchAll(const QRegularExpression&)`.
- 编码/解码：`htmlEscape`, `htmlUnescape`, `urlEncode`, `urlDecode`, `base64Encode`, `base64Decode`.
- 路径：`normalizePath`, `getExtension`, `getBaseName`, `getDirName`, `joinPath`.
- 格式化：`formatBytes(qint64 bytes, int precision = 2)`, `formatDuration(qint64 ms)`, `formatNumber(qint64 num, const QString& sep = ",")`, `formatPercent(double val, int precision = 1)`.
- 验证：`isEmpty`, `isBlank`, `isNumeric`, `isEmail(const QString&)`, `isUrl(const QString&)`, `isIpAddress(const QString&)`, `isUuid(const QString&)`.
- 生成：`generateUuid()`, `generateRandomString(int len, const QString& charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")`, `slugify(const QString& str)`.
- 相似度：`similarity(const QString& a, const QString& b)`（基于 Levenshtein 距离），`int levenshteinDistance(const QString& a, const QString& b)`.

#### 2.2.2 文件工具 (`FileUtils`)

- 读写：`bool readFile(const QString& path, QString& out)`, `bool readFile(const QString& path, QByteArray& out)`, `bool writeFile(const QString& path, const QString& content)`, `bool writeFile(const QString& path, const QByteArray& content)`, `bool appendFile(const QString& path, const QString& content)`, `bool appendFile(const QString& path, const QByteArray& content)`.
- 复制/移动/删除：`bool copyFile(const QString& src, const QString& dst, bool overwrite = true)`, `bool moveFile(const QString& src, const QString& dst)`, `bool deleteFile(const QString& path)`, `bool deleteDir(const QString& path, bool recursive = true)`.
- 目录操作：`bool createDir(const QString& path, bool createParents = true)`, `bool dirExists(const QString& path)`, `QStringList listFiles(const QString& path, const QStringList& filters = {}, bool recursive = false)`, `QStringList listDirs(const QString& path, bool recursive = false)`, `qint64 dirSize(const QString& path)`, `int countFiles(const QString& path, bool recursive = true)`.
- 文件信息：`static qint64 fileSize(const QString& path)`, `static QDateTime lastModified(const QString& path)`, `static QString mimeType(const QString& path)`, `static QString fileType(const QString& path)`（返回 `"file"`,`"dir"`，`"symlink"` 或 `"unknown"`），`static QString permissions(const QString& path)`, `static QString owner(const QString& path)`.
- 搜索：`QStringList findFiles(const QString& path, const QString& pattern, bool recursive = true)`（使用 `QRegularExpression` 不区分大小写），`QStringList findFiles(const QString& path, const QRegularExpression& regex, bool recursive = true)`.
- 临时文件/目录：`static QString createTempFile(const QString& prefix = "tmp", const QString& suffix = "")`, `static QString createTempDir(const QString& prefix = "tmp")`, `static bool removeTempFile(const QString& path)`.
- 文件监视器（内部类 `FileUtils::FileWatcher`）：
  - `bool addPath(const QString& path);`
  - `void removePath(const QString& path);`
  - `void setCallback(const std::function<void(const QString&)>& cb);`（文件或目录变化时回调）。
- 文件锁（内部类 `FileUtils::FileLock`）：基于 `fcntl`/`flock` 的简单互斥锁，构造函数 `FileLock(const QString& lockFilePath)`，`bool tryLock(int timeoutMs = 0)`, `void unlock()`, `bool isLocked()`.

#### 2.2.3 进程工具 (`ProcessUtils`)

- 同步执行：
  ```cpp
  struct Result {
      int exitCode = -1;
      QString stdout;
      QString stderr;
      bool timedOut = false;
      bool crashed = false;
  };
  static Result execute(const QString& program, const QStringList& args = {}, int timeoutMs = 30000, const QString& workingDir = {});
  static Result execute(const QString& command, int timeoutMs = 30000); // 简单空格分割
  ```
- 异步执行（`AsyncProcess`）：
  - `using OutputCallback = std::function<void(const QString&)>;`
  - `using FinishedCallback = std::function<void(const Result&)>;`
  - `void start(const QString& program, const QStringList& args = {}, const QString& workingDir = {});`
  - `void start(const QString& command);`
  - `void terminate();` `void kill();`
  - `void setStdoutCallback(OutputCallback cb);`
  - `void setStderrCallback(OutputCallback cb);`
  - `void setFinishedCallback(FinishedCallback cb);`
  - `bool isRunning() const;`
  - `qint64 processId() const;`
  - 信号：`stdoutReceived(const QString&)`, `stderrReceived(const QString&)`, `finished(const Result&)`, `errorOccurred(QProcess::ProcessError)`.
- 可执行文件查找：`static QString findExecutable(const QString& name);`（先用 `QStandardPaths::findExecutable`，再遍历 `$PATH` 和应用目录），`static QStringList findExecutables(const QString& name);`。
- 进程信息：`static QStringList runningProcesses();`（使用 `ps -eo comm`），`static bool isProcessRunning(const QString& name);`（`pgrep -x`），`static qint64 getProcessId(const QString& name);`（`pgrep -x` 返回第一个 PID），`static void killProcess(qint64 pid, int signal = SIGTERM);`, `static void killProcessByName(const QString& name, int signal = SIGTERM);`（`pkill -x -<signal>`）。
- 环境变量：`static QString getEnv(const QString& name, const QString& def = {});`, `static void setEnv(const QString& name, const QString& value);`, `static QStringList getEnvironment();`（返回 `QProcess::systemEnvironment()`）。

#### 2.2.4 系统工具 (`SystemUtils`)

提供硬件、系统、网络、显示、电池等信息的静态方法，大多数读取 `/proc` 或使用 Qt 接口。

- 操作系统：
  - `static QString osName();`（`QSysInfo::prettyProductName()`）
  - `static QString osVersion();`（`QSysInfo::productVersion()`）
  - `static QString osArchitecture();`（`QSysInfo::currentCpuArchitecture()`）
  - `static QString kernelVersion();`（`uname -r`）
  - `static QString distribution();`（读取 `/etc/os-release` 的 `ID=`）
  - `static QString distributionVersion();`（读取 `/etc/os-release` 的 `VERSION_ID=`）
  - `static QString desktopEnvironment();`（`XDG_CURRENT_DESKTOP` 或 `DESKTOP_SESSION`）
- CPU：
  - `static QString cpuModel();`（`/proc/cpuinfo` 的 `model name`）
  - `static int cpuCores();`（`QThread::idealThreadCount()`）
  - `static int cpuThreads();`（`/proc/cpuinfo` 处理器数量）
- 内存：
  - `static qint64 totalMemory();`（`/proc/meminfo` `MemTotal`）
  - `static qint64 availableMemory();`（`/proc/meminfo` `MemAvailable`，否则 `MemFree+Buffers+Cached`）
  - `static qint64 totalSwap();`（`/proc/meminfo` `SwapTotal`）
  - `static qint64 availableSwap();`（`/proc/meminfo` `SwapFree`）
  - `static MemoryUsage memoryUsage();`（结构包含 `total`, `used`, `free`, `buffers`, `cached`, `available`）
- 交换内存使用类型同上。
- 磁盘：
  - `struct DiskInfo { QString device; QString mountPoint; QString fsType; qint64 totalBytes; qint64 freeBytes; qint64 availableBytes; };`
  - `static QList<DiskInfo> diskInfo();`（遍历 `QStorageInfo::mountedVolumes()`）
- 网络：
  - `struct NetworkInterface { QString name; QString ipAddress; QString macAddress; bool isUp; bool isLoopback; };`
  - `static QList<NetworkInterface> networkInterfaces();`（使用 `ip -j addr show` 解析 JSON，备用读取 `/sys/class/net/*`）
- 显示：
  - `struct ScreenInfo { QString name; QRect geometry; QRect availableGeometry; qreal devicePixelRatio = 1.0; int refreshRate = 60; bool isPrimary = false; };`
  - `static QList<ScreenInfo> screens();`（通过 `QGuiApplication::screens()`）
  - `static ScreenInfo primaryScreen();`
- 电池：
  - `struct BatteryInfo { bool present = false; int percentage = 0; bool charging = false; int timeRemaining = -1; }; // 秒`
  - `static BatteryInfo batteryInfo();`（读取 `/sys/class/power_supply/BAT*`）
- 系统运行时间：
  - `static qint64 uptime();`（`/proc/uptime` 首位数值，秒）
  - `static QString uptimeString();`（格式为 `X天 Y时 Z分 W秒`）
- 用户信息：
  - `static QString currentUser();`（`getenv("USER")`）
  - `static QString homeDirectory();`（`QDir::homePath()`）
  - `static QString userName();` 同上
  - `static QString userDisplayName();`（从 `/etc/passwd` 的 GECOS 字段取第一段）
- 语言/区域：
  - `static QString systemLanguage();`（`QLocale::system().name()`）
  - `static QString systemLocale();` 同上
  - `static QStringList uiLanguages();`（`QLocale::system().uiLanguages()`）
- 时间/时区：
  - `static QString timeZone();`（`QTimeZone::systemTimeZoneId()`）
  - `static bool isDaylightSavings();`（`QDateTime::currentDateTime().isDaylightTime()`）
- DPI/缩放：
  - `static qreal screenScaleFactor();`（主屏幕 `devicePixelRatio()`）
  - `static int dpi();`（主屏幕 `logicalDotsPerInch()`）
- 启动时间：
  - `static QDateTime bootTime();`（`QDateTime::currentDateTime().addSecs(-uptime())`）
- CPU 使用率：
  - `struct CpuUsage { double user = 0; double system = 0; double idle = 0; double iowait = 0; };`
  - `static CpuUsage cpuUsage();`（读取 `/proc/stat` 第一行）
  - `static QList<CpuUsage> perCpuUsage();`（每个 `cpuN` 行）
- 内存使用率同上。

#### 2.2.5 日期时间工具 (`DateTimeUtils`)

- 当前时间：
  - `static QDateTime now();`
  - `static QDate today();`
  - `static QTime currentTime();`
  - `static qint64 currentTimestamp();` // 自纪元毫秒
  - `static qint64 currentUnixTime();` // 自纪元秒
- 解析/格式化：
  - `static QDateTime fromString(const QString& str, const QString& format = Qt::ISODate);`
  - `static QString toString(const QDateTime& dt, const QString& format = Qt::ISODate);`
  - `static QString toString(const QDate& d, const QString& format = Qt::ISODate);`
  - `static QString toString(const QTime& t, const QString& format = Qt::ISODate);`
- 相对时间：
  - `static QString toRelativeString(const QDateTime& dt, const QDateTime& relativeTo = QDateTime());`
  - `static QString toShortRelativeString(const QDateTime& dt, const QDateTime& relativeTo = QDateTime());`
- 时间计算：
  - `static QDateTime startOfDay(const QDateTime& dt);`
  - `static QDateTime endOfDay(const QDateTime& dt);`
  - `static QDateTime startOfWeek(const QDateTime& dt, Qt::DayOfWeek startOfWeek = Qt::Monday);`
  - `static QDateTime endOfWeek(const QDateTime& dt, Qt::DayOfWeek startOfWeek = Qt::Monday);`
  - `static QDateTime startOfMonth(const QDateTime& dt);`
  - `static QDateTime endOfMonth(const QDateTime& dt);`
  - `static QDateTime startOfYear(const QDateTime& dt);`
  - `static QDateTime endOfYear(const QDateTime& dt);`
- 加法/减法：
  - `static QDateTime addDays(const QDateTime& dt, int days);`
  - `static QDateTime addMonths(const QDateTime& dt, int months);`
  - `static QDateTime addYears(const QDateTime& dt, int years);`
  - `static QDateTime addHours(const QDateTime& dt, int hours);`
  - `static QDateTime addMinutes(const QDateTime& dt, int minutes);`
  - `static QDateTime addSeconds(const QDateTime& dt, int seconds);`
  - `static QDateTime addMilliseconds(const QDateTime& dt, qint64 ms);`
- 差值：
  - `static qint64 daysBetween(const QDateTime& a, const QDateTime& b);`
  - `static qint64 hoursBetween(const QDateTime& a, const QDateTime& b);`
  - `static qint64 minutesBetween(const QDateTime& a, const QDateTime& b);`
  - `static qint64 secondsBetween(const QDateTime& a, const QDateTime& b);`
  - `static qint64 millisecondsBetween(const QDateTime& a, const QDateTime& b);`
- 判断：
  - `static bool isToday(const QDateTime& dt);`
  - `static bool isYesterday(const QDateTime& dt);`
  - `static bool isTomorrow(const QDateTime& dt);`
  - `static bool isThisWeek(const QDateTime& dt);`
  - `static bool isThisMonth(const QDateTime& dt);`
  - `static bool isThisYear(const QDateTime& dt);`
  - `static bool isPast(const QDateTime& dt);`
  - `static bool isFuture(const QDateTime& dt);`
  - `static bool isSameDay(const QDateTime& a, const QDateTime& b);`
  - `static bool isSameWeek(const QDateTime& a, const QDateTime& b);`
  - `static bool isSameMonth(const QDateTime& a, const QDateTime& b);`
  - `static bool isSameYear(const QDateTime& a, const QDateTime& b);`
- 时区转换：
  - `static QDateTime toTimeZone(const QDateTime& dt, const QString& tzId);`
  - `static QDateTime toLocalTime(const QDateTime& dt);`
  - `static QDateTime toUTC(const QDateTime& dt);`
  - `static QStringList availableTimeZones();`（`QTimeZone::availableTimeZoneIds()`）
- 常见格式解析（返回 `std::optional<QDateTime>`，解析失败返回 `nullopt`）：
  - `static std::optional<QDateTime> parseHttpDate(const QString& str);`
  - `static std::optional<QDateTime> parseRfc2822(const QString& str);`
  - `static std::optional<QDateTime> parseIso8601(const QString& str);`
  - `static std::optional<QDateTime> parseUnixTimestamp(const QString& str);`
- 常见格式格式化：
  - `static QString toHttpDate(const QDateTime& dt);`
  - `static QString toRfc2822(const QDateTime& dt);`
  - `static QString toIso8601(const QDateTime& dt);`
  - `static QString toUnixTimestamp(const QDateTime& dt);`
- 年龄：
  - `static int age(const QDate& birth, const QDate& ref = QDate());`
- 日期时间运算：
  - `static QDateTime nextWeekday(const QDateTime& dt, Qt::DayOfWeek day);`
  - `static QDateTime previousWeekday(const QDateTime& dt, Qt::DayOfWeek day);`
  - `static QDateTime nextOccurrence(const QDateTime& dt, const QTime& time);`
  - `static QDateTime previousOccurrence(const QDateTime& dt, const QTime& time);`

#### 2.2.6 变量工具 (`VariantUtils`)

- 类型安全获取：
  ```cpp
  template<typename T>
  static std::optional<T> get(const QVariant& v);
  template<typename T>
  static T getOrDefault(const QVariant& v, const T& def);
  template<typename T>
  static T getOrDefault(const QVariantMap& m, const QString& key, const T& def);
  ```
- JSON ↔ QVariant：
  - `static QVariant fromJson(const QString& json);`
  - `static QVariant fromJson(const QByteArray& json);`
  - `static QString toJson(const QVariant& v, bool compact = true);`
  - `static QByteArray toJsonBinary(const QVariant& v, bool compact = true);`
- QVariantMap 工具：
  - `static QVariantMap merge(const QVariantMap& a, const QVariantMap& b);`（深度合并）
  - `static QVariantMap filter(const QVariantMap& m, const QStringList& keys);`
  - `static QVariantMap omit(const QVariantMap& m, const QStringList& keys);`（等同于 `exclude`）
  - `static QVariantMap pick(const QVariantMap& m, const QStringList& keys);`（等同于 `filter`）
- 嵌套路径访问：
  - `static QVariant getPath(const QVariant& v, const QString& path, const QVariant& def = {});`（点分割，支持数组索引如 `arr.0.name`）
  - `static bool setPath(QVariant& v, const QString& path, const QVariant& value);`
  - `static bool hasPath(const QVariant& v, const QString& path);`
  - `static void removePath(QVariant& v, const QString& path);`
- 比较：
  - `static int compare(const QVariant& a, const QVariant& b);`
  - `static bool equals(const QVariant& a, const QVariant& b);`
- 克隆：`static QVariant deepClone(const QVariant& v);`（实际上返回自身，因为 QVariant 隐式共享）
- 类型判断：
  - `static bool isNull(const QVariant& v);`
  - `static bool isEmpty(const QVariant& v);`
  - `static bool isScalar(const QVariant& v);`
  - `static bool isContainer(const QVariant& v);`（`List` 或 `Map`）
- 常用转换快捷方法：
  - `static QString toString(const QVariant& v, const QString& def = {});`
  - `static int toInt(const QVariant& v, int def = 0);`
  - `static qint64 toLongLong(const QVariant& v, qint64 def = 0);`
  - `static double toDouble(const QVariant& v, double def = 0.0);`
  - `static bool toBool(const QVariant& v, bool def = false);`
  - `static QColor toColor(const QVariant& v, const QColor& def = {});`
  - `static QSize toSize(const QVariant& v, const QSize& def = {});`
  - `static QPoint toPoint(const QVariant& v, const QPoint& def = {});`
  - `static QRect toRect(const QVariant& v, const QRect& def = {});`
  - `static QUrl toUrl(const QVariant& v, const QUrl& def = {});`
  - `static QUuid toUuid(const QVariant& v, const QUuid& def = {});`
  - `static QDateTime toDateTime(const QVariant& v, const QDateTime& def = {});`
  - `static QByteArray toByteArray(const QVariant& v, const QByteArray& def = {});`
  - `static QVariantList toList(const QVariant& v, const QVariantList& def = {});`
  - `static QVariantMap toMap(const QVariant& v, const QVariantMap& def = {});`

### 2.3 libexplorer-ipc

DBus 服务、接口、对象、消息总线和服务监视器。

#### 2.3.1 DBusService

- `explicit DBusService(const QString& serviceName, QObject* parent = nullptr);`
- `bool start();` — 连接到会话总线并注册服务名；若服务名已存在则返回 `false`。
- `void stop();` — 注销所有对象和服务名。
- `bool exists(const QString& serviceName);`、`bool isRunning(const QString& serviceName);`（通过调用 `org.freedesktop.DBus.ListNames` 判断）。
- `QDBusConnection connection() const;` — 返回内部会话总线连接。
- `bool registerObject(const QString& path, QObject* object);` — 将 `object` 导出到指定 DBus 路径；内部使用 `QDBusConnection::registerObject`。
- `bool unregisterObject(const QString& path);` — 注销对象。
- `template<typename... Args> void emitSignal(const QString& iface, const QString& signal, Args&&... args);` — 发送自定义信号（路径固定为 `/`，实际使用时应先注册对象再从该对象发送信号；此方法为便捷封装，实际项目中建议由具体对象发送信号）。
- 已显式实例化常用参数类型（同上）。

#### 2.3.2 DBusInterface

- `explicit DBusInterface(const QString& service, const QString& path, const QString& iface = "", const QDBusConnection& conn = QDBusConnection::sessionBus(), QObject* parent = nullptr);` — 如果 `iface` 为空，则使用 `service` 作为接口名。
- `bool isValid() const;` — 包装的 `QDBusInterface::isValid()`。
- 调用方法：
  - `template<typename... Args> QDBusReply<QVariant> call(const QString& method, Args&&... args);`
  - `template<typename... Args> QDBusReply<void> callNoReply(const QString& method, Args&&... args);`
- 属性访问：
  - `QDBusReply<QVariant> getProperty(const QString& name);`
  - `QDBusReply<void> setProperty(const QString& name, const QVariant& value);`
- 信号连接：
  - `template<typename Func> QMetaObject::Connection connectSignal(const QString& signalName, Func&& func);`（内部使用 `QDBusInterface::signalCalled` 信号，仅演示简单参数解析；实际项目中建议使用 `QDBusInterface::connect` 直接连接到槽）。
- `QDBusInterface* interface();` — 返回底层接口指针。

#### 2.3.3 DBusObject

- `explicit DBusObject(QObject* parent = nullptr);`
- `bool exportToBus(const QString& serviceName, const QString& objectPath, const QDBusConnection& conn = QDBusConnection::sessionBus();`
- `void unexportFromBus();`
- `bool isExported() const;`
- `QString serviceName() const;`
- `QString objectPath() const;`
- `QDBusConnection connection() const;`
- 受保护虚函数（子类实现）：
  - `virtual QString dbusInterface() const { return ""; }` — 返回 DBus 接口名（若为空则使用服务名）。
  - `virtual QString dbusIntrospection() const { return ""; }` — 可选的内省 XML。
- 内部通过 `QDBusAbstractAdaptor` 实现方法和属性的转发（`methodCall`、`property`、`setProperty`）。

#### 2.3.4 MessageBus

- 简单的基于 DBus 信号的发布/订阅系统（主题字符串 + `QVariant` 消息）。
- `explicit MessageBus(QObject* parent = nullptr);`
- `bool init(const QDBusConnection& conn = QDBusConnection::sessionBus();` — 初始化并建立匹配规则。
- 发布：
  - `void publish(const QString& topic, const QVariant& msg = {});`
  - `void publish(const QString& topic, const QString& msg);`
  - `void publish(const QString& topic, int msg);`
  - `void publish(const QString& topic, bool msg);`
- 订阅：
  - `using MessageHandler = std::function<void(const QString& topic, const QVariant& msg)>;`
  - `int subscribe(const QString& topic, MessageHandler cb);` 返回订阅 ID。
  - `void unsubscribe(int id);`（实际实现需遍历查找，此处为简化；生产环境建议保存反向映射）。
  - `void unsubscribeAll(const QString& topic);`
- 信号：`void messagePublished(const QString& topic, const QVariant& msg);` — 每次发布后发出。
- 直接收发原始 DBus 消息（不常用）：
  - `bool sendMessage(const QDBusMessage& msg);`
  - `QDBusMessage receiveMessage(int timeoutMs = 1000);`

#### 2.3.5 ServiceWatcher

- 监视指定 DBus 服务的出现和消失。
- `explicit ServiceWatcher(QObject* parent = nullptr);`
- `bool watchService(const QString& serviceName);` — 添加到监视列表。
- `void unwatchService(const QString& serviceName);`
- `void unwatchAll();`
- `bool isServiceAvailable(const QString& serviceName) const;` — 调用 `org.freedesktop.DBus.ListNames` 判断。
- 信号：
  - `void serviceAppeared(const QString& name);`
  - `void serviceVanished(const QString& name);`

### 2.4 libexplorer-hotkeys

热键管理（基于 Qt 的 `QShortcut` 实现，非真正全局热键；实际项目中建议集成 `KGlobalAccel` 或 `libkeybinder3`）。

#### 2.4.1 Hotkey 结构

```cpp
struct Hotkey {
    QString id;               // 唯一标识
    QKeySequence sequence;    // 如 Qt::CTRL | Qt::Key_T
    QString description;
    bool enabled = true;
    QString context;          // 如 "global", "filemanager"
};
```

#### 2.4.2 HotkeyManager

- `explicit HotkeyManager(QObject* parent = nullptr);`
- `bool init();` — 可用于平台特定初始化（如注册到 KGlobalAccel）。
- `void shutdown();` — 注销所有快捷方式。
- 注册/更新/删除：
  - `bool registerHotkey(const Hotkey& hk);`
  - `bool unregisterHotkey(const QString& id);`
  - `bool updateHotkey(const QString& id, const Hotkey& hk);`
- 查询：
  - `bool isRegistered(const QString& id) const;`
  - `Hotkey getHotkey(const QString& id) const;`
  - `QVector<Hotkey> allHotkeys() const;`
  - `QVector<Hotkey> hotkeysForContext(const QString& ctx) const;`
- 使能控制：
  - `bool setEnabled(const QString& id, bool en);`
  - `bool setContextEnabled(const QString& ctx, bool en);`
  - `void setGlobalEnabled(bool en);`
- 信号：
  - `void hotkeyActivated(const QString& id);`
  - `void hotkeyRegistered(const QString& id);`
  - `void hotkeyUnregistered(const QString& id);`
  - `void hotkeyUpdated(const QString& id);`

#### 2.4.3 GlobalShortcut（简化单例包装）

- `static bool registerShortcut(const QString& id, const QKeySequence& seq, const std::function<void()>& cb, const QString& desc = "");`
- `static bool unregisterShortcut(const QString& id);`
- `static bool isRegistered(const QString& id);`
- 内部使用 `HotkeyManager` 单例（在 `QCoreApplication::instance()` 可用时创建）。

### 2.5 libexplorer-layer

Layer-shell 封装（基于 Wayland 不稳定协议 `zwlr_layer_shell_v1`）。

#### 2.5.1 枚举与选项

```cpp
enum class Layer { Background, Bottom, Top, Overlay };
struct LayerSurfaceOptions {
    Layer layer = Layer::Top;
    QString namespace_; // 用于分组表面
    QString description;
    QSize size;         // 0 表示使用窗口大小
    QPointF anchor;     // 左上角为原点的比例 (0~1)，每分量表示是否锚定 해당边
    QMargins margin;    // 外边距（像素）
    qreal exclusiveZone = -1; // 专用区域（像素），-1 表示自动
    bool keyboardInteractivity = false;
};
```

#### 2.5.2 LayerSurface

- `explicit LayerSurface(QWindow* window, const LayerSurfaceOptions& opts = {});`
- `~LayerSurface();`
- 初始化/关闭：
  - `bool init();` — 连接到 Wayland 显示、获取 `wl_compositor`、`zwlr_layer_shell_v1`、创建 `wl_surface` 和 `zwlr_layer_surface_v1`，设置初始属性（图层、命名空间、描述、尺寸、锚点、边距、专用区、键盘交互性）。
  - `void shutdown();` — 销毁 `wl_surface`、`zwlr_layer_surface_v1`、断开 Wayland 连接。
- 属性设置（每个设置函数会更新内部选项并在已有 surface 上重新应用；若未初始化则仅更新选项）：
  - `void setLayer(Layer l);`、`void setNamespace(const QString& ns);`、`void setDescription(const QString& d);`、`void setSize(const QSize& s);`、`void setAnchor(qreal l, qreal t, qreal r, qreal b);`、`void setMargin(int l, int t, int r, int b);`、`void setExclusiveZone(qreal z);`、`void setKeyboardInteractivity(bool k);`
- 显示/隐藏：
  - `bool show();` — 提交 surface 并标记为可见。
  - `void hide();` — 标记为不可见（实际实现中可能需要取消映射或设置为透明）。
  - `bool isVisible() const;`
  - `QWindow* window() const;` — 返回关联的窗口。
- 信号：
  - `void layerChanged(Layer l);`
  - `void namespaceChanged(const QString& ns);`
  - `void descriptionChanged(const QString& d);`
  - `void sizeChanged(const QSize& s);`
  - `void anchorChanged(qreal l, qreal t, qreal r, qreal b);`
  - `void marginChanged(int l, int t, int r, int b);`
  - `void exclusiveZoneChanged(qreal z);`
  - `void keyboardInteractivityChanged(bool k);`
  - `void visibleChanged(bool v);`
  - `void closed();` — 当 compositor 销毁 surface 时发出。

#### 2.5.3 LayerShellManager（单例）

- `static LayerShellManager& instance();`
- `bool init();` — 检查是否在 Wayland 会话（`XDG_SESSION_TYPE=="wayland"`），尝试连接到 Wayland 显示（可选）。
- `void shutdown();`
- `std::unique_ptr<LayerSurface> createSurface(QWindow* window, const LayerSurfaceOptions& opts = {});` — 若管理器未初始化则先调用 `init()`。
- `bool isAvailable() const;` — 是否已成功初始化。
- `wl_display* display() const;` — 返回底层 Wayland 显示连接（可用于高级用途）。

#### 2.5.4 工具函数 (`utils`)

- `QString layerToString(Layer l);`、`std::optional<Layer> stringToLayer(const QString& str);`
- `LayerSurfaceOptions defaultOptionsForLayer(Layer l);` — 为每层提供合理默认值（如背景层全屏锚点，底部层底部锚点并设专用区等）。

### 2.6 libexplorer-ui

UI 组件库，基于 Qt Widgets，提供主题管理和基础控件（均继承自 `BaseWidget`，自动跟随主题变化）。

#### 2.6.1 ThemeManager

- `static ThemeManager& instance();`
- 枚举：
  ```cpp
  enum class ThemeType { Light, Dark, HighContrast };
  enum class ColorRole { Window, WindowText, Base, AlternateBase, Text,
                         Button, ButtonText, BrightText, Light, Midlight, Dark,
                         Mid, Shadow, Highlight, HighlightedText, Link, LinkVisited };
  ```
- 初始化：`bool init();` — 加载默认主题数据（硬编码 RGB 值）。
- 主题切换：`void setTheme(ThemeType t);` — 应用对应调色板并更新 `QGuiApplication::setPalette()`。
- `ThemeType currentTheme() const;`
- 颜色查询：`QColor color(ColorRole role) const;`
- 整个调色板：`QPalette palette() const;`
- 图标提供者：
  - `class IconProvider { public: virtual QIcon icon(const QString& name) = 0; virtual ~IconProvider() = default; };`
  - `void setIconProvider(IconProvider* p);`
  - `IconProvider* iconProvider() const;` — 未设置时返回 `nullptr`，组件可自行降级为系统图标。
- 信号：`void themeChanged(ThemeType t);`、`void colorsChanged();`

#### 2.6.2 BaseWidget

所有 UI 组件的基类，提供自动主题跟随和占位文本、图标支持。

- `explicit BaseWidget(QWidget* parent = nullptr);`
- `virtual ~BaseWidget();`
- `void applyTheme();` — 子类可重写以使用 `ThemeManager::instance().color()` 等自定义样式。
- `void setAutoTheme(bool en);` — 默认 `true`，收到 `QApplication::paletteChanged` 事件时自动调用 `applyTheme()`。
- `bool autoTheme() const;`
- `void setIcon(const QIcon& ic);`、`QIcon icon() const;`
- `void setPlaceholderText(const QString& txt);`、`QString placeholderText() const;`

#### 2.6.3 示例控件（Button, Label 等）

为保持简洁，这里仅列出关键特性；完整实现见源码。

- **Button** (`QPushButton` 子类)
  - 构造函数：`Button(const QString& text = "", QWidget* parent = nullptr);`、`Button(const QIcon& icon, const QString& text = "", QWidget* parent = nullptr);`
  - 按钮类型：`enum class ButtonType { Normal, Primary, Secondary, Success, Danger, Warning, Info, Link };`
  - `void setButtonType(ButtonType t);`、`ButtonType buttonType() const;`
  - 加载状态：`void setLoading(bool load);`、`bool isLoading() const;` — 显示旋转图标并禁用按钮。
  - 主题响应：重写 `applyTheme()` 根据 `ButtonType` 调整背景/前景色（Primary 使用强调色，Success/Danger/Warning/Info 使用语义颜色，Link 透明并带下划线）。
- **Label** (`QLabel` 子类)
  - 构造函数：`Label(const QString& text = "", QWidget* parent = nullptr);`、`Label(QWidget* parent = nullptr);`
  - 标签类型：`enum class LabelType { Normal, Heading1, Heading2, Heading3, Caption, Body, Overline };`
  - `void setLabelType(LabelType t);`、`LabelType labelType() const;`
  - `void setAlignment(Qt::Alignment a);`（默认左|垂直居中）
  - `void setTextColor(const QColor& c);`（空则使用主题 `WindowText`）
  - 主题响应：重写 `applyTheme()` 根据 `LabelType` 调整字体大小、weight、letter-spacing 以及文本颜色。

其余控件（`LineEdit`, `ComboBox`, `ListView`, `TreeView`, `TabWidget`, `MenuBar`, `StatusBar`, `Dialog`）均继承自 `BaseWidget`，提供基本的样式统一和占位文本/图标支持；具体实现可参考源码。

---

## 3. 守护进程架构

`daemon/shell-daemon` 为会话守护进程，负责组件生命周期、热键分发、服务注册与插件管理。

### 3.1 主要类

- **Daemon** (`Daemon.h/.cpp`)：单例式（实际为栈对象）入口。
  - `bool init(int& argc, char** argv);` — 解析命令行（`-c <config>`、`-d` 调试）、初始化 Qt（若无 `QCoreApplication` 则创建临时实例）、初始化所有子系统（ExplorerModel、SignalDispatcher、ServiceManager、PluginLoader、LayerShellManager、DBusService、MessageBus、**WindowManager**）。
  - `int run();` — 进入 `QCoreApplication::exec()` 主事件循环。
  - `void shutdown();` — 按相反顺序关闭子系统。
  - 访问器：
    - `ServiceManager& serviceManager();`
    - `PluginLoader& pluginLoader();`
    - `LayerShellManager& layerShellManager();`
    - `ExplorerModel& explorerModel();`
    - `SignalDispatcher& signalDispatcher();`
    - `WindowManager& windowManager();` **← 新增**

- **ServiceManager** (`ServiceManager.h/.cpp`)：管理后台服务与插件（当前为演示实现）。
  - `bool init();`、`void shutdown();`
  - `bool registerService(const QString& name, QObject* svc);`、`QObject* service(const QString& name) const;`、`bool unregisterService(const QString& name);`
  - `bool loadPlugin(const QString& filePath);`（内部使用 `QPluginLoader`，存储 `std::unique_ptr<QPluginLoader>` 与 `QObject*` 映射）
  - `bool unloadPlugin(const QString& name);` — 先取消注册服务再卸载插件。
  - 信号：`serviceRegistered(const QString& name)`、`serviceUnregistered(const QString& name)`、`pluginLoaded(const QString& name)`、`pluginUnloaded(const QString& name)`。

- **PluginLoader** (`PluginLoader.h/.cpp`)：对 `QPluginLoader` 的轻量封装（同上）。

- 其他子系统（`ExplorerModel`, `SignalDispatcher`, `LayerShellManager`, `DBusService`, `MessageBus`, **`WindowManager`**）均为单例或值对象，在 `Daemon::Impl` 中持有。

### 3.2 DBus 暴露对象

守护进程以服务名 `org.explorer.Daemon` 注册，并将以下对象导出到根路径：

- `/org/explorer/Core` — 导出 `ExplorerModel`（供组件查询文件系统等）。
- `/org/explorer/Signals` — 导出 `SignalDispatcher`（供组件发送/接收自定义信号）。
- `/org/explorer/Services` — 导出 `ServiceManager`（供组件注册后台服务或查询已注册服务）。
- `/org/explorer/Plugins` — 导出 `PluginLoader`（供组件加载/卸载插件）。
- `/org/explorer/WindowManager` — **导出 `WindowManager` 服务（新增）**。

这些对象通过 `libexplorer-ipc` 的 `DBusObject` 导出，组件端可用 `libexplorer-ipc::DBusInterface` 调用。

### 3.3 消息总线与热键

- `MessageBus` 实例守护进程内部单例，用于组件间轻量级通知（如“壁纸已变更”“网络状态改变”等）。
- 热键通过 `HotkeyManager`（守护进程创建的单例）注册，守护进程在收到 `hotkeyActivated(id)` 时，根据配置（可内置映射或通过 DBus 查询）触发对应动作（如启动 `org.explorer.FileManager` 服务、弹出运行框等）。

---

## 4. 示例组件：文件管理器

`components/filemanager` 为演示如何使用核心库与 UI 库构建一个功能完整的文件管理器。

### 4.1 依赖

- 直接链接：`Qt6::Core Qt6::Gui Qt6::Widgets`
- 通过 CMake `add_subdirectory(../../../libs/...)` 链接的共享库：
  - `libexplorer-core`
  - `libexplorer-utils`
  - `libexplorer-ipc`
  - `libexplorer-hotkeys`
  - `libexplorer-layer`
  - `libexplorer-ui`

### 4.2 主要类

- **FileManagerWindow** (`FileManagerWindow.h/.cpp`)：主窗口，继承自 `QMainWindow`。
  - 使用 `QTabWidget` 实现多标签页。
  - 每个标签页内部：
    - `QFileSystemModel`（Qt 提供的文件系统模型）—— *实际项目中建议替换为 `libexplorer-core` 的 `FileSystemModel` 以获得更好的自定义操作和进度报告。* 此处为演示直接使用 Qt 模型以减少样板代码。
    - `QTreeView`（左侧目录树）
    - `QListView`（右侧文件列表）
    - `QSplitter` 水平分割两者
    - `QLabel` 状态栏（显示选中项数量和总大小）
  - 工具栏：返回、前进、地址栏（“转到”按钮）、新建文件夹、删除、刷新。
  - 菜单栏：文件（新建文件夹、删除、刷新、退出）、位置（主目录、上一级）、视图（切换视图预留）。
  - 信号槽：
    - 双击 `QTreeView`/`QListView`：若是目录则导航到该目录；若是文件则使用 `QDesktopServices::openUrl(QUrl::fromLocalFile(path))` 打开。
    - “转到”按钮：读取地址栏内容，若为有效目录则设置为当前根路径。
    - 新建文件夹：弹出 `QInputDialog` 获得名称，调用 `QFileSystemModel::mkdir()`（实际项目中应使用 `libexplorer-core::FileOperations::mkdir` 以获得进度回调）。
    - 删除：弹出确认框，遍历所选索引（去重），使用 `QFile::remove` 或 `QDir::removeRecursively` 删除，成功后刷新父目录。
    - 刷新：调用当前模型的 `refresh()`。
    - 主目录：打开 `QStandardPaths::homeLocation()` 的新标签页。
    - 上一级：将当前路径向上一级（除非已在根目录）。
    - 选中变化：计算所选文件（不包括目录）总大小，更新状态栏标签并在 `QStatusBar` 中临时显示 2 秒。
  - **WindowManager 集成**：
    - 启动时自动连接 `org.explorer.WindowManager` DBus 服务
    - 启动后延迟注册自身窗口（标题、图标、应用名、初始状态）
    - 监听 `windowActivateRequested`、`windowMinimizeRequested`、`windowMaximizeRequested`、`windowCloseRequested` 信号并相应处理
    - 窗口标题变化自动同步到 WindowManager
    - 析构时自动注销窗口
  - 析构时清理所有标签页的模型与视图对象。

### 4.3 资源文件

- `filemanager.desktop`：桌面文件，供菜单或自动启动使用。
  ```ini
  [Desktop Entry]
  Name=File Manager
  Comment=Explorer Linux File Manager
  Exec=filemanager
  Icon=folder
  Terminal=false
  Type=Application
  Categories=System;FileTools;
  MimeType=inode/directory;
  ```

### 4.4 构建

组件拥有自己的 `CMakeLists.txt`，通过 `add_subdirectory(../../../libs/...)` 链接共享库，随后 `add_executable(filemanager …)` 并链接 Qt 库与共享库。

### 4.5 运行

- 无参数：打开主目录的单标签页窗口。
- `-p /some/path`：打开指定路径的标签页（并尝试移除默认主目录标签页）。
- 窗口支持标签页拖放重排、右键关闭标签页。

---

## 5. WindowManager 服务

**新增核心服务**，作为窗口管理的中央枢纽，运行在守护进程中，通过 DBus 为所有组件提供窗口管理功能。

### 5.1 服务标识

- **DBus 服务名**：`org.explorer.WindowManager`
- **DBus 对象路径**：`/org/explorer/WindowManager`
- **DBus 接口名**：`org.explorer.WindowManager`

### 5.1 数据结构

#### WindowInfo（窗口信息）
```cpp
struct WindowInfo {
    QString id;           // 唯一标识（win_ + UUID前8位 + 时间戳后4位）
    QString title;        // 窗口标题
    QString icon;         // 图标名称或路径
    QString appName;      // 应用程序名称
    bool isMinimized = false;
    bool isMaximized = false;
    bool isActive = false;
    quint64 timestamp = 0; // 注册时间戳，用于排序
};
```

#### WindowState（窗口状态变更）
```cpp
struct WindowState {
    bool minimized = false;
    bool maximized = false;
    bool active = false;
    bool hasMinimized = false;  // 标记该字段是否有效
    bool hasMaximized = false;
    bool hasActive = false;
};
```

### 5.2 DBus 接口定义

#### 窗口注册/注销（由应用程序调用）
- `QString registerWindow(const QString& title, const QString& icon, const QString& appName, bool isMinimized = false, bool isMaximized = false, bool isActive = false)` — 返回窗口 ID
- `bool unregisterWindow(const QString& id)`

#### 窗口状态更新
- `bool setWindowState(const QString& id, const QVariantMap& state)` — 批量更新状态
- `bool setWindowTitle(const QString& id, const QString& title)`
- `bool setWindowIcon(const QString& id, const QString& icon)`
- `bool setWindowMinimized(const QString& id, bool minimized)`
- `bool setWindowMaximized(const QString& id, bool maximized)`
- `bool setWindowActive(const QString& id, bool active)`

#### 查询接口（由任务栏等调用）
- `QVariantList getWindowList() const` — 返回所有窗口的 `QVariantMap` 列表
- `QString getActiveWindow() const` — 返回当前激活窗口 ID
- `QVariantMap getWindowInfo(const QString& id) const`
- `int getWindowCount() const`

#### 窗口操作（由任务栏等调用）
- `bool activateWindow(const QString& id)` — 激活窗口（发送 `windowActivateRequested` 信号给应用）
- `bool minimizeWindow(const QString& id)`
- `bool maximizeWindow(const QString& id)`
- `bool closeWindow(const QString& id)` — 发送 `windowCloseRequested` 信号

### 5.3 信号（DBus 信号 + 本地 Qt 信号）

- `windowRegistered(const QString& id, const QVariantMap& info)` — 窗口注册
- `windowUnregistered(const QString& id)` — 窗口注销
- `windowStateChanged(const QString& id, const QVariantMap& state)` — 窗口状态变化
- `activeWindowChanged(const QString& id)` — 激活窗口变化
- `windowListChanged()` — 窗口列表变化
- `windowActivateRequested(const QString& id)` — 请求应用激活窗口
- `windowMinimizeRequested(const QString& id)` — 请求最小化
- `windowMaximizeRequested(const QString& id)` — 请求最大化
- `windowCloseRequested(const QString& id)` — 请求关闭

### 5.4 关键实现细节

- **窗口 ID 生成**：`win_` + UUID 前 8 位 + 时间戳后 4 位（十六进制）
- **激活窗口跟踪**：自动维护 `m_activeWindowId`，注销激活窗口时自动选择最新窗口作为激活窗口
- **状态同步**：所有状态变更均发送 DBus 信号并发射本地 Qt 信号
- **优雅关闭**：`shutdown()` 时向所有已注册窗口发送 `windowUnregistered` 信号
- **DBus 导出**：使用 `QDBusConnection::ExportScriptableSlots | ExportScriptableSignals` 导出所有 `Q_INVOKABLE` 方法和 `Q_SIGNAL`

---

## 6. 核心组件详解

### 6.1 任务栏 (`components/taskbar`)

**功能**：底部任务栏，包含开始按钮、窗口列表、系统托盘、时钟。

#### 关键类
- **TaskbarWindow** (`TaskbarWindow.h/.cpp`)：主窗口，继承 `QMainWindow`
  - LayerShell Bottom 层，独占区域 = 高度，锚定底部
  - 左侧：开始按钮 (`StartButton`)
  - 中间：窗口按钮容器 (`m_windowButtonsContainer` + `QHBoxLayout`)
  - 右侧：时钟 (`ClockLabel`) + 系统托盘 (`SystemTray`)
  - **WindowManager 集成**：
    - 通过 `TaskbarManager` 连接 `org.explorer.WindowManager` DBus 服务
    - 监听 `windowRegistered`、`windowUnregistered`、`windowStateChanged`、`activeWindowChanged`、`windowListChanged` 信号
    - `updateWindowList(const QVariantList&)` 同步 UI
    - 点击窗口按钮 → 调用 `WindowManager::activateWindow(id)`
    - 右键菜单（关闭、最小化、最大化）调用对应 WindowManager 方法

- **TaskbarManager** (`TaskbarManager.h/.cpp`)
  - 通过 `QDBusInterface` 连接 `org.explorer.WindowManager`
  - 定时器（2秒）周期性调用 `getWindowList()` 并发射 `windowListUpdated(QVariantList)`
  - 处理 WindowManager 信号转发给 `TaskbarWindow`

- **WindowButton** (`WindowButton.h/.cpp`)
  - 继承 `QPushButton`，存储 `m_windowId`
  - `clicked` 信号转发为 `windowClicked(windowId)`
  - 支持激活态显示（`setIsActive`）

- **StartButton** (`StartButton.h/.cpp`)
  - 继承 `QPushButton`，点击触发 `TaskbarManager::toggleStartMenu()`

- **SystemTray** (`SystemTray.h/.cpp`)
  - 封装 `QSystemTrayIcon`，提供 `iconActivated(QString reason)` 信号

- **ClockLabel** (`ClockLabel.h/.cpp`)
  - 定时器每秒更新时间显示

### 6.2 开始菜单 (`components/startmenu`)

**功能**：弹出式开始菜单，应用列表、搜索、电源选项。

**关键类**：
- **StartMenuWindow** (`ui/StartMenuWindow.h/.cpp`) — LayerShell Top 层，锚定左下角
- **StartMenuManager** (`core/StartMenuManager.h/.cpp`) — 解析 `.desktop` 文件、DBus 通信、应用启动
- **ApplicationItem** (`ui/ApplicationItem.h/.cpp`) — 应用条目控件

### 6.3 桌面 (`components/desktop`)

**功能**：桌面背景、图标、右键菜单、拖拽。

**关键类**：
- **DesktopWindow** (`core/DesktopWindow.h/.cpp`) — LayerShell Background 层，全屏锚定
- **DesktopView** (`ui/DesktopView.h/.cpp`) — 自定义 `QListView`（IconMode），支持拖拽、右键菜单
- 支持壁纸配置（颜色/图片）、桌面图标拖拽、右键菜单（刷新/新建文件夹/更换壁纸）

### 6.4 运行对话框 (`components/run-dialog`)

**功能**：Win+R 风格运行框，命令执行。

**关键类**：
- **RunDialog** (`RunDialog.h/.cpp`) — LayerShell Overlay 层，居中显示
- DBus 服务 `org.explorer.RunDialog`，支持 Show/Hide/Toggle
- 命令执行：解析参数、`ProcessUtils::findExecutable` 查找可执行文件、 `QProcess::startDetached`

### 6.5 通知中心 (`components/notification-center`)

**功能**：通知气泡、历史列表、清除全部。

**关键类**：
- `NotificationItem` / `NotificationModel` / `NotificationDelegate` — 数据模型与渲染
- `NotificationCenter` — LayerShell Top 层，右下角锚定
- DBus 接口 `org.explorer.NotificationCenter`：`Notify(summary, body, iconName, timeout)`, `CloseNotification(id)`, `GetCapabilities()`, `Show()`, `Hide()`
- 自动过期（默认 5 秒）、自动隐藏（8 秒）、清除全部

### 6.6 全局搜索 (`components/search`)

**功能**：应用+文件搜索。

**关键类**：
- `SearchWindow` — LayerShell Overlay，顶部居中
- `SearchModel` — 300ms 防抖搜索 `.desktop` 文件 + 家目录文件
- `SearchDelegate` — 自定义渲染（图标、名称、描述、类型）

### 6.7 锁屏 (`components/lock-screen`)

**功能**：全屏锁屏，密码验证。

**关键类**：
- `LockScreenWindow` — LayerShell Overlay，全屏锚定，键盘交互
- 半透明暗色背景、居中内容卡片、密码输入、解锁按钮
- 时间日期实时更新、密码错误摇晃动画、错误提示自动清除
- DBus 注册、MessageBus 订阅 lock/unlock 主题

---

## 7. 构建系统与依赖

### 7.1 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.25)
project(explorer-linux VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus X11Extras)

# libs
add_subdirectory(libs/libexplorer-core)
add_subdirectory(libs/libexplorer-utils)
add_subdirectory(libs/libexplorer-ui)
add_subdirectory(libs/libexplorer-layer)
add_subdirectory(libs/libexplorer-ipc)
add_subdirectory(libs/libexplorer-hotkeys)

# components
add_subdirectory(components/filemanager)
add_subdirectory(components/taskbar)
add_subdirectory(components/startmenu)
add_subdirectory(components/desktop)
add_subdirectory(components/run-dialog)
add_subdirectory(components/search)
add_subdirectory(components/lock-screen)
add_subdirectory(components/notification-center)

# daemon
add_subdirectory(daemon/shell-daemon)

enable_testing()
add_subdirectory(tests)
```

### 依赖列表

| 库/包 | 用途 | 安装示例 (Debian/Ubuntu) |
|-------|------|--------------------------|
| Qt6 (Core, Gui, Widgets, DBus, X11Extras) | UI、DBus、X11 辅助 | `sudo apt install qt6-base-dev qt6-declarative-dev qt6-tools-dev-tools` |
| wayland-protocols | Layer-shell 协议头 | `sudo apt install wayland-protocols` |
| wayland-client | Wayland 客户端库 | `sudo apt install libwayland-client0 libwayland-client0-dev` |
| pkg-config | 查找 Wayland 库 | `sudo apt install pkg-config` |
| KF5GlobalAccel (可选) | 真正全局热键 | `sudo apt install libkf5globalaccel-dev` |

### 编译选项
- `CMAKE_BUILD_TYPE`：`Release`（默认）或 `Debug`。
- 如需启用调试信息和日志，可在 `libexplorer-utils` 中加入 `QDEBUG` 开关。

---

## 8. 部署与运行

### 8.1 开发调试

```bash
# 在构建目录下
./components/filemanager/filemanager   # 运行文件管理器
./daemon/shell-daemon/shell-daemon     # 运行守护进程（可观察 DBus 服务）
./components/taskbar/taskbar           # 运行任务栏
./components/startmenu/startmenu --test --show 0,0  # 测试开始菜单
```

### 产品化部署

1. 将所有可执行文件复制至目标机器的 `/opt/explorer-linux/bin` 或 `/usr/local/bin`。
2. 将共享库复制至对应的 `lib` 目录，并更新 `ldconfig`（或设置 `LD_LIBRARY_PATH`）。
3. 确保依赖的 Qt6 库已在目标机器上可用（可通过静态链接或发布运行时依赖）。
4. 将 `*.desktop` 文件复制至 `/usr/local/share/applications`（或 `/usr/share/applications`），使菜单中出现。
5. 如需开机自启动守护进程，可创建系统服务单元：
   ```ini
   # /etc/systemd/system/explorer-daemon.service
   [Unit]
   Description=Explorer Linux Daemon
   After=graphical.target

   [Service]
   ExecStart=/usr/local/bin/shell-daemon
   Restart=on-failure

   [Install]
   WantedBy=graphical.target
   ```
   然后 `systemctl daemon-reload && systemctl enable --now explorer-daemon.service`。

### 注意事项

- 本项目目前未进行硬件适配测试，若在真实硬件上运行，请自行检查：
  - Wayland 合成器（如 KWin）是否支持 `layer-shell` 及所需协议版本。
  - DBus 会话总线是否正常。
  - 热键管理器是否与所用窗口管理器冲突（建议后期改用 `KGlobalAccel`）。
- 日志系统尚未实现，建议后期加入 `spdlog` 或 Qt 的 `qInstallMessageHandler` 以统一输出。

---

## 9. 后续工作建议

基于现有框架，后续可按以下优先级推进：

| 优先级 | 任务 | 说明 |
|--------|------|------|
| **P0** | 替换 `QFileSystemModel` 为 `libexplorer-core::FileSystemModel` | 以获得一致的文件操作进度报告和自定义角色（如图标自定义、文件属性等）。 |
| **P0** | 集成真正的全局热键（KGlobalAccel 或 libkeybinder3） | 当前 `HotkeyManager` 基于 `QShortcut` 仅在应用窗口激活时有效。 |
| **P0** | 实现通知中心与系统托盘（使用 `QSystemTrayIcon` 或层级表面） | 为组件提供统一的通知发布入口。 |
| **P0** | **任务栏完善** - 窗口分组、进度指示器、右键菜单、缩略图预览 | 让任务栏真正可用于日常操作。 |
| **P1** | 完成开始菜单、桌面、运行框等核心组件 | 参考文件管理器的模式，使用 `QAbstractItemModel` 或自定义模型展示应用列表、快捷方式等。 |
| **P1** | 丰富图标主题（参考 Win11 风格） | 在 `libexplorer-ui` 中提供图标提供者实现，从主题目录加载 SVG/PNG。 |
| **P1** | 添加插件机制（服务插件、UI 插件） | 允许第三方通过 `QLibrary` 或 `qtplugin` 扩展功能（如文件预览插件、云存储插件）。 |
| **P2** | 配置系统持久化与热重载 | 当前使用 `QSettings` INI 文件，可增添文件监视以实时生效。 |
| **P2** | 国际化（i18n）支持 | 使用 Qt 的 `QTranslator` 与 `.ts`/`.qm` 文件，提供中文/英文切换。 |
| **P2** | 搜索组件增强 - 文件内容搜索、Web 搜索集成 | 增强 `components/search` 的搜索能力。 |
| **P3** | 安全与权限控制（如文件操作提示、UAC 式提升） | 为敏感操作添加确认框或通过 `polkit` 获得临时权限。 |
| **P3** | 性能与内存优化（模型延迟加载、缓存） | 对大目录使用懒加载、缓存图标等。 |
| **P4** | 完整的系统集成测试与 CI | 在 GitHub Actions 或类似平台上编译多种发行版（Ubuntu, Fedora, Arch）并运行基本功能测试。 |

---

## 10. 已知问题与限制

| 问题 | 说明 | 备注 |
|------|------|------|
| 热键仅在应用激活时生效 | `HotkeyManager` 基于 `QShortcut`，未注册真正的全局热键 | 后期需集成 `KGlobalAccel` 或 `libkeybinder3`。 |
| 文件管理器使用 `Qt::QFileSystemModel` | 未使用自定义 `FileSystemModel`，失去进度报告和自定义角色 | 后期替换为 `libexplorer-core` 实现。 |
| 日志系统缺失 | 目前仅使用 `qDebug()`、`qWarning()`、`qCritical()` | 建议加入结构化日志库。 |
| 未实现国际化 | 所有 UI 文本硬编码为中文或英文 | 后期添加 `.ts` 文件。 |
| 未处理 Wayland 显示异常 | 若不在 Wayland 会话中，`LayerShellManager` 将不可用，但组件仍可运行（仅无法贴边/全屏） | 可在守护进程启动时检查并给出警告。 |
| 插件机制尚未实现 | `ServiceManager::loadPlugin` 为演示实现，缺少版本检查、依赖解析、沙箱等 | 生产环境需使用 `QPluginLoader` 并定义清晰的插件接口。 |
| 未进行安全审计 | 代码未经过渗透测试或安全扫描 | 如在生产环境使用，请先进行安全评估。 |
| **WindowManager 为模拟实现** | 实际应通过 Wayland `foreign-toplevel-list-v1` 从合成器获取真实窗口列表 | 当前为演示实现，组件通过 DBus 手动注册窗口。 |
| 任务栏窗口列表为演示数据 | 当前通过 `addTestWindows()` 添加测试窗口，未连接真实窗口列表 | 需在 `TaskbarManager::updateWindowList()` 中解析真实窗口列表并同步 UI。 |
| 未实现窗口分组/缩略图预览 | 任务栏仅显示简单按钮 | 类似 Windows 7+ 的窗口分组、缩略图预览需后续实现。 |

---

> 本文档截至 2025 年反映项目最新状态。后续代码变更请以源码为准，并及时更新此交接文档。

祝开发顺利！