# Explorer on Linux

一个基于 Qt6 和 C++20 的轻量级 Linux 桌面环境（**出于娱乐目的实现**）。

## ⚠️ 重要声明

> **本项目仅用于学习和娱乐目的，因条件有限，无法在真实硬件上进行完整测试。所有代码仅在开发机上编译并通过基本单元测试，未在实际 Linux 发行版上运行完整桌面会话。**

---

## 📁 目录结构

```
explorer-linux/
├── CMakeLists.txt              # 顶层 CMake 构建文件
├── .gitignore
├── README.md                   # 本文档
├── HANDOFF.md                  # 交接文档（详细架构与接口文档）
├── docs/                       # 设计文档目录
│   ├── architecture.md         # 架构总览
│   ├── compositor-notes.md     # 合成器选型笔记
│   ├── external-integration.md # 第三方接入指南
│   ├── ipc-protocol.md         # IPC 协议定义
│   ├── layer-shell-notes.md    # Layer-shell 使用笔记
│   └── adr/                    # 架构决策记录
├── libs/                       # 核心功能库（6 个核心库）
│   ├── libexplorer-core/       # 核心功能：配置、信号、注册表、模型、文件系统、文件操作
│   ├── libexplorer-utils/      # 工具库：字符串、文件、进程、系统、日期时间、变量工具
│   ├── libexplorer-ipc/        # DBus 通信：服务、接口、对象、消息总线、服务监视器
│   ├── libexplorer-hotkeys/    # 热键管理（基于 QShortcut，预留 KGlobalAccel 集成）
│   ├── libexplorer-layer/      # Wayland Layer-shell 封装（zwlr_layer_shell_v1）
│   └── libexplorer-ui/         # UI 组件库：主题管理、基础控件
├── components/                 # 桌面组件（独立可执行进程）
│   ├── filemanager/            # 文件管理器（多标签、双栏、文件操作、WindowManager 集成）
│   ├── taskbar/                # 任务栏（开始按钮、窗口列表、系统托盘、时钟、WindowManager 集成）
│   ├── startmenu/              # 开始菜单（应用列表、搜索、电源选项、LayerShell 集成）
│   ├── desktop/                # 桌面背景/图标（LayerShell Background 层、右键菜单、拖拽）
│   ├── run-dialog/             # 运行对话框（Win+R 风格、命令执行、LayerShell Overlay）
│   ├── notification-center/    # 通知中心（DBus 接口、自动过期、LayerShell Top 层）
│   ├── search/                 # 全局搜索（应用/文件搜索、LayerShell Overlay）
│   ├── lock-screen/            # 锁屏界面（LayerShell Overlay、密码验证）
│   └── (预留组件) taskmanager/ registry-editor/ control-panel/ terminal/ calculator/ screenshot/ notepad/
├── daemon/                     # 后台守护进程
│   └── shell-daemon/           # 核心守护进程（服务管理、插件加载、WindowManager、DBus 暴露）
├── protocols/                  # Wayland 协议 XML（layer-shell、foreign-toplevel 等）
└── external-bindings/          # 外部程序热键桥接示例
```

---

## 🚀 快速开始

### 依赖要求

| 依赖 | 版本/用途 | 安装示例 |
|------|-----------|----------|
| Qt6 | Core, Gui, Widgets, DBus, X11Extras | `sudo apt install qt6-base-dev qt6-declarative-dev qt6-tools-dev-tools` |
| Wayland Protocols | Layer-shell 协议头 | `sudo apt install wayland-protocols` |
| Wayland Client | Wayland 客户端库 | `sudo apt install libwayland-client0 libwayland-client0-dev` |
| pkg-config | 查找 Wayland 库 | `sudo apt install pkg-config` |
| KF5GlobalAccel (可选) | 真正的全局热键 | `sudo apt install libkf5globalaccel-dev` |

### 编译步骤

```bash
# 1. 克隆仓库
git clone <repository-url>
cd explorer-linux

# 2. 创建构建目录
mkdir -p build && cd build

# 3. 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 4. 编译
make -j$(nproc)

# 可选：安装到系统
# sudo make install
```

### 运行示例

```bash
# 运行文件管理器（默认打开主目录）
./components/filemanager/filemanager

# 指定起始路径
./components/filemanager/filemanager -p /home/yourname/Documents

# 运行守护进程（后台服务）
./daemon/shell-daemon/shell-daemon

# 运行任务栏（需要守护进程运行以提供 WindowManager 服务）
./components/taskbar/taskbar

# 运行开始菜单（测试模式）
./components/startmenu/startmenu --test --show 0,0
```

---

## 🏗️ 核心架构概览

### 进程模型

