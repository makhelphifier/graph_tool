#include "editablelineitem.h"
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QCursor>
#include <QPen>

EditableLineItem::EditableLineItem(QPointF startPoint, QPointF endPoint, QGraphicsItem *parent)
    : QGraphicsLineItem(startPoint.x(), startPoint.y(), endPoint.x(), endPoint.y(), parent),
    rotationAngle(0), // 初始化旋转角度
    rotationHandleOffset(20.0) // 初始化旋转端点偏移
{
    createHandles(); // 创建控制端点
    updateHandlesPosition(); // 更新端点位置
    setFlags(ItemIsSelectable | ItemIsMovable); // 设置可选择和可移动
    setAcceptHoverEvents(true); // 接受悬停事件
}

EditableLineItem::~EditableLineItem()
{
    // 清理端点，确保未绑定到其他对象时删除
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
    // 创建新直线对象，复制起始点和结束点
    EditableLineItem* newItem = new EditableLineItem(line().p1(), line().p2());

    // 复制通用属性
    newItem->setPos(pos()); // 位置
    newItem->setRotation(rotation()); // 旋转
    newItem->setScale(scale()); // 缩放
    newItem->setTransform(transform()); // 变换
    newItem->setFlags(flags()); // 标志位
    newItem->setEnabled(isEnabled()); // 启用状态
    newItem->setVisible(isVisible()); // 可见性
    newItem->setZValue(zValue()); // Z值

    // 复制画笔属性
    newItem->setPen(pen());

    // 复制自定义属性
    newItem->rotationAngle = this->rotationAngle; // 旋转角度
    newItem->rotationHandleOffset = this->rotationHandleOffset; // 旋转端点偏移
    newItem->updateHandlesPosition(); // 更新新对象的端点位置

    qDebug() << "复制了一个 EditableLineItem 对象。"; // 打印复制信息
    return newItem;
}

void EditableLineItem::rotate(qreal angle, const QPointF& rotationCenterScene)
{
    // 获取当前线段的场景坐标
    QLineF currentLine = line();
    QPointF startPoint = mapToScene(currentLine.p1());
    QPointF endPoint = mapToScene(currentLine.p2());

    // 计算角度差
    qreal angleDiff = angle - rotationAngle;

    // 创建旋转变换
    QTransform rotationTransform;
    rotationTransform.translate(rotationCenterScene.x(), rotationCenterScene.y());
    rotationTransform.rotate(angleDiff);
    rotationTransform.translate(-rotationCenterScene.x(), -rotationCenterScene.y());

    // 计算旋转后新位置
    QPointF newStartPoint = rotationTransform.map(startPoint);
    QPointF newEndPoint = rotationTransform.map(endPoint);

    // 转换回本地坐标
    QPointF newStartLocal = mapFromScene(newStartPoint);
    QPointF newEndLocal = mapFromScene(newEndPoint);

    // 更新线段位置
    setLine(newStartLocal.x(), newStartLocal.y(), newEndLocal.x(), newEndLocal.y());

    // 更新旋转角度
    rotationAngle = angle;

    // 保存旋转端点本地位置
    QPointF rotationHandleLocalPos = rotationHandle->pos();

    // 更新其他端点位置
    updateHandlesPosition();

    // 恢复旋转端点位置，确保拖动时不移动
    rotationHandle->setPos(rotationHandleLocalPos);

    qDebug() << "旋转线段至角度:" << angle << ", 新起点:" << newStartLocal
             << ", 新终点:" << newEndLocal << ", 旋转中心:" << rotationCenterScene; // 打印旋转信息
}

