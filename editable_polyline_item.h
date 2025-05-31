#ifndef EDITABLE_POLYLINE_ITEM_H
#define EDITABLE_POLYLINE_ITEM_H

#include <QGraphicsItem>
#include <QVector>
#include <QPointF>
#include <QVariant>
#include "handle_item.h"
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
    bool isClosed() const { return isClosed_; } // 获取闭合状态
    void setClosed(bool closed); //设置闭合状态
    QPen pen() const { return linePen; } // 获取画笔
    void setPen(const QPen &pen); //设置画笔
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
    bool isClosed_ = false; // 新增：折线是否闭合

};

#endif // EDITABLE_POLYLINE_ITEM_H
