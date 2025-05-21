#include "editablepolylineitem.h"
#include <QPainter>
#include <QGraphicsScene>
#include <QCursor>
#include <QDebug>

EditablePolylineItem::EditablePolylineItem(const QVector<QPointF>& points, QGraphicsItem *parent)
    : QGraphicsItem(parent), points(points), linePen(Qt::black, 1) // 初始化折线顶点和画笔
{
    createHandles(); // 创建控制端点
    updateHandlesPosition(); // 更新端点位置
    setFlags(ItemIsSelectable | ItemIsMovable); // 设置可选择和可移动
    setAcceptHoverEvents(true); // 接受悬停事件
}

EditablePolylineItem::~EditablePolylineItem()
{
    // 清理端点，确保未绑定到其他对象时删除
    for (HandleItem* handle : handles) {
        if (handle && handle->parentItem() != this) {
            delete handle;
        }
    }
}

void EditablePolylineItem::setPen(const QPen &pen)
{
    linePen = pen; // 设置画笔
    update(); // 请求重绘
}

EditablePolylineItem* EditablePolylineItem::clone() const
{
    // 创建新折线对象，复制顶点
    EditablePolylineItem* newItem = new EditablePolylineItem(points);
    newItem->setPos(pos()); // 复制位置
    newItem->setRotation(rotation()); // 复制旋转
    newItem->setScale(scale()); // 复制缩放
    newItem->setTransform(transform()); // 复制变换
    newItem->setFlags(flags()); // 复制标志位
    newItem->setEnabled(isEnabled()); // 复制启用状态
    newItem->setVisible(isVisible()); // 复制可见性
    newItem->setZValue(zValue()); // 复制Z值
    newItem->setPen(linePen); // 复制画笔
    newItem->updateHandlesPosition(); // 更新端点位置
    qDebug() << "复制了一个 EditablePolylineItem 对象。"; // 打印复制信息
    return newItem;
}

void EditablePolylineItem::updatePoint(int index, const QPointF& newPoint)
{
    if (index >= 0 && index < points.size()) {
        points[index] = newPoint; // 更新指定顶点
        updateHandlesPosition(); // 更新端点位置
        prepareGeometryChange(); // 准备几何变化
        update(); // 请求重绘
        qDebug() << "更新折线顶点索引" << index << "至" << newPoint; // 打印更新信息
    }
}

void EditablePolylineItem::setSelectedState(bool selected)
{
    // 设置所有端点的可见性
    for (HandleItem* handle : handles) {
        handle->setVisible(selected);
    }
}

QRectF EditablePolylineItem::boundingRect() const
{
    if (points.isEmpty()) return QRectF(); // 空顶点返回空矩形

    QPainterPath path;
    if (!points.isEmpty()) {
        path.moveTo(points.first()); // 移动到第一个点
        for (int i = 1; i < points.size(); ++i) {
            path.lineTo(points[i]); // 连接后续点
        }
        if (isClosed_ && points.size() >= 3) { // 如果闭合且点数足够
            path.lineTo(points.first()); // 闭合路径
        }
    }

    // 为画笔宽度和端点大小添加边距
    qreal penWidth = linePen.widthF();
    qreal margin = penWidth / 2.0 + 4.0; // 端点边距
    return path.boundingRect().adjusted(-margin, -margin, margin, margin); // 返回调整后的边界矩形
}

void EditablePolylineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option); // 未使用参数
    Q_UNUSED(widget); // 未使用参数

    painter->setPen(linePen); // 设置画笔
    if (points.size() >= 2) {
        for (int i = 0; i < points.size() - 1; ++i) {
            painter->drawLine(points[i], points[i + 1]); // 绘制线段
        }
        // 如果闭合且点数足够，绘制闭合线段
        if (isClosed_ && points.size() >= 3) {
            qDebug() << "绘制折线闭合线段:" << this << "从" << points.last() << "到" << points.first(); // 打印闭合信息
            painter->drawLine(points.last(), points.first());
        }
    }
}

QVariant EditablePolylineItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        updateHandlesPosition(); // 位置变更时更新端点
    }
    return QGraphicsItem::itemChange(change, value);
}

void EditablePolylineItem::createHandles()
{
    const qreal handleSize = 4.0; // 端点大小
    const qreal handleRadius = handleSize / 2.0; // 端点半径

    handles.clear(); // 清空现有端点
    for (int i = 0; i < points.size(); ++i) {
        HandleItem *handle = new HandleItem(-handleRadius, -handleRadius, handleSize, handleSize, this);
        handle->setPen(QPen(Qt::gray, 0)); // 灰色边框
        handle->setBrush(Qt::gray); // 灰色填充
        handle->setFlag(QGraphicsItem::ItemIsMovable, true); // 可移动
        handle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true); // 忽略变换
        handle->setVisible(false); // 初始不可见
        handle->setCursor(Qt::SizeAllCursor); // 设置光标样式
        handles.append(handle); // 添加端点到列表
    }
}

void EditablePolylineItem::updateHandlesPosition()
{
    if (handles.size() != points.size()) {
        // 如果顶点数量变化，重新创建端点
        for (HandleItem* handle : handles) {
            if (handle && handle->parentItem() == this) {
                delete handle;
            }
        }
        handles.clear();
        createHandles(); // 重新创建端点
    }

    // 更新每个端点位置
    for (int i = 0; i < points.size(); ++i) {
        handles[i]->setPos(points[i]);
    }
}

void EditablePolylineItem::setClosed(bool closed)
{
    if (isClosed_ == closed) return; // 状态未变则不操作

    isClosed_ = closed; // 设置闭合状态
    qDebug() << "折线 " << this << " 设置为 " << (isClosed_ ? "闭合" : "开放"); // 打印状态信息

    // 因边界矩形计算依赖闭合状态，需准备几何变化
    prepareGeometryChange();
    update(); // 请求重绘
}
