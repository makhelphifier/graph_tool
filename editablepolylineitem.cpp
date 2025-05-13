#include "editablepolylineitem.h"
#include <QPainter>
#include <QGraphicsScene>
#include <QCursor>
#include <QDebug>

EditablePolylineItem::EditablePolylineItem(const QVector<QPointF>& points, QGraphicsItem *parent)
    : QGraphicsItem(parent), points(points), linePen(Qt::black, 1)
{
    createHandles();
    updateHandlesPosition();
    setFlags(ItemIsSelectable | ItemIsMovable);
    setAcceptHoverEvents(true);
}

EditablePolylineItem::~EditablePolylineItem()
{
    for (HandleItem* handle : handles) {
        if (handle && handle->parentItem() != this) {
            delete handle;
        }
    }
}
void EditablePolylineItem::setPen(const QPen &pen)
{
    linePen = pen;
    update();
}
EditablePolylineItem* EditablePolylineItem::clone() const
{
    EditablePolylineItem* newItem = new EditablePolylineItem(points);
    newItem->setPos(pos());
    newItem->setRotation(rotation());
    newItem->setScale(scale());
    newItem->setTransform(transform());
    newItem->setFlags(flags());
    newItem->setEnabled(isEnabled());
    newItem->setVisible(isVisible());
    newItem->setZValue(zValue());
    newItem->setPen(linePen);
    newItem->updateHandlesPosition();
    qDebug() << "Cloned an EditablePolylineItem.";
    return newItem;
}

void EditablePolylineItem::updatePoint(int index, const QPointF& newPoint)
{
    if (index >= 0 && index < points.size()) {
        points[index] = newPoint;
        updateHandlesPosition();
        prepareGeometryChange();
        update();
        qDebug() << "Updated polyline point at index" << index << "to" << newPoint;
    }
}

void EditablePolylineItem::setSelectedState(bool selected)
{
    for (HandleItem* handle : handles) {
        handle->setVisible(selected);
    }
}

QRectF EditablePolylineItem::boundingRect() const
{
    if (points.isEmpty()) return QRectF();

    qreal minX = points.first().x();
    qreal maxX = points.first().x();
    qreal minY = points.first().y();
    qreal maxY = points.first().y();

    for (const QPointF& p : points) {
        minX = qMin(minX, p.x());
        maxX = qMax(maxX, p.x());
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
    }

    return QRectF(minX, minY, maxX - minX, maxY - minY).adjusted(-2, -2, 2, 2);
}

void EditablePolylineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(linePen);
    if (points.size() >= 2) {
        for (int i = 0; i < points.size() - 1; ++i) {
            painter->drawLine(points[i], points[i + 1]);
        }
    }
}

QVariant EditablePolylineItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        updateHandlesPosition();
    }
    return QGraphicsItem::itemChange(change, value);
}

void EditablePolylineItem::createHandles()
{
    const qreal handleSize = 4.0;
    const qreal handleRadius = handleSize / 2.0;

    handles.clear();
    for (int i = 0; i < points.size(); ++i) {
        HandleItem *handle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
        handle->setPen(QPen(Qt::gray, 0));
        handle->setBrush(Qt::gray);
        handle->setFlag(QGraphicsItem::ItemIsMovable, true);
        handle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        handle->setVisible(false);
        handle->setCursor(Qt::SizeAllCursor);
        handles.append(handle);
    }
}

void EditablePolylineItem::updateHandlesPosition()
{
    if (handles.size() != points.size()) {
        // 如果顶点数量变化，重新创建控制端点
        for (HandleItem* handle : handles) {
            if (handle && handle->parentItem() == this) {
                delete handle;
            }
        }
        handles.clear();
        createHandles();
    }

    for (int i = 0; i < points.size(); ++i) {
        handles[i]->setPos(points[i]);
    }
}


void EditablePolylineItem::setClosed(bool closed)
{
    isClosed_ = closed;
    update();
    qDebug() << "Polyline set to" << (closed ? "closed" : "open");
}

