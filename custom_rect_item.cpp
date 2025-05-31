#include "custom_rect_item.h"
#include <QPainter>

CustomRectItem::CustomRectItem(const QRectF &rect, QGraphicsItem *parent)
    : QGraphicsRectItem(rect, parent)
{
}

void CustomRectItem::setFillPixmap(const QPixmap &pixmap)
{
    fillPixmap = pixmap;
}

void CustomRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 绘制边框
    painter->setPen(pen());
    painter->drawRect(rect());

    // 如果有填充图片，绘制拉伸后的图片
    if (!fillPixmap.isNull()) {
        painter->drawPixmap(rect().toRect(), fillPixmap);
    } else if (brush().style() != Qt::NoBrush) {
        // 否则使用默认的刷子填充（颜色）
        painter->setBrush(brush());
        painter->fillRect(rect(), brush());
    }
}
