#pragma once

#include <QLabel>
#include <libexplorer-ui/BaseWidget.h>
#include <QTimer>

namespace explorer::components {

class ClockLabel : public explorer::ui::BaseWidget {
    Q_OBJECT
public:
    explicit ClockLabel(QWidget* parent = nullptr);
    ~ClockLabel() override;

public slots:
    void updateTime();

private:
    QTimer* timer;
};

} // namespace explorer::components

#include "ClockLabel.moc"