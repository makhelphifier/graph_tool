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
    EditableLineItem(QPointF startPoint, QPointF endPoint, QGraphicsItem *parent = nullptr); // 构造函数
    ~EditableLineItem(); // 析构函数
    void updateLine(QPointF newStart, QPointF newEnd); // 更新直线位置
    void rotate(qreal angle, const QPointF& rotationCenterScened); // 旋转直线
    void handleMoved(); // 端点移动处理
    void setSelectedState(bool selected); // 设置选中状态，控制端点可见性
    HandleItem* getStartHandle() const { return startHandle; } // 获取起点端点
    HandleItem* getEndHandle() const { return endHandle; } // 获取终点端点
    HandleItem* getRotationHandle() const { return rotationHandle; } // 获取旋转端点

    void setStartHandle(HandleItem* handle); // 设置起点端点
    void setEndHandle(HandleItem* handle); // 设置终点端点
    void setRotationHandle(HandleItem* handle); // 设置旋转端点

    virtual EditableLineItem* clone() const; // 克隆直线对象
    qreal angleFromPoint(const QPointF& origin, const QPointF& point); // 计算角度

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override; // 项变化处理

private:
    void createHandles(); // 创建端点
    void updateHandlesPosition(); // 更新端点位置

    HandleItem *startHandle = nullptr; // 起点端点
    HandleItem *endHandle = nullptr; // 终点端点
    HandleItem *rotationHandle = nullptr; // 旋转端点
    qreal rotationAngle; // 旋转角度
    qreal rotationHandleOffset; // 旋转端点偏移
};

#endif // EDITABLELINEITEM_H
