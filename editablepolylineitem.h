#ifndef EDITABLEPOLYLINEITEM_H
#define EDITABLEPOLYLINEITEM_H

#include <QGraphicsItem>
#include <QVector>
#include <QPointF>
#include <QVariant>
#include "handleitem.h"
#include <QPen>
class EditablePolylineItem : public QGraphicsItem
{
public:
    EditablePolylineItem(const QVector<QPointF>& points, QGraphicsItem *parent = nullptr);
    ~EditablePolylineItem();

    void updatePoint(int index, const QPointF& newPoint);
    void setSelectedState(bool selected); // 设置选中状态，控制端点可见性
    QVector<HandleItem*> getHandles() const { return handles; }
    virtual EditablePolylineItem* clone() const;

    // QGraphicsItem 接口实现
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void createHandles();
    void updateHandlesPosition();

    QVector<QPointF> points; // 折线的顶点列表
    QVector<HandleItem*> handles; // 每个顶点的控制端点
    QPen linePen; // 折线的画笔样式
};

#endif // EDITABLEPOLYLINEITEM_H
