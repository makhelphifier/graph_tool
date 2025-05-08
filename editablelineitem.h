#ifndef EDITABLELINEITEM_H
#define EDITABLELINEITEM_H

#include <QGraphicsLineItem>
#include <QGraphicsItem>
#include <QPointF>
#include <QVariant>
#include "handleitem.h"

class EditableLineItem : public QGraphicsLineItem
{
public:
    EditableLineItem(QPointF startPoint, QPointF endPoint, QGraphicsItem *parent = nullptr);
    void updateLine(QPointF newStart, QPointF newEnd);
    void rotate(qreal angle);
    void handleMoved();
    void setSelectedState(bool selected); // 新增：设置选中状态，控制端点可见性
    HandleItem* getStartHandle() const { return startHandle; } // 获取起点端点
    HandleItem* getEndHandle() const { return endHandle; } // 获取终点端点
    void setStartHandle(HandleItem* handle); // 设置起点端点
    void setEndHandle(HandleItem* handle);   // 设置终点端点
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void createHandles();
    void updateHandlesPosition(); // 新增：更新端点位置

    HandleItem *startHandle = nullptr;
    HandleItem *endHandle = nullptr;
    QGraphicsEllipseItem *rotateHandle = nullptr;
    qreal rotationAngle;
};

#endif // EDITABLELINEITEM_H
