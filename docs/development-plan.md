# 开发计划

> 更新于 2026-08-14。本文档与 README「开发进度」章节同步。

## 路线图

```
阶段0 spike 验证 ──→ 阶段1 地基 ──→ 阶段2 filemanager ──→ 阶段3 shell 组件
                                                              │
                    阶段6 发布 ←── 阶段5 外部接入 ←── 阶段4 系统集成
```

## 阶段 0：技术验证（spike）

| Spike | 验证内容 | 验收标准 |
|-------|---------|---------|
| S1 | KWin layer-shell：面板贴边/背景层/键盘独占 | 跑通最小 layer-shell 面板 |
| S2 | foreign-toplevel：窗口列表/激活状态/标题/图标 | 拿到窗口列表 |
| S3 | kglobalaccel：热键注册/冲突处理 | 注册 + 触发成功 |

- 产出：docs/compositor-notes.md 实测记录 + ADR-001 合成器选型
- 决策点：S1/S2 支持不完整时的降级方案（KWin DBus / wlr-layer-shell）

## 阶段 1：地基（libs 层）

- 顶层 CMake 全链路编译通过 + CI（构建 + 单测）
- libexplorer-utils（日志/单实例）+ libexplorer-core（fs 抽象/配置）
- 验收：cmake --build 通过，core 单测全绿

## 阶段 2：filemanager MVP（里程碑 M1）

| 周 | 交付 |
|----|------|
| W1 | 主窗口三栏 + 导航树 + 文件列表 + 面包屑 + 前进后退 |
| W2 | 多选/拖放/右键菜单/属性框/F2 重命名 |
| W3 | 标签页/搜索/回收站/压缩 |
| W4 | Win11 主题（深浅色 QSS）+ 打磨 |

- 验收：可独立运行的日常可用文件管理器（不依赖 shell）

## 阶段 3：shell 组件（里程碑 M2）

- taskbar（layer-shell + foreign-toplevel 取窗口）
- startmenu（.desktop 解析 + 固定项 + 搜索）
- desktop（壁纸 + 图标 + 右键）
- run-dialog + 全局热键打通（Win+E/R/D）
- 验收：能组成一个可用桌面会话

## 阶段 4：系统集成（里程碑 M3）

- notification-center（XDG notification + SNI 托盘）
- search 全局搜索
- shell-daemon 完整化（监督/会话管理）
- 验收：日常可当主 DE 使用

## 阶段 5：外部组件接入

- 按 docs/external-integration.md 与第三方作者沟通（regedit、任务管理器等）
- 热键桥接配置（hotkeys.conf）
- 验收：外部组件通过热键启动，无冲突

## 阶段 6：打磨与发布（里程碑 M4）

- 锁屏、贴靠微调、动画
- 性能/稳定性
- 打包（AppImage/.deb）

## 横切规则

1. 每阶段文档留痕：ADR 记录决策，README 同步更新
2. 单命令单职责：一次只做一件事，做完立即验证
3. 版本管理：v0.1.0 起步，每里程碑升版本号 + 备份
4. 可运行优先：每阶段结束都有可运行产物

## 当前进度

- ✅ 路线选型（B 路线 / KWin / 热键桥接）
- ✅ 项目结构梳理 + 骨架落地（2026-08-14）
- ⏳ 阶段 0 spike（下一步）
