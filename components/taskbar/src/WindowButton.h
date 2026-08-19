#pragma once

#include <QPushButton>
#include <libexplorer-ui/BaseWidget.h>
#include <QString>
#include <QIcon>

namespace explorer::components {

class WindowButton : public explorer::ui::BaseWidget {
    Q_OBJECT
public:
    explicit WindowButton(const QString& windowId, const QString& title, const QIcon& icon = QIcon(), QWidget* parent = nullptr);
    ~WindowButton() override;

    void setWindowTitle(const QString& title);
    QString windowTitle() const;
    void setWindowIcon(const QIcon& icon);
    QIcon windowIcon() const;
    void setIsActive(bool active);
    bool isActive() const;
    
    // 窗口 ID（唯一标识符）
    QString windowId() const { return m_windowId; }
    void setWindowId(const QString& id) { m_windowId = id; }

signals:
    void windowClicked(const QString& windowId);

private:
    QString m_windowId;
    QString m_title;
    QIcon m_icon;
    bool m_isActive = false;
};

} // namespace explorer::components

#include "WindowButton.moc"