#ifndef HANDLE_ITEM_H
#define HANDLE_ITEM_H

#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QVariant>

class EditableLineItem;

class HandleItem : public QGraphicsEllipseItem
{
public:
    HandleItem(qreal x, qreal y, qreal width, qreal height,
               QGraphicsItem *parent = nullptr);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};

#endif // HANDLE_ITEM_H
