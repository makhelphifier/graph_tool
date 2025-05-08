#include "editablelineitem.h"
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QCursor>
#include <QPen>

EditableLineItem::EditableLineItem(QPointF startPoint, QPointF endPoint, QGraphicsItem *parent)
    : QGraphicsLineItem(startPoint.x(), startPoint.y(), endPoint.x(), endPoint.y(), parent),
    rotationAngle(0)
{
    createHandles();
}

void EditableLineItem::rotate(qreal angle)
{

}

void EditableLineItem::createHandles()
{
    const qreal handleSize = 6.0;
    const qreal handleRadius = handleSize / 2.0;
    startHandle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
    startHandle->setPen(QPen(Qt::black, 0));
    startHandle->setBrush(Qt::red);
    startHandle->setFlag(QGraphicsItem::ItemIsMovable, true);
    startHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    startHandle->setVisible(false);
    startHandle->setCursor(Qt::SizeAllCursor);

    endHandle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
    endHandle->setPen(QPen(Qt::black, 0));
    endHandle->setBrush(Qt::blue);
    endHandle->setFlag(QGraphicsItem::ItemIsMovable, true);
    endHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    endHandle->setVisible(false);
    endHandle->setCursor(Qt::SizeAllCursor);

    updateLine(line().p1(), line().p2());

}

void EditableLineItem::updateHandlesPosition()
{
    QLineF lineData = line();
    startHandle->setPos(lineData.p1());
    endHandle->setPos(lineData.p2());
}
QVariant EditableLineItem::itemChange(GraphicsItemChange change, const QVariant &value)
{

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
}
