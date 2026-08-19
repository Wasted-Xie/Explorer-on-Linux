#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QIcon>
#include <QFontMetrics>

#include "SearchResult.h"

namespace explorer::search {

// 搜索结果自定义委托
class SearchDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit SearchDelegate(QObject* parent = nullptr);
    ~SearchDelegate() override;

    // 绘制项目
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    // 项目大小
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    // 设置主题颜色
    void setColors(const QColor& background, const QColor& hoverBackground,
                   const QColor& selectedBackground, const QColor& textColor,
                   const QColor& descriptionColor);

private:
    struct Colors {
        QColor background;
        QColor hoverBackground;
        QColor selectedBackground;
        QColor textColor;
        QColor descriptionColor;
        QColor separatorColor;
    } m_colors;

    int m_itemHeight = 56;
    int m_iconSize = 32;
    int m_padding = 12;
    int m_spacing = 8;
    mutable QFont m_nameFont;
    mutable QFont m_descFont;
};

} // namespace explorer::search