#pragma once

#include <QWidget>
#include <QString>
#include <QIcon>
#include <QSize>
#include <QColor>
#include <QPalette>
#include <functional>

namespace explorer::ui {

// 主题管理器
class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    // 主题类型
    enum class ThemeType {
        Light,
        Dark,
        HighContrast
    };

    // 颜色角色
    enum class ColorRole {
        Window,
        WindowText,
        Base,
        AlternateBase,
        Text,
        Button,
        ButtonText,
        BrightText,
        Light,
        Midlight,
        Dark,
        Mid,
        Shadow,
        Highlight,
        HighlightedText,
        Link,
        LinkVisited
    };

    // 初始化
    bool init();
    void setTheme(ThemeType type);
    ThemeType currentTheme() const;

    // 颜色获取
    QColor color(ColorRole role) const;
    QPalette palette() const;

    // 图标提供者
    class IconProvider {
    public:
        virtual QIcon icon(const QString& name) = 0;
        virtual ~IconProvider() = default;
    };

    void setIconProvider(IconProvider* provider);
    IconProvider* iconProvider() const;

signals:
    void themeChanged(ThemeType type);
    void colorsChanged();

private:
    ThemeManager();
    ~ThemeManager();

    class Impl;
    std::unique_ptr<Impl> d;
};

// 基础 UI 组件 - 所有组件的基类
class BaseWidget : public QWidget {
    Q_OBJECT
public:
    explicit BaseWidget(QWidget* parent = nullptr);
    ~BaseWidget() override;

    // 主题相关
    void applyTheme();
    void setAutoTheme(bool enabled);
    bool autoTheme() const;

    // 图标
    void setIcon(const QIcon& icon);
    QIcon icon() const;

    // 提示文本
    void setPlaceholderText(const QString& text);
    QString placeholderText() const;

protected:
    bool m_autoTheme = true;
    QIcon m_icon;
    QString m_placeholderText;
};

} // namespace explorer::ui

#include "ThemeManager.moc"
#include "BaseWidget.moc"