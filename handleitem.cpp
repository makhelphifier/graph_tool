#include "handleitem.h"
#include "editablelineitem.h"

QVariant HandleItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        qDebug() << "端点位置已变更"; // 打印位置变更信息
        EditableLineItem* lineItem = dynamic_cast<EditableLineItem*>(parentItem()); // 尝试转换父项为直线项
        if (lineItem) {
            qDebug() << "调用 EditableLineItem 的 handleMoved 方法"; // 打印调用信息
            lineItem->handleMoved(); // 调用处理方法
        } else {
            qDebug() << "无法将父项转换为 EditableLineItem"; // 打印转换失败信息
        }
    }
    return QGraphicsEllipseItem::itemChange(change, value); // 调用基类方法
}

HandleItem::HandleItem(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent)
    : QGraphicsEllipseItem(x, y, width, height, parent) // 构造函数，初始化椭圆项
{
    // 构造函数逻辑为空，仅初始化基类
}
