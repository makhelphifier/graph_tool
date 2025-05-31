#include "editable_line_item.h"
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QCursor>
#include <QPen>
EditableLineItem::EditableLineItem(QPointF startPoint, QPointF endPoint, QGraphicsItem *parent)
    : QGraphicsLineItem(startPoint.x(), startPoint.y(), endPoint.x(), endPoint.y(), parent),
    rotationAngle(0),
    rotationHandleOffset(20.0)
{
    createHandles();
    updateHandlesPosition();
    setFlags(ItemIsSelectable | ItemIsMovable);
    setAcceptHoverEvents(true);
}

EditableLineItem::~EditableLineItem()
{
    if (startHandle && startHandle->parentItem() != this) {
        delete startHandle;
        startHandle = nullptr;
    }
    if (endHandle && endHandle->parentItem() != this) {
        delete endHandle;
        endHandle = nullptr;
    }
    if (rotationHandle && rotationHandle->parentItem() != this) {
        delete rotationHandle;
        rotationHandle = nullptr;
    }
}

EditableLineItem* EditableLineItem::clone() const
{
    // 创建一个新的 EditableLineItem 实例
    // 复制线条的起始点和结束点
    EditableLineItem* newItem = new EditableLineItem(line().p1(), line().p2());

    // 复制通用的 QGraphicsItem 属性
    newItem->setPos(pos());
    newItem->setRotation(rotation());
    newItem->setScale(scale());
    newItem->setTransform(transform());
    // 复制标志位，但要小心，某些标志位可能不适合复制
    newItem->setFlags(flags());
    newItem->setEnabled(isEnabled());
    newItem->setVisible(isVisible());
    newItem->setZValue(zValue());

    // 复制线条的画笔属性
    newItem->setPen(pen());

    // 复制自定义属性，如旋转角度
    newItem->rotationAngle = this->rotationAngle;
    newItem->rotationHandleOffset = this->rotationHandleOffset;
    // 更新新项的控制柄位置以匹配其状态
    newItem->updateHandlesPosition();

    qDebug() << "Cloned an EditableLineItem.";
    return newItem;
}
void EditableLineItem::rotate(qreal angle, const QPointF& rotationCenterScene)
{
    // 获取当前线段的起点和终点（场景坐标）
    QLineF currentLine = line();
    QPointF startPoint = mapToScene(currentLine.p1());
    QPointF endPoint = mapToScene(currentLine.p2());

    // 获取之前的旋转角度
    qreal angleDiff = angle - rotationAngle;

    // 围绕旋转中心进行旋转变换
    QTransform rotationTransform;
    rotationTransform.translate(rotationCenterScene.x(), rotationCenterScene.y());
    rotationTransform.rotate(angleDiff);
    rotationTransform.translate(-rotationCenterScene.x(), -rotationCenterScene.y());

    // 计算旋转后的新位置
    QPointF newStartPoint = rotationTransform.map(startPoint);
    QPointF newEndPoint = rotationTransform.map(endPoint);

    // 将新位置转换回本地坐标
    QPointF newStartLocal = mapFromScene(newStartPoint);
    QPointF newEndLocal = mapFromScene(newEndPoint);

    // 更新线段位置
    setLine(newStartLocal.x(), newStartLocal.y(), newEndLocal.x(), newEndLocal.y());

    // 更新旋转角度
    rotationAngle = angle;

    // 临时保存旋转柄的本地坐标位置
    QPointF rotationHandleLocalPos = rotationHandle->pos();

    // 更新其他控制柄位置
    updateHandlesPosition();

    // 恢复旋转柄位置，确保在拖动过程中不移动
    rotationHandle->setPos(rotationHandleLocalPos);

    qDebug() << "Rotated line to angle:" << angle << ", New start:" << newStartLocal
             << ", New end:" << newEndLocal << ", Rotation center:" << rotationCenterScene;
}


