# 第三方项目接入指南

## 原则：零注册、零改造、热键桥接

外部项目保持完全独立，本项目不抢其快捷键、不要求任何接口实现。

## 接入方式（L0 外部程序）

在 hotkeys.conf 中配置一行映射：

```ini
Ctrl+Shift+Esc = /usr/local/bin/linux-regedit
```

- 不要求 DBus 接口
- 不要求 manifest 注册（可选）
- 不要求源码在仓库内
