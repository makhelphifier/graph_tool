#ifndef HANDLEITEM_H
#define HANDLEITEM_H

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

#endif // HANDLEITEM_H
