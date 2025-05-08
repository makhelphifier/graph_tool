#include "handleitem.h"
#include "editablelineitem.h"

QVariant HandleItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged) {
        qDebug() << "HandleItem position changed";
        EditableLineItem* lineItem = dynamic_cast<EditableLineItem*>(parentItem());
        if (lineItem) {
            qDebug() << "Calling handleMoved for EditableLineItem";
            lineItem->handleMoved();
        } else {
            qDebug() << "Failed to cast parentItem to EditableLineItem";
        }
    }
    return QGraphicsEllipseItem::itemChange(change, value);

}
HandleItem::HandleItem(qreal x, qreal y, qreal width, qreal height,
                       QGraphicsItem *parent)
    : QGraphicsEllipseItem(x, y, width, height, parent) {

}
