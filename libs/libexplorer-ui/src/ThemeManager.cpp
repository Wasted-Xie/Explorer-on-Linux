#include "ThemeManager.h"
#include <QPalette>
#include <QColor>
#include <QGui/QGuiApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace explorer::ui {

class ThemeManager::Impl {
public:
    Impl() {
        loadDefaultThemes();
        m_currentTheme = ThemeType::Light; // 默认亮色主题
        applyTheme(m_currentTheme);
    }

    void loadDefaultThemes() {
        // 亮色主题
        m_themes[ThemeType::Light] = {
            {ColorRole::Window, QColor(255, 255, 255)},
            {ColorRole::WindowText, QColor(0, 0, 0)},
            {ColorRole::Base, QColor(255, 255, 255)},
            {ColorRole::AlternateBase, QColor(240, 240, 240)},
            {ColorRole::Text, QColor(0, 0, 0)},
            {ColorRole::Button, QColor(240, 240, 240)},
            {ColorRole::ButtonText, QColor(0, 0, 0)},
            {ColorRole::BrightText, QColor(255, 0, 0)},
            {ColorRole::Light, QColor(255, 255, 255)},
            {ColorRole::Midlight, QColor(225, 225, 225)},
            {ColorRole::Dark, QColor(128, 128, 128)},
            {ColorRole::Mid, QColor(150, 150, 150)},
            {ColorRole::Shadow, QColor(100, 100, 100)},
            {ColorRole::Highlight, QColor(42, 130, 218)},
            {ColorRole::HighlightedText, QColor(255, 255, 255)},
            {ColorRole::Link, QColor(0, 0, 255)},
            {ColorRole::LinkVisited, QColor(128, 0, 128)}
        };

        // 暗色主题
        m_themes[ThemeType::Dark] = {
            {ColorRole::Window, QColor(45, 45, 45)},
            {ColorRole::WindowText, QColor(255, 255, 255)},
            {ColorRole::Base, QColor(55, 55, 55)},
            {ColorRole::AlternateBase, QColor(60, 60, 60)},
            {ColorRole::Text, QColor(255, 255, 255)},
            {ColorRole::Button, QColor(60, 60, 60)},
            {ColorRole::ButtonText, QColor(255, 255, 255)},
            {ColorRole::BrightText, QColor(255, 85, 85)},
            {ColorRole::Light, QColor(80, 80, 80)},
            {ColorRole::Midlight, QColor(100, 100, 100)},
            {ColorRole::Dark, QColor(30, 30, 30)},
            {ColorRole::Mid, QColor(50, 50, 50)},
            {ColorRole::Shadow, QColor(20, 20, 20)},
            {ColorRole::Highlight, QColor(42, 130, 218)},
            {ColorRole::HighlightedText, QColor(255, 255, 255)},
            {ColorRole::Link, QColor(85, 170, 255)},
            {ColorRole::LinkVisited, QColor(170, 85, 255)}
        };

        // 高对比度主题
        m_themes[ThemeType::HighContrast] = {
            {ColorRole::Window, QColor(0, 0, 0)},
            {ColorRole::WindowText, QColor(255, 255, 255)},
            {ColorRole::Base, QColor(0, 0, 0)},
            {ColorRole::AlternateBase, QColor(0, 0, 0)},
            {ColorRole::Text, QColor(255, 255, 255)},
            {ColorRole::Button, QColor(0, 0, 0)},
            {ColorRole::ButtonText, QColor(255, 255, 255)},
            {ColorRole::BrightText, QColor(255, 255, 0)},
            {ColorRole::Light, QColor(255, 255, 255)},
            {ColorRole::Midlight, QColor(128, 128, 128)},
            {ColorRole::Dark, QColor(0, 0, 0)},
            {ColorRole::Mid, QColor(0, 0, 0)},
            {ColorRole::Shadow, QColor(0, 0, 0)},
            {ColorRole::Highlight, QColor(255, 255, 0)},
            {ColorRole::HighlightedText, QColor(0, 0, 0)},
            {ColorRole::Link, QColor(255, 255, 0)},
            {ColorRole::LinkVisited, QColor(255, 0, 255)}
        };
    }

    void applyTheme(ThemeType type) {
        auto it = m_themes.find(type);
        if (it == m_themes.end()) return;

        QPalette palette;
        for (const auto& [role, color] : it->second) {
            switch (role) {
                case ColorRole::Window: palette.setColor(QPalette::Window, color); break;
                case ColorRole::WindowText: palette.setColor(QPalette::WindowText, color); break;
                case ColorRole::Base: palette.setColor(QPalette::Base, color); break;
                case ColorRole::AlternateBase: palette.setColor(QPalette::AlternateBase, color); break;
                case ColorRole::Text: palette.setColor(QPalette::Text, color); break;
                case ColorRole::Button: palette.setColor(QPalette::Button, color); break;
                case ColorRole::ButtonText: palette.setColor(QPalette::ButtonText, color); break;
                case ColorRole::BrightText: palette.setColor(QPalette::BrightText, color); break;
                case ColorRole::Light: palette.setColor(QPalette::Light, color); break;
                case ColorRole::Midlight: palette.setColor(QPalette::Midlight, color); break;
                case ColorRole::Dark: palette.setColor(QPalette::Dark, color); break;
                case ColorRole::Mid: palette.setColor(QPalette::Mid, color); break;
                case ColorRole::Shadow: palette.setColor(QPalette::Shadow, color); break;
                case ColorRole::Highlight: palette.setColor(QPalette::Highlight, color); break;
                case ColorRole::HighlightedText: palette.setColor(QPalette::HighlightedText, color); break;
                case ColorRole::Link: palette.setColor(QPalette::Link, color); break;
                case ColorRole::LinkVisited: palette.setColor(QPalette::LinkVisited, color); break;
            }
        }
        
        m_currentPalette = palette;
        QGuiApplication::setPalette(palette);
    }

    ThemeManager* q;
    ThemeType m_currentTheme = ThemeType::Light;
    QPalette m_currentPalette;
    QMap<ThemeType, QMap<ColorRole, QColor>> m_themes;
    IconProvider* m_iconProvider = nullptr;
};

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager() : d(std::make_unique<Impl>(this)) {}
ThemeManager::~ThemeManager() = default;

bool ThemeManager::init() {
    return d->init();
}

void ThemeManager::setTheme(ThemeType type) {
    if (d->m_currentTheme == type) return;
    d->m_currentTheme = type;
    d->applyTheme(type);
    emit themeChanged(type);
    emit colorsChanged();
}

ThemeType ThemeManager::currentTheme() const {
    return d->m_currentTheme;
}

QColor ThemeManager::color(ColorRole role) const {
    auto themeIt = d->m_themes.find(d->m_currentTheme);
    if (themeIt == d->m_themes.end()) return QColor();
    
    auto colorIt = themeIt->second.find(role);
    return colorIt != themeIt->second.end() ? colorIt->second : QColor();
}

QPalette ThemeManager::palette() const {
    return d->m_currentPalette;
}

void ThemeManager::setIconProvider(IconProvider* provider) {
    d->m_iconProvider = provider;
}

ThemeManager::IconProvider* ThemeManager::iconProvider() const {
    return d->m_iconProvider;
}

} // namespace explorer::ui

#include "ThemeManager.moc"