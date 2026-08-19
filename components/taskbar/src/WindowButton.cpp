#include "WindowButton.h"
#include <QApplication>
#include <QDebug>

namespace explorer::components {

WindowButton::WindowButton(const QString& windowId, const QString& title, const QIcon& icon, QWidget* parent)
    : explorer::ui::BaseWidget(parent), m_windowId(windowId), m_title(title), m_icon(icon) {
    setCheckable(true);
    setAutoExclusive(false); // Allow multiple buttons to be checked (though we'll manage this)
    setFixedHeight(24);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMinimumWidth(60);
    setMaximumWidth(200);
    setText(title);
    setIcon(icon);
    setToolTip(title);
    // Style will be applied via theme
    setFlat(true);
    connect(this, &QPushButton::clicked, this, [this]() {
        emit windowClicked(m_windowId);
    });
}

WindowButton::~WindowButton() = default;

void WindowButton::setWindowTitle(const QString& title) {
    if (m_title == title) return;
    m_title = title;
    setText(title);
    setToolTip(title);
}

QString WindowButton::windowTitle() const {
    return m_title;
}

void WindowButton::setWindowIcon(const QIcon& icon) {
    if (m_icon == icon) return;
    m_icon = icon;
    setIcon(icon);
}

QIcon WindowButton::windowIcon() const {
    return m_icon;
}

void WindowButton::setIsActive(bool active) {
    if (m_isActive == active) return;
    m_isActive = active;
    // Update appearance based on active state
    // This will be handled by theme/styles
    update();
}

bool WindowButton::isActive() const {
    return m_isActive;
}

} // namespace explorer::components

#include "WindowButton.moc"