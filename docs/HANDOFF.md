# 项目交接文档（HANDOFF）

> 交接日期：2026-08-18
> 交接状态：探索完成、骨架就绪、代码未开始

本文档供接手人快速了解项目全貌，无需翻阅全部对话记录即可继续开发。

---

## 一、项目概况

**Explorer for Linux** —— 在 Linux 上重现 Windows Explorer（Shell）交互范式的桌面环境项目。

- 不是"移植"（不碰 Win32/COM），而是"重实现交互范式"
- 覆盖范围：文件管理器 + 任务栏 + 开始菜单 + 桌面 + 系统工具套件
- 参考生态：B 站/Awesome-Windows-on-Linux 的整活项目浪潮，本项目走务实可用路线

## 二、已完成的决策（不可轻易推翻，改动需 ADR）

| # | 决策 | 结论 | 文档 |
|---|------|------|------|
| 1 | 技术路线 | B 路线：自研 shell 组件集 + 挂现有合成器（不写完整 DE） | architecture.md |
| 2 | 合成器 | KWin（操作逻辑最接近 Windows） | compositor-notes.md |
| 3 | 组件形态 | 独立进程 + layer-shell + DBus 通信 | architecture.md |
| 4 | 技术栈 | Qt 6 / C++20，core 不依赖 UI | architecture.md |
| 5 | 第三方接入 | 热键桥接：零注册/零改造/零冲突 | external-integration.md |
| 6 | 构建 | CMake monorepo，组件可单独构建 | CMakeLists.txt |

## 三、已完成的工作

1. **项目结构**：97 目录 + 占位文件，高度模块化（libs / components / daemon 三层分离）
2. **预留组件**：7 个空壳（taskmanager、registry-editor、control-panel、terminal、screenshot、calculator、notepad），含 manifest.json 模板
3. **文档体系**：架构、开发计划、接入指南、合成器笔记、IPC、ADR 目录
4. **顶层 CMakeLists**：依赖顺序已排（libs → components → daemon），但子目录 CMakeLists 未写

## 四、当前状态与未完成事项

### 未开始（代码零行）

项目目前**没有任何源码**，全部是目录骨架和文档。

### 接手后第一优先（P0）

| 任务 | 为什么先做 | 验收 |
|------|-----------|------|
| spike S1：KWin layer-shell 实测 | 任务栏/桌面/开始菜单的实现基础，KWin 支持不完整则方案要降级 | 最小 layer-shell 面板贴边成功 |
| spike S2：foreign-toplevel 实测 | 任务栏窗口列表取数路径 | 拿到窗口列表（标题/激活状态） |
| spike S3：kglobalaccel 实测 | 全局热键实现路径 | 热键注册并触发成功 |

### 其次（P1）

| 任务 | 说明 |
|------|------|
| 阶段 1 地基 | libexplorer-utils + libexplorer-core 可编译、单测、CI |
| 阶段 2 filemanager MVP | 里程碑 M1：主窗口三栏 + 导航 + 列表 + 面包屑 + 前进后退 |

## 五、环境要求

| 项 | 要求 | 备注 |
|----|------|------|
| 操作系统 | Linux（目标），开发可先在 Windows 进行 | Qt 跨平台，core/utils 无合成器依赖 |
| 桌面环境 | KWin（KDE Plasma 6） | spike 实测用 |
| 构建 | CMake ≥ 3.25，Ninja 或 Make | |
| 编译器 | GCC/Clang，C++20 | |
| 依赖 | Qt 6（Core/Gui/Widgets） | kglobalaccel、wayland-protocols 阶段 3 起需要 |

## 六、风险与降级预案

| 风险 | 影响 | 降级方案 |
|------|------|---------|
| KWin 对 layer-shell 支持不完整 | 任务栏/桌面无法实现 | 改用 wlr-layer-shell 或 KWin 插件（S1 实测后决策） |
| foreign-toplevel 拿不到窗口图标 | 任务栏显示残缺 | KWin DBus（org.kde.KWin）补充取数 |
| Wayland 全局热键受限 | 热键桥接失效 | kglobalaccel / KWin 快捷键方案 |
| 单人开发进度慢 | 里程碑延迟 | 已按"可运行优先"切分，每阶段可独立交付 |

## 七、接手步骤（建议顺序）

1. 通读 `README.md` → `docs/architecture.md` → `docs/development-plan.md`
2. 在目标 Linux 机器（KWin）上执行 3 个 spike（S1/S2/S3），更新 `docs/compositor-notes.md`
3. 写 `docs/adr/001-kwin-compositor.md` 记录 spike 结论
4. 开始阶段 1：补齐各子目录 CMakeLists，让顶层构建通过
5. 开始阶段 2：filemanager 最小窗口

## 八、相关参考

- 第三方接入候选：heyManNice/regedit（注册表编辑器，github.com/heyManNice/regedit）
- 生态参考：windowix/awesome-windows-on-linux（同类项目合集）
- 同类（整活参考）：macOS-Terminal/Explorer-for-Linux（Qt 复刻 Win11 资源管理器，含未响应套壳）
- 主题资源：yeyushengfan258/Win11-icon-theme（Win11 图标主题）

## 九、交接说明

- 项目源码骨架位置：工作区 `explorer-linux/`（与桌面副本 `Desktop\explorer-linux` 同步）
- 归档包：`Desktop\explorer-linux-archive-2026-08-18.zip`（含全部内容）
- 版本：v0.0.2（无代码，纯骨架 + 文档）
