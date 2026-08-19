#pragma once

#include <QWidget>
#include <QIcon>
#include <QString>
#include "libs/libexplorer-ui/src/BaseWidget.h"

namespace explorer::startmenu {

class ApplicationItem : public explorer::ui::BaseWidget {
    Q_OBJECT
public:
    explicit ApplicationItem(const QString& name, const QString& exec, const QIcon& icon = QIcon(), QWidget* parent = nullptr);
    ~ApplicationItem() override;

    QString name() const;
    QString exec() const;
    QIcon icon() const;

    void setName(const QString& name);
    void setExec(const QString& exec);
    void setIcon(const QIcon& icon);

signals:
    void launched(const QString& exec);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::startmenu

#include "ApplicationItem.moc"