void EditableLineItem::createHandles()
{
    const qreal handleSize = 4.0; // 端点大小
    const qreal handleRadius = handleSize / 2.0; // 端点半径

    // 创建起点端点
    startHandle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
    startHandle->setPen(QPen(Qt::gray, 0)); // 灰色边框
    startHandle->setBrush(Qt::gray); // 灰色填充
    startHandle->setFlag(QGraphicsItem::ItemIsMovable, true); // 可移动
    startHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true); // 忽略变换
    startHandle->setVisible(false); // 初始不可见
    startHandle->setCursor(Qt::SizeAllCursor); // 设置光标样式

    // 创建终点端点
    endHandle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
    endHandle->setPen(QPen(Qt::gray, 0)); // 灰色边框
    endHandle->setBrush(Qt::gray); // 灰色填充
    endHandle->setFlag(QGraphicsItem::ItemIsMovable, true); // 可移动
    endHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true); // 忽略变换
    endHandle->setVisible(false); // 初始不可见
    endHandle->setCursor(Qt::SizeAllCursor); // 设置光标样式

    // 创建旋转端点
    rotationHandle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
    rotationHandle->setPen(QPen(Qt::blue, 0)); // 蓝色边框
    rotationHandle->setBrush(Qt::blue); // 蓝色填充
    rotationHandle->setFlag(QGraphicsItem::ItemIsMovable, true); // 可移动
    rotationHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true); // 忽略变换
    rotationHandle->setVisible(false); // 初始不可见
    rotationHandle->setCursor(Qt::CrossCursor); // 设置光标样式

    updateLine(line().p1(), line().p2()); // 更新线段
}

void EditableLineItem::updateHandlesPosition()
{
    QLineF lineData = line();
    startHandle->setPos(lineData.p1()); // 更新起点位置
    endHandle->setPos(lineData.p2()); // 更新终点位置

    // 计算中点
    QPointF midPoint = (lineData.p1() + lineData.p2()) / 2.0;

    // 计算方向向量
    QPointF direction = lineData.p2() - lineData.p1();
    // 计算垂直方向（中垂线，旋转90度）
    QPointF perpendicularDirection(-direction.y(), direction.x());
    // 归一化并缩放至指定偏移
    qreal length = qSqrt(perpendicularDirection.x() * perpendicularDirection.x() + perpendicularDirection.y() * perpendicularDirection.y());
    if (length > 0) {
        perpendicularDirection = perpendicularDirection * (rotationHandleOffset / length);
    } else {
        perpendicularDirection = QPointF(0, -rotationHandleOffset); // 线段长度为0时默认向上偏移
    }

    // 确保“向上”方向（Qt坐标系y轴向下）
    if (perpendicularDirection.y() > 0) {
        perpendicularDirection = -perpendicularDirection; // 反转方向
    }

    // 设置旋转端点位置
    QPointF rotationPos = midPoint + perpendicularDirection;
    rotationHandle->setPos(rotationPos);
}

QVariant EditableLineItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        return QVariant(); // 阻止默认位置变更处理
    } else if (change == ItemPositionHasChanged) {
        updateHandlesPosition(); // 位置变更时更新端点
    }
    return QGraphicsLineItem::itemChange(change, value);
}

void EditableLineItem::updateLine(QPointF newStartScene, QPointF newEndScene)
{
    // 场景坐标转本地坐标
    QPointF newStartLocal = mapFromScene(newStartScene);
    QPointF newEndLocal = mapFromScene(newEndScene);

    // 更新线段位置
    setLine(newStartLocal.x(), newStartLocal.y(), newEndLocal.x(), newEndLocal.y());
    startHandle->setPos(newStartLocal); // 更新起点端点位置
    endHandle->setPos(newEndLocal); // 更新终点端点位置
    qDebug() << "更新线段: 起点=" << newStartLocal << ", 终点=" << newEndLocal; // 打印更新信息
    qDebug() << "起点端点位置=" << startHandle->pos() << ", 终点端点位置=" << endHandle->pos(); // 打印端点位置
}

void EditableLineItem::handleMoved()
{
    // 端点移动处理（暂空）
}

void EditableLineItem::setSelectedState(bool selected)
{
    startHandle->setVisible(selected); // 设置起点端点可见性
    endHandle->setVisible(selected); // 设置终点端点可见性
    rotationHandle->setVisible(selected); // 设置旋转端点可见性
}

void EditableLineItem::setStartHandle(HandleItem *handle)
{
    // 设置起点端点（暂空）
}

void EditableLineItem::setEndHandle(HandleItem *handle)
{
    // 设置终点端点（暂空）
}

void EditableLineItem::setRotationHandle(HandleItem *handle)
{
    // 设置旋转端点（暂空）
}

qreal EditableLineItem::angleFromPoint(const QPointF& origin, const QPointF& point)
{
    QPointF vector = point - origin; // 计算向量
    qreal angleRadians = qAtan2(vector.y(), vector.x()); // 计算弧度角度
    qreal angleDegrees = qRadiansToDegrees(angleRadians); // 转成度数
    QLineF line(origin, point);
    return line.angle(); // 返回线段角度
}
