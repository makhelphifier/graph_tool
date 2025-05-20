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

    QPainterPath path;
    if (!points.isEmpty()) {
        path.moveTo(points.first());
        for (int i = 1; i < points.size(); ++i) {
            path.lineTo(points[i]);
        }
        if (isClosed_ && points.size() >= 3) { // 如果闭合且点数足够形成多边形
            path.lineTo(points.first()); // 将路径闭合以正确计算边界
        }
    }

    // 为画笔宽度和可能的控制柄大小添加一些边距
    qreal penWidth = linePen.widthF();
    qreal margin = penWidth / 2.0 + 4.0; // 4.0 是一个大致的控制柄边距
    return path.boundingRect().adjusted(-margin, -margin, margin, margin);
}


void EditablePolylineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(linePen); // 设置画笔
    if (points.size() >= 2) {
        for (int i = 0; i < points.size() - 1; ++i) {
            painter->drawLine(points[i], points[i + 1]);
        }
        // 如果折线是闭合的，并且有足够的点来形成一个闭合形状，则绘制连接最后一个点和第一个点的线
        if (isClosed_ && points.size() >= 3) {
            qDebug() << "Painting closing segment for polyline:" << this << "from" << points.last() << "to" << points.first();
            painter->drawLine(points.last(), points.first());
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
    if (isClosed_ == closed) return; // 如果状态未改变，则不执行任何操作

    isClosed_ = closed;
    qDebug() << "Polyline " << this << " set to " << (isClosed_ ? "closed" : "open");

    // 因为 boundingRect 的计算依赖于 isClosed_，所以需要调用 prepareGeometryChange
    prepareGeometryChange();
    update(); // 请求重绘
}
