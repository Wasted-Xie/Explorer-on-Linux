#pragma once

#include <QListView>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPoint>

namespace explorer::desktop {

class DesktopView : public QListView {
    Q_OBJECT
public:
    explicit DesktopView(QAbstractItemModel* model = nullptr, QWidget* parent = nullptr);
    ~DesktopView() override;

    // 设置图标大小
    void setIconSize(const QSize& size);
    QSize iconSize() const;

    // 设置网格大小
    void setGridSize(const QSize& size);
    QSize gridSize() const;

    // 设置间距
    void setSpacing(int spacing);
    int spacing() const;

signals:
    // 自定义信号，传递更多上下文信息
    void itemActivated(const QModelIndex& index);
    void itemContextMenuRequested(const QPoint& pos);

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QPoint m_dragStartPosition;
    bool m_dragging = false;
    QModelIndex m_dragIndex;
};

} // namespace explorer::desktop