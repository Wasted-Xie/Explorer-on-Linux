#include "StartButton.h"
#include <QApplication>
#include <QDebug>

namespace explorer::components {

StartButton::StartButton(QWidget* parent)
    : QPushButton(parent) {
    setupUI();
    setFixedSize(40, 30);
    setToolTip("开始菜单");
    // Use a start menu icon from theme
    setIcon(QIcon::fromTheme("start-here"));
}

StartButton::~StartButton() = default;

void StartButton::setupUI() {
    // No context menu; left-click will emit clicked() signal
    // Optionally set flat style
    setFlat(true);
}

} // namespace explorer::components

#include "StartButton.moc"