void EditableLineItem::createHandles()
{
    const qreal handleSize = 4.0;
    const qreal handleRadius = handleSize / 2.0;
    startHandle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
    startHandle->setPen(QPen(Qt::gray, 0));
    startHandle->setBrush(Qt::gray);
    startHandle->setFlag(QGraphicsItem::ItemIsMovable, true);
    startHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    startHandle->setVisible(false);
    startHandle->setCursor(Qt::SizeAllCursor);

    endHandle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
    endHandle->setPen(QPen(Qt::gray, 0));
    endHandle->setBrush(Qt::gray);
    endHandle->setFlag(QGraphicsItem::ItemIsMovable, true);
    endHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    endHandle->setVisible(false);
    endHandle->setCursor(Qt::SizeAllCursor);


    rotationHandle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
    rotationHandle->setPen(QPen(Qt::blue, 0));
    rotationHandle->setBrush(Qt::blue);
    rotationHandle->setFlag(QGraphicsItem::ItemIsMovable, true);
    rotationHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    rotationHandle->setVisible(false);
    rotationHandle->setCursor(Qt::CrossCursor);

    updateLine(line().p1(), line().p2());

}


void EditableLineItem::updateHandlesPosition()
{
    QLineF lineData = line();
    startHandle->setPos(lineData.p1());
    endHandle->setPos(lineData.p2());

    // 计算线段的中点
    QPointF midPoint = (lineData.p1() + lineData.p2()) / 2.0;

    // 计算线段的方向向量
    QPointF direction = lineData.p2() - lineData.p1();
    // 计算垂直于线段的方向（中垂线方向，旋转90度）
    QPointF perpendicularDirection(-direction.y(), direction.x());
    // 归一化垂直向量并缩放到20px
    qreal length = qSqrt(perpendicularDirection.x() * perpendicularDirection.x() + perpendicularDirection.y() * perpendicularDirection.y());
    if (length > 0) {
        perpendicularDirection = perpendicularDirection * (rotationHandleOffset / length);
    } else {
        perpendicularDirection = QPointF(0, -rotationHandleOffset); // 如果线段长度为0，默认向上偏移
    }

    // 确定“向上”方向：在Qt坐标系中，y轴向下，所以“向上”通常是y值减小的方向
    // 如果perpendicularDirection的y值大于0（向下），则反转方向以确保“向上”
    if (perpendicularDirection.y() > 0) {
        perpendicularDirection = -perpendicularDirection;
    }

    // 将旋转端点放置在中点加上垂直偏移的位置
    QPointF rotationPos = midPoint + perpendicularDirection;
    rotationHandle->setPos(rotationPos);
}



QVariant EditableLineItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        return QVariant(); // Prevent default handling of this specific ItemPositionChange
    } else if (change == ItemPositionHasChanged) {
        updateHandlesPosition();
    }
    return QGraphicsLineItem::itemChange(change, value);
}
void EditableLineItem::updateLine(QPointF newStartScene, QPointF newEndScene)
{
    // 将场景坐标转换为本地坐标
    QPointF newStartLocal = mapFromScene(newStartScene);
    QPointF newEndLocal = mapFromScene(newEndScene);

    setLine(newStartLocal.x(), newStartLocal.y(), newEndLocal.x(), newEndLocal.y());
    startHandle->setPos(newStartLocal);
    endHandle->setPos(newEndLocal);
    qDebug() << "Updated line: Start=" << newStartLocal << ", End=" << newEndLocal;
    qDebug() << "Start handle pos=" << startHandle->pos() << ", End handle pos=" << endHandle->pos();
}


void EditableLineItem::handleMoved()
{

}

void EditableLineItem::setSelectedState(bool selected)
{
    startHandle->setVisible(selected);
    endHandle->setVisible(selected);
    rotationHandle->setVisible(selected);
}

void EditableLineItem::setStartHandle(HandleItem *handle)
{

}

void EditableLineItem::setEndHandle(HandleItem *handle)
{

}

void EditableLineItem::setRotationHandle(HandleItem *handle)
{

}


qreal EditableLineItem::angleFromPoint(const QPointF& origin, const QPointF& point)
{
    QPointF vector = point - origin;
    qreal angleRadians = qAtan2(vector.y(), vector.x());
    qreal angleDegrees = qRadiansToDegrees(angleRadians);
    QLineF line(origin, point);
    return line.angle();
}
