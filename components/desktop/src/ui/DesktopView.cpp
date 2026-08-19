#include "DesktopView.h"

#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QDebug>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QVariant>

namespace explorer::desktop {

DesktopView::DesktopView(QAbstractItemModel* model, QWidget* parent)
    : QListView(parent)
{
    setModel(model);

    // 设置视图模式为图标模式
    setViewMode(QListView::IconMode);
    setMovement(QListView::Static);
    setResizeMode(QListView::Adjust);
    setFlow(QListView::LeftToRight);
    setWrapping(true);

    // 设置默认图标和网格大小
    setIconSize(QSize(64, 64));
    setGridSize(QSize(100, 90));
    setSpacing(10);

    // 设置选择模式
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);

    // 启用右键菜单
    setContextMenuPolicy(Qt::CustomContextMenu);

    // 设置均匀的项目大小
    setUniformItemSizes(true);

    // 设置文本省略模式
    setTextElideMode(Qt::ElideRight);

    // 设置字体
    QFont font = this->font();
    font.setPointSize(9);
    setFont(font);

    // 设置样式表
    setStyleSheet(R"(
        QListView {
            background: transparent;
            border: none;
            outline: none;
        }
        QListView::item {
            background: transparent;
            border: none;
            padding: 4px;
        }
        QListView::item:selected {
            background: rgba(255, 255, 255, 30);
            border-radius: 4px;
        }
        QListView::item:hover {
            background: rgba(255, 255, 255, 15);
            border-radius: 4px;
        }
    )");

    // 设置属性
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFocusPolicy(Qt::NoFocus); // 桌面图标不需要焦点
}

DesktopView::~DesktopView() = default;

void DesktopView::setIconSize(const QSize& size) {
    QListView::setIconSize(size);
}

QSize DesktopView::iconSize() const {
    return QListView::iconSize();
}

void DesktopView::setGridSize(const QSize& size) {
    QListView::setGridSize(size);
}

QSize DesktopView::gridSize() const {
    return QListView::gridSize();
}

void DesktopView::setSpacing(int spacing) {
    QListView::setSpacing(spacing);
}

int DesktopView::spacing() const {
    return QListView::spacing();
}

void DesktopView::mouseDoubleClickEvent(QMouseEvent* event) {
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        emit doubleClicked(index);
        emit itemActivated(index);
    }
    QListView::mouseDoubleClickEvent(event);
}

void DesktopView::contextMenuEvent(QContextMenuEvent* event) {
    emit customContextMenuRequested(event->pos());
    emit itemContextMenuRequested(event->pos());
    QListView::contextMenuEvent(event);
}

void DesktopView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
        m_dragIndex = indexAt(event->pos());
    }
    QListView::mousePressEvent(event);
}

void DesktopView::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        int distance = (event->pos() - m_dragStartPosition).manhattanLength();
        if (distance >= QApplication::startDragDistance() && m_dragIndex.isValid() && !m_dragging) {
            startDrag(m_dragIndex);
        }
    }
    QListView::mouseMoveEvent(event);
}

void DesktopView::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    QListView::mouseReleaseEvent(event);
}

void DesktopView::startDrag(const QModelIndex& index) {
    if (!index.isValid() || !model()) {
        return;
    }

    // 获取文件路径
    QString filePath = model()->data(index, Qt::UserRole + 1).toString(); // 假设文件路径存储在 UserRole+1
    if (filePath.isEmpty()) {
        // 尝试从 DisplayRole 获取文件名，然后拼接路径
        QString fileName = model()->data(index, Qt::DisplayRole).toString();
        // 这里需要桌面路径，暂时跳过
        return;
    }

    QMimeData* mimeData = new QMimeData();
    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(filePath));
    mimeData->setUrls(urls);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // 设置拖拽图标
    QPixmap pixmap = qvariant_cast<QPixmap>(model()->data(index, Qt::DecorationRole));
    if (!pixmap.isNull()) {
        drag->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        drag->setHotSpot(QPoint(32, 32));
    }

    m_dragging = true;
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
    m_dragging = false;

    Q_UNUSED(dropAction);
}

void DesktopView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QListView::dragEnterEvent(event);
    }
}

void DesktopView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QListView::dragMoveEvent(event);
    }
}

void DesktopView::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        // 处理文件拖放到桌面
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                QString srcPath = url.toLocalFile();
                qDebug() << "File dropped on desktop:" << srcPath;
                // TODO: 实现复制/移动/链接文件到桌面
            }
        }
        event->acceptProposedAction();
    } else {
        QListView::dropEvent(event);
    }
}

} // namespace explorer::desktop