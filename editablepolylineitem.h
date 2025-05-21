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
    EditablePolylineItem(const QVector<QPointF>& points, QGraphicsItem *parent = nullptr); // 构造函数
    ~EditablePolylineItem(); // 析构函数

    void updatePoint(int index, const QPointF& newPoint); // 更新顶点位置
    void setSelectedState(bool selected); // 设置选中状态，控制端点可见性
    QVector<HandleItem*> getHandles() const { return handles; } // 获取所有端点
    virtual EditablePolylineItem* clone() const; // 克隆折线对象
    bool isClosed() const { return isClosed_; } // 获取闭合状态
    void setClosed(bool closed); // 设置闭合状态
    QPen pen() const { return linePen; } // 获取画笔
    void setPen(const QPen &pen); // 设置画笔

    // QGraphicsItem 接口实现
    QRectF boundingRect() const override; // 获取边界矩形
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override; // 绘制折线

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override; // 项变化处理

private:
    void createHandles(); // 创建端点
    void updateHandlesPosition(); // 更新端点位置

    QVector<QPointF> points; // 折线顶点列表
    QVector<HandleItem*> handles; // 顶点控制端点列表
    QPen linePen; // 折线画笔样式
    bool isClosed_ = false; // 折线是否闭合
};

#endif // EDITABLEPOLYLINEITEM_H