```
┌─────────────────────────────────────────────────────────────┐
│                    Wayland Compositor (KWin)                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  Taskbar    │  │  Desktop    │  │    StartMenu        │  │
│  │ (Bottom)    │  │ (Background)│  │   (Top/Overlay)     │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
│         │                │                      │            │
└─────────┼────────────────┼──────────────────────┼────────────┘
          │                │                      │
          ▼                ▼                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Session DBus (org.explorer.*)             │
│  ┌──────────────┐  ┌──────────────────┐  ┌──────────────┐  │
│  │  WindowMgr   │  │   Daemon         │  │  MessageBus  │  │
│  │ (org.explorer│  │ (org.explorer.   │  │ (Pub/Sub)    │  │
│  │  .WindowMgr) │  │   Daemon)        │  │              │  │
│  └──────┬───────┘  └────────┬─────────┘  └──────┬───────┘  │
│         │                   │                     │         │
└─────────┼───────────────────┼─────────────────────┼─────────┘
          │                   │                     │
          ▼                   ▼                     ▼
┌─────────────────────────────────────────────────────────────┐
│                      Client Components                        │
│  ┌───────────┐  ┌────────────┐  ┌──────────┐  ┌──────────┐  │
│  │FileManager│  │ Taskbar    │  │ StartMenu│  │Desktop   │  │
│  │           │  │            │  │          │  │          │  │
│  └───────────┘  └────────────┘  └──────────┘  └──────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 核心设计原则

- **模块化**：每个功能封装为独立库/组件，便于测试与复用
- **进程隔离**：每个组件独立进程，单点故障不拖垮整个桌面
- **DBus 通信**：组件间通过 `org.explorer.*` DBus 接口通信
- **LayerShell 集成**：Wayland 表面管理（Background/Bottom/Top/Overlay 四层）
- **主题系统**：统一的 ThemeManager（浅色/深色/高对比度）

---

## 📦 核心组件状态

| 组件 | 状态 | 说明 |
|------|------|------|
| **libs (6个核心库)** | ✅ 完成 | 核心库、工具库、IPC、热键、LayerShell、UI |
| **shell-daemon** | ✅ 完成 | 核心守护进程、服务管理、WindowManager 服务 |
| **WindowManager** | ✅ 完成 | 窗口注册/管理/激活 DBus 服务 |
| **filemanager** | ✅ 完成 | 多标签、双栏、文件操作、WindowManager 集成 |
| **taskbar** | ✅ 完成 | 开始按钮、窗口列表、系统托盘、时钟、WindowManager 集成 |
| **startmenu** | ✅ 完成 | 应用列表、搜索、电源选项、LayerShell Top |
| **desktop** | ✅ 完成 | 背景/图标、右键菜单、拖拽、LayerShell Background |
| **run-dialog** | ✅ 完成 | Win+R 风格、命令执行、LayerShell Overlay |
| **notification-center** | ✅ 完成 | DBus 接口、自动过期、LayerShell Top |
| **search** | ✅ 完成 | 应用/文件搜索、LayerShell Overlay |
| **lock-screen** | ✅ 完成 | LayerShell Overlay、密码验证、DBus 集成 |
| **预留组件** | 🔲 预留 | taskmanager/registry-editor/control-panel/terminal/calculator/screenshot/notepad |

---

## 🔌 关键接口速查

### WindowManager (org.explorer.WindowManager)
- **注册窗口**: `registerWindow(title, icon, appName, minimized, maximized, active) → windowId`
- **注销窗口**: `unregisterWindow(windowId)`
- **窗口操作**: `activateWindow(id)`, `minimizeWindow(id)`, `maximizeWindow(id)`, `closeWindow(id)`
- **查询接口**: `getWindowList()`, `getActiveWindow()`, `getWindowInfo(id)`, `getWindowCount()`
- **信号**: `windowRegistered`, `windowUnregistered`, `windowStateChanged`, `activeWindowChanged`, `windowListChanged`

### Daemon (org.explorer.Daemon)
- 服务: `/org/explorer/Core`, `/org/explorer/Signals`, `/org/explorer/Services`, `/org/explorer/Plugins`

### FileManager 窗口注册
```cpp
// 启动时自动注册
windowId = WindowManager.registerWindow(
    "文件管理器 - Explorer Linux", "folder", "FileManager", 
    false, false, true
);
// 处理激活/最小化/最大化/关闭请求信号
```

---

## ⚙️ 构建与运行

```bash
# 标准构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 运行测试
ctest --output-on-failure

# 可选安装
sudo make install
```

### 环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `XDG_SESSION_TYPE` | 必须为 `wayland` 以启用 LayerShell | - |
| `XDG_CURRENT_DESKTOP` | 桌面环境标识 | `explorer-linux` |
| `QT_QPA_PLATFORM` | Qt 平台插件 | `wayland` |

---

## ⚠️ 已知限制

| 限制 | 说明 |
|------|------|
| **未在真实硬件测试** | 仅在开发机编译通过，未在真实 Wayland 合成器上完整运行 |
| **热键仅应用级** | 当前基于 QShortcut，需集成 KGlobalAccel 实现真正全局热键 |
| **日志系统未实现** | 仅使用 qDebug/qWarning/qCritical，建议后期接入 spdlog |
| **国际化未实现** | 所有 UI 文本硬编码，需添加 .ts/.qm 翻译文件 |
| **Wayland 依赖** | 仅支持 Wayland 会话，X11 不支持 LayerShell 功能 |
| **WindowManager 为模拟实现** | 实际应通过 foreign-toplevel-list-v1 从合成器获取真实窗口列表 |

---

## 📄 许可证

待定（建议 MIT，与接入生态一致）

---

## 🤝 贡献指南

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送分支 (`git push origin feature/amazing-feature`)
4. 发起 Pull Request

---

**祝您探索愉快！** 🎉

> 本项目仅供学习娱乐，代码质量与架构设计供参考，欢迎交流指正。