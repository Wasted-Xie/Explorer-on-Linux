#pragma once

#include <QPushButton>

namespace explorer::components {

class StartButton : public QPushButton {
    Q_OBJECT
public:
    explicit StartButton(QWidget* parent = nullptr);
    ~StartButton() override;

private:
    void setupUI();
};

} // namespace explorer::components

#include "StartButton.moc"