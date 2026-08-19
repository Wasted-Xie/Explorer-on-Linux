#include "SearchDelegate.h"
#include "SearchModel.h"
#include <QApplication>
#include <QStyle>
#include <QDebug>

namespace explorer::search {

SearchDelegate::SearchDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
    // 默认颜色
    m_colors.background = QColor(30, 30, 30, 230);
    m_colors.hoverBackground = QColor(60, 60, 60, 240);
    m_colors.selectedBackground = QColor(0, 120, 215, 200);
    m_colors.textColor = Qt::white;
    m_colors.descriptionColor = QColor(180, 180, 180);
    m_colors.separatorColor = QColor(80, 80, 80);

    // 字体
    m_nameFont = QApplication::font();
    m_nameFont.setPointSize(11);
    m_nameFont.setWeight(QFont::Medium);

    m_descFont = QApplication::font();
    m_descFont.setPointSize(9);
}

SearchDelegate::~SearchDelegate() = default;

void SearchDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const {
    if (!index.isValid()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    QRect rect = option.rect;

    // 获取数据
    QString name = index.data(SearchModel::NameRole).toString();
    QString description = index.data(SearchModel::DescriptionRole).toString();
    QIcon icon = qvariant_cast<QIcon>(index.data(SearchModel::IconRole));
    int type = index.data(SearchModel::TypeRole).toInt();
    double score = index.data(SearchModel::ScoreRole).toDouble();

    bool isSelected = option.state & QStyle::State_Selected;
    bool isHovered = option.state & QStyle::State_MouseOver;

    // 绘制背景
    QColor bgColor = m_colors.background;
    if (isSelected) {
        bgColor = m_colors.selectedBackground;
    } else if (isHovered) {
        bgColor = m_colors.hoverBackground;
    }

    painter->fillRect(rect, bgColor);

    // 绘制分隔线（底部）
    painter->setPen(QPen(m_colors.separatorColor, 1));
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());

    // 图标区域
    int iconX = m_padding;
    int iconY = (rect.height() - m_iconSize) / 2;
    QRect iconRect(iconX, iconY, m_iconSize, m_iconSize);

    if (!icon.isNull()) {
        QPixmap pixmap = icon.pixmap(m_iconSize, m_iconSize);
        painter->drawPixmap(iconRect, pixmap);
    } else {
        // 默认图标占位符
        painter->setPen(QPen(m_colors.textColor, 1));
        painter->setFont(m_nameFont);
        QString placeholder = (type == static_cast<int>(ResultType::Application)) ? "📦" : "📄";
        painter->drawText(iconRect, Qt::AlignCenter, placeholder);
    }

    // 文本区域
    int textX = iconX + m_iconSize + m_spacing;
    int textWidth = rect.width() - textX - m_padding;

    // 名称
    QRect nameRect(textX, rect.top() + m_padding, textWidth, m_nameFont.pointSize() + 4);
    painter->setPen(m_colors.textColor);
    painter->setFont(m_nameFont);

    QString elidedName = painter->fontMetrics().elidedText(name, Qt::ElideRight, textWidth);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // 描述
    if (!description.isEmpty()) {
        QRect descRect(textX, nameRect.bottom() + 2, textWidth, m_descFont.pointSize() + 2);
        painter->setPen(m_colors.descriptionColor);
        painter->setFont(m_descFont);

        QString elidedDesc = painter->fontMetrics().elidedText(description, Qt::ElideRight, textWidth);
        painter->drawText(descRect, Qt::AlignLeft | Qt::AlignVCenter, elidedDesc);
    }

    // 类型标签（可选，显示在右侧）
    if (score > 0) {
        QString typeLabel;
        switch (static_cast<ResultType>(type)) {
            case ResultType::Application: typeLabel = "App"; break;
            case ResultType::File: typeLabel = "File"; break;
            case ResultType::Folder: typeLabel = "Folder"; break;
        }

        if (!typeLabel.isEmpty()) {
            painter->setFont(m_descFont);
            QRect typeRect(rect.right() - m_padding - 50, rect.center().y() - 10, 50, 20);
            painter->setPen(m_colors.descriptionColor);
            painter->drawText(typeRect, Qt::AlignRight | Qt::AlignVCenter, typeLabel);
        }
    }

    painter->restore();
}

QSize SearchDelegate::sizeHint(const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(0, m_itemHeight);
}

void SearchDelegate::setColors(const QColor& background, const QColor& hoverBackground,
                              const QColor& selectedBackground, const QColor& textColor,
                              const QColor& descriptionColor) {
    m_colors.background = background;
    m_colors.hoverBackground = hoverBackground;
    m_colors.selectedBackground = selectedBackground;
    m_colors.textColor = textColor;
    m_colors.descriptionColor = descriptionColor;
}

} // namespace explorer::search

#include "SearchDelegate.moc"