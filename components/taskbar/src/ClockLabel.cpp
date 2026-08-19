#include "ClockLabel.h"
#include <QDateTime>
#include <libexplorer-utils/DateTimeUtils.h>

namespace explorer::components {

ClockLabel::ClockLabel(QWidget* parent)
    : explorer::ui::BaseWidget(parent), timer(new QTimer(this)) {
    setAlignment(Qt::AlignCenter);
    setMinimumWidth(80);
    setToolTip("日期时间");
    timer->setInterval(1000); // update every second
    connect(timer, &QTimer::timeout, this, &ClockLabel::updateTime);
    timer->start();
    updateTime(); // initial update
}

ClockLabel::~ClockLabel() = default;

void ClockLabel::updateTime() {
    // Use DateTimeUtils for formatting if desired, but simple format for now
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    QString dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    setText(QString("%1\n%2").arg(timeStr).arg(dateStr));
    // Optionally set tooltip with full date time
    setToolTip(QDateTime::currentDateTime().toString());
}

} // namespace explorer::components

#include "ClockLabel.moc"