#include "graphicstoolview.h"
#include "editablelineitem.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QCursor>  // 包含光标头文件
#include <QGraphicsItem>
#include <QPainterPath>

GraphicsToolView::GraphicsToolView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent),
    isDraggingSelectionGroup(false),
    isDragging(false),
    previewLine(nullptr),
    draggedHandle(nullptr),
    draggedItem(nullptr),
    isShiftPressed(false),
    isCtrlPressedForCopy(false),
    drawingColor(Qt::black),  // 默认绘图颜色为黑色
    isDrawingPolyline(false),
    draggedHandleIndex(-1),
    closePolylineOnFinish(false),
    previewClosingSegment(nullptr)
{
    setRenderHint(QPainter::Antialiasing);
}

// 设置绘图颜色
void GraphicsToolView::setDrawingColor(const QColor &color)
{
    drawingColor = color;
    qDebug() << "绘图颜色设置为：" << color.name();
}

// 获取当前绘图颜色
QColor GraphicsToolView::currentDrawingColor() const
{
    return drawingColor;
}

// 显示颜色选择器
void GraphicsToolView::showColorSelector()
{
    if (colorSelector) {
        QPoint viewCenter = viewport()->rect().center();
        QPoint globalPos = mapToGlobal(viewCenter);
        colorSelector->move(globalPos);
        colorSelector->show();
    }
}

// 处理颜色选择信号
void GraphicsToolView::onColorSelected(const QColor &color)
{
    setDrawingColor(color);
    qDebug() << "选择的颜色：" << color.name();
}

void GraphicsToolView::setDrawingMode(DrawingMode mode)
{
    qDebug() << "设置绘图模式为：" << static_cast<int>(mode);
    if (currentMode == DrawingMode::Polyline && isDrawingPolyline) {
        qDebug() << "切换模式时正在绘制折线，结束当前折线。";
        finishPolyline();
    }
    currentMode = mode;
    cleanupDrawing();
    cleanupSelection();
    switch (mode) {
    case DrawingMode::None:
        setCursor(Qt::ArrowCursor);
        setMouseTracking(false);
        break;
    case DrawingMode::Line:
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
        qDebug() << "进入直线模式";
        break;
    case DrawingMode::Polyline:
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
        qDebug() << "进入折线模式";
        break;
    default:
        setCursor(Qt::ArrowCursor);
        setMouseTracking(false);
        break;
    }
}

void GraphicsToolView::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "鼠标按下事件，按钮：" << event->button();
    bool isCtrlPressed = event->modifiers() & Qt::ControlModifier;
    if (currentMode == DrawingMode::None) {
        qDebug() << "处理无模式按下事件";
        handleNoneModePress(event);
    } else if (currentMode == DrawingMode::Line) {
        qDebug() << "处理直线模式按下事件";
        handleLineModePress(event);
    } else if (currentMode == DrawingMode::Polyline) {
        if (event->button() == Qt::LeftButton) {
            qDebug() << "处理折线模式按下事件（左键）";
            handlePolylineModePress(event);
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void GraphicsToolView::mouseDoubleClickEvent(QMouseEvent *event)
{
    qDebug() << "双击事件，按钮：" << event->button() << "模式：" << static_cast<int>(currentMode) << "是否绘制折线：" << isDrawingPolyline;
    if (event->button() == Qt::LeftButton && currentMode == DrawingMode::Polyline && isDrawingPolyline) {
        qDebug() << "折线双击：结束折线绘制";
        finishPolyline();
        event->accept();
    } else {
        qDebug() << "非折线结束双击，传递给基类";
        QGraphicsView::mouseDoubleClickEvent(event);
    }
}

void GraphicsToolView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    updateCursorBasedOnPosition(scenePos);
    if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
        handleLineModeMove(event);
    } else if (currentMode == DrawingMode::Polyline && isDrawingPolyline && isDragging) {
        handlePolylineModeMove(event);
    } else if (currentMode == DrawingMode::None && draggedHandle && draggedItem) {
        handleHandleMove(event);
    } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup && !selectedItems.isEmpty()) {
        handleGroupMove(event);
    }
}

void GraphicsToolView::wheelEvent(QWheelEvent *event)
{
    const qreal delta = event->angleDelta().y();
    qreal scaleFactor = 1.15;
    if (delta > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void GraphicsToolView::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "按键事件，按键：" << event->key();
    if (event->key() == Qt::Key_Escape) {
        if (currentMode != DrawingMode::None || isDrawingPolyline) {
            qDebug() << "按下Esc，取消绘制";
            cleanupDrawing();
            setDrawingMode(DrawingMode::None);
        } else if (!selectedItems.isEmpty()) {
            qDebug() << "按下Esc，清除选中";
            cleanupSelection();
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Shift) {
        if (currentMode == DrawingMode::Line) {
            isShiftPressed = true;
            qDebug() << "按下Shift，限制直线方向";
        }
    } else  // 处理 'C' 键，用于闭合折线并结束绘制
        if (event->key() == Qt::Key_C) {
            if (currentMode == DrawingMode::Polyline && isDrawingPolyline) {
                qDebug() << "按下C，尝试闭合并完成折线";
                // 确保至少有2个点才能形成一个可以闭合的线段 (形成图形至少需要3个点)
                if (polylinePoints.size() >= 2) {
                    closePolylineOnFinish = true; // 设置闭合标志
                    finishPolyline();             // 调用 finishPolyline 来完成绘制和闭合
                } else {
                    qDebug() << "点数不足 (<2)，无法闭合折线。将作为开放折线结束（如果可能）或丢弃。";
                    closePolylineOnFinish = false; // 确保不会尝试闭合一个无效的多边形
                    finishPolyline(); // 正常结束，finishPolyline 会处理点数不足的情况
                }
                event->accept();
                return;
            }
        }
    if (!event->isAccepted()) {
        QGraphicsView::keyPressEvent(event);
    }
}

void GraphicsToolView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift) {
        isShiftPressed = false;
        qDebug() << "释放Shift，不限制直线方向";
        if (currentMode == DrawingMode::Polyline && previewLine && !polylinePoints.isEmpty()) {
            // 当Shift状态改变时，更新预览线
            updatePolylinePreview(previewLine->line().p2());
        }
    }
    QGraphicsView::keyReleaseEvent(event);
}
void GraphicsToolView::finishPolyline()
{
    qDebug() << "结束折线绘制，是否绘制中：" << isDrawingPolyline << "点数：" << polylinePoints.size();
    if (isDrawingPolyline) {
        if (currentPolyline && polylinePoints.size() >= 2) { // 折线至少需要2个点
            qDebug() << "折线绘制完成，点数：" << polylinePoints.size();
            if (closePolylineOnFinish && polylinePoints.size() >= 3) { // 闭合的多边形至少需要3个点
                currentPolyline->setClosed(true); // 设置 EditablePolylineItem 为闭合
                qDebug() << "折线设置为闭合";
            } else {
                currentPolyline->setClosed(false); // 明确设置为不闭合
                qDebug() << "折线设置为开放 (或点数不足以闭合)";
            }
            // currentPolyline 已经在场景中了，不需要再次添加
            // scene()->addItem(currentPolyline); // 这行是多余的，因为 currentPolyline 在 handlePolylineModePress 中已添加
            currentPolyline = nullptr; // 完成后，重置 currentPolyline 指针，因为实际对象已由场景管理
        } else if (currentPolyline) { // 点数不足 (例如只有1个点后就结束了)
            qDebug() << "折线点数不足，丢弃图形";
            scene()->removeItem(currentPolyline);
            delete currentPolyline;
            currentPolyline = nullptr;
        } else {
            qDebug() << "无折线图形或点数不足";
        }

        // 清理状态
        polylinePoints.clear();
        isDrawingPolyline = false;
        isDragging = false; // isDragging 应该在 mouseReleaseEvent 或类似地方重置

        if (previewLine) {
            scene()->removeItem(previewLine);
            delete previewLine;
            previewLine = nullptr;
        }
        if (previewClosingSegment) { // 清理闭合预览线
            scene()->removeItem(previewClosingSegment);
            delete previewClosingSegment;
            previewClosingSegment = nullptr;
        }
        closePolylineOnFinish = false; // 为下次绘制重置

        qDebug() << "折线绘制结束，重置为无模式";
        setDrawingMode(DrawingMode::None); // 可选：绘制完成后返回无模式
    } else {
        qDebug() << "调用结束折线，但不在绘制模式";
    }
}
void GraphicsToolView::handlePolylineModePress(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    if (isShiftPressed && !polylinePoints.isEmpty()) {
        QPointF lastPoint = polylinePoints.last();
        qreal dx = qAbs(scenePos.x() - lastPoint.x());
        qreal dy = qAbs(scenePos.y() - lastPoint.y());
        if (dx > dy) {
            scenePos.setY(lastPoint.y());
        } else {
            scenePos.setX(lastPoint.x());
        }
    }
    if (!isDrawingPolyline) {
        polylinePoints.clear();
        if (currentPolyline) {
            scene()->removeItem(currentPolyline);
            delete currentPolyline;
            currentPolyline = nullptr;
        }
        if (previewLine) {
            scene()->removeItem(previewLine);
            delete previewLine;
            previewLine = nullptr;
        }
        polylinePoints.append(scenePos);
        isDrawingPolyline = true;
        isDragging = true;
        qDebug() << "开始新折线，位置：" << scenePos;
    } else {
        if (polylinePoints.isEmpty() || polylinePoints.last() != scenePos) {
            polylinePoints.append(scenePos);
            qDebug() << "添加折线点，位置：" << scenePos;
        }
        if (polylinePoints.size() >= 2) {
            if (currentPolyline) {
                scene()->removeItem(currentPolyline);
                delete currentPolyline;
            }
            currentPolyline = new EditablePolylineItem(polylinePoints);
            QPen pen = currentPolyline->pen();
            pen.setColor(drawingColor);
            currentPolyline->setPen(pen);
            scene()->addItem(currentPolyline);
            qDebug() << "更新折线，点数：" << polylinePoints.size();
        }
    }
    if (isDrawingPolyline && !polylinePoints.isEmpty()) {
        if (previewLine) {
            scene()->removeItem(previewLine);
            delete previewLine;
        }
        previewLine = new EditableLineItem(polylinePoints.last(), scenePos);
        QPen pen = previewLine->pen();
        pen.setColor(drawingColor);
        pen.setStyle(Qt::DashLine);
        previewLine->setPen(pen);
        scene()->addItem(previewLine);
    }
}

void GraphicsToolView::handlePolylineModeMove(QMouseEvent *event)
{
    if (!isDrawingPolyline || polylinePoints.isEmpty() || !isDragging) {
        return;
    }
    QPointF scenePos = mapToScene(event->pos());
    updatePolylinePreview(scenePos); // 调用新的辅助函数
}



// 新增辅助函数
void GraphicsToolView::updatePolylinePreview(const QPointF& currentMousePos)
{
    if (!isDrawingPolyline || polylinePoints.isEmpty()) {
        return;
    }

    QPointF lastPoint = polylinePoints.last();
    QPointF effectiveMousePos = currentMousePos;

    if (isShiftPressed) {
        qreal dx = qAbs(effectiveMousePos.x() - lastPoint.x());
        qreal dy = qAbs(effectiveMousePos.y() - lastPoint.y());
        if (dx > dy) {
            effectiveMousePos.setY(lastPoint.y());
        } else {
            effectiveMousePos.setX(lastPoint.x());
        }
    }

    // 更新主预览线 (从最后一个点到当前鼠标)
    if (previewLine) {
        previewLine->setLine(lastPoint.x(), lastPoint.y(), effectiveMousePos.x(), effectiveMousePos.y());
    } else {
        previewLine = new EditableLineItem(lastPoint, effectiveMousePos);
        QPen pen = previewLine->pen();
        pen.setColor(drawingColor);
        pen.setStyle(Qt::DashLine);
        previewLine->setPen(pen);
        scene()->addItem(previewLine);
    }
    previewLine->setVisible(true);


    // 更新闭合预览线段
    if (closePolylineOnFinish && polylinePoints.size() >= 1) { // 至少需要1个已确定的点来形成闭合预览
        if (!previewClosingSegment) {
            previewClosingSegment = new EditableLineItem(effectiveMousePos, polylinePoints.first());
            QPen pen = previewClosingSegment->pen();
            pen.setColor(drawingColor); // 可以用不同颜色或样式区分
            pen.setStyle(Qt::DotLine);   // 例如用点线表示闭合预览
            previewClosingSegment->setPen(pen);
            scene()->addItem(previewClosingSegment);
        }
        previewClosingSegment->setLine(effectiveMousePos.x(), effectiveMousePos.y(), polylinePoints.first().x(), polylinePoints.first().y());
        previewClosingSegment->setVisible(true);
    } else {
        if (previewClosingSegment) {
            previewClosingSegment->setVisible(false);
        }
    }
}


void GraphicsToolView::handleNoneModePress(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    QGraphicsItem *item = scene()->itemAt(scenePos, transform());
    qreal tolerance = 10.0;  // 容差，扩展选中区域
    if (checkHandleHit(scenePos)) {
        return;
    }
    if (checkSelectedGroupHit(scenePos, tolerance)) {
        this->isCtrlPressedForCopy = event->modifiers() & Qt::ControlModifier;
        qDebug() << "复制时Ctrl键状态：" << isCtrlPressedForCopy;
        return;
    }
    handleItemSelection(scenePos, tolerance, event->modifiers() & Qt::ControlModifier);
}

bool GraphicsToolView::checkHandleHit(const QPointF &scenePos)
{
    for (QGraphicsItem* item : selectedItems) {
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            HandleItem* startHandle = editableLine->getStartHandle();
            HandleItem* endHandle = editableLine->getEndHandle();
            HandleItem* rotationHandle = editableLine->getRotationHandle();
            if (startHandle && startHandle->isVisible() && startHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = startHandle;
                draggedItem = item;
                qDebug() << "选中起点端点，位置：" << scenePos;
                return true;
            } else if (endHandle && endHandle->isVisible() && endHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = endHandle;
                draggedItem = item;
                qDebug() << "选中终点端点，位置：" << scenePos;
                return true;
            } else if (rotationHandle && rotationHandle->isVisible() && rotationHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = rotationHandle;
                draggedItem = item;
                fixedRotationCenter = editableLine->mapToScene(editableLine->getRotationHandle()->pos());
                qDebug() << "旋转中心固定，位置：" << fixedRotationCenter;
                qDebug() << "选中旋转端点，位置：" << scenePos;
                return true;
            } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                QVector<HandleItem*> handles = editablePolyline->getHandles();
                for (int i = 0; i < handles.size(); ++i) {
                    HandleItem* handle = handles[i];
                    if (handle && handle->isVisible() && handle->sceneBoundingRect().contains(scenePos)) {
                        draggedHandle = handle;
                        draggedItem = item;
                        draggedHandleIndex = i;
                        qDebug() << "选中折线端点，索引：" << i << "位置：" << scenePos;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool GraphicsToolView::checkSelectedGroupHit(const QPointF &scenePos, qreal tolerance)
{
    for (QGraphicsItem* item : selectedItems) {
        QRectF boundingRect = item->boundingRect();
        boundingRect.adjust(-tolerance, -tolerance, tolerance, tolerance);
        QRectF sceneRect = item->mapToScene(boundingRect).boundingRect();
        if (sceneRect.contains(scenePos)) {
            isDraggingSelectionGroup = true;
            dragStartPosition = scenePos;
            lastDragPos = scenePos;
            qDebug() << "开始拖动选中组，位置：" << scenePos;
            return true;
        }
    }
    return false;
}

void GraphicsToolView::handleItemSelection(const QPointF &scenePos, qreal tolerance, bool isMultiSelect)
{
    QList<QGraphicsItem*> items = scene()->items();
    bool itemSelected = false;
    for (QGraphicsItem* item : items) {
        QRectF boundingRect = item->boundingRect();
        boundingRect.adjust(-tolerance, -tolerance, tolerance, tolerance);
        QRectF sceneRect = item->mapToScene(boundingRect).boundingRect();
        if (sceneRect.contains(scenePos)) {
            if (isMultiSelect) {
                if (!selectedItems.contains(item)) {
                    selectedItems.append(item);
                    if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                        editableLine->setSelectedState(true);
                    } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                        editablePolyline->setSelectedState(true);
                    }
                    qDebug() << "添加图形到选中（多选），位置：" << scenePos;
                    qDebug() << "选中图形总数：" << selectedItems.size();
                }
            } else {
                cleanupSelection();
                selectedItems.append(item);
                if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                    editableLine->setSelectedState(true);
                } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                    editablePolyline->setSelectedState(true);
                }
                qDebug() << "选中图形（单选），位置：" << scenePos;
            }
            itemSelected = true;
            break;
        }
    }
    if (!itemSelected) {
        cleanupSelection();
        qDebug() << "点击空白区域，清除选中";
    } else {
        qDebug() << "选中图形总数：" << selectedItems.size();
    }
}

void GraphicsToolView::handleLineModePress(QMouseEvent *event)
{
    selectedItems.clear();
    if (event->button() != Qt::LeftButton) return;
    QPointF scenePos = mapToScene(event->pos());
    if (isShiftPressed && !startPoint.isNull()) {
        qreal dx = qAbs(scenePos.x() - startPoint.x());
        qreal dy = qAbs(scenePos.y() - startPoint.y());
        if (dx > dy) scenePos.setY(startPoint.y());
        else scenePos.setX(startPoint.x());
    }
    if (startPoint.isNull()) {
        startPoint = scenePos;
        isDragging = true;
        qDebug() << "直线模式：起点设置为：" << startPoint;
        if (previewLine) {
            scene()->removeItem(previewLine);
            delete previewLine;
            previewLine = nullptr;
        }
        previewLine = new EditableLineItem(startPoint, startPoint);
        QPen pen = previewLine->pen();
        pen.setColor(drawingColor);
        pen.setStyle(Qt::SolidLine);
        previewLine->setPen(pen);
        previewLine->setSelected(false); // 取消选中状态
        previewLine->setFlag(QGraphicsItem::ItemIsSelectable, false);
        scene()->addItem(previewLine);
    } else {
        endPoint = scenePos;
        qDebug() << "直线模式：终点设置为：" << endPoint;
        EditableLineItem *line = new EditableLineItem(startPoint, endPoint);
        QPen pen = line->pen();
        pen.setColor(drawingColor);
        line->setPen(pen);
        line->setSelected(false);
        scene()->addItem(line);
        qDebug() << "创建直线，从" << startPoint << "到" << endPoint;
        cleanupDrawing();
        setDrawingMode(DrawingMode::None); selectedItems.clear();
    }
}

void GraphicsToolView::updateCursorBasedOnPosition(const QPointF &scenePos)
{
    bool isOverSelectedItem = false;
    qreal tolerance = 10.0;  // 容差，扩展选中区域
    for (QGraphicsItem* item : selectedItems) {
        QRectF boundingRect = item->boundingRect();
        boundingRect.adjust(-tolerance, -tolerance, tolerance, tolerance);
        QRectF sceneRect = item->mapToScene(boundingRect).boundingRect();
        if (sceneRect.contains(scenePos)) {
            isOverSelectedItem = true;
            break;
        }
    }
    if (isOverSelectedItem && currentMode == DrawingMode::None) {
        setCursor(Qt::SizeAllCursor);
    } else if (currentMode == DrawingMode::None) {
        setCursor(Qt::ArrowCursor);
    }
}

void GraphicsToolView::handleLineModeMove(QMouseEvent *event)
{
    if (startPoint.isNull() || !isDragging || !previewLine) return;
    QPointF scenePos = mapToScene(event->pos());
    if (isShiftPressed) {
        qreal dx = qAbs(scenePos.x() - startPoint.x());
        qreal dy = qAbs(scenePos.y() - startPoint.y());
        if (dx > dy) scenePos.setY(startPoint.y());
        else scenePos.setX(startPoint.x());
    }
    previewLine->setLine(QLineF(startPoint, scenePos));
}

void GraphicsToolView::applyColorToSelectedItems(const QColor &color)
{
    for (QGraphicsItem* item : selectedItems) {
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            QPen pen = editableLine->pen();
            pen.setColor(color);
            editableLine->setPen(pen);
        } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
            QPen pen = editablePolyline->pen();
            pen.setColor(color);
            editablePolyline->setPen(pen);
        }
    }
}

void GraphicsToolView::handleHandleMove(QMouseEvent *event)
{
    if (!draggedItem || !draggedHandle) {
        qWarning() << "无效的拖动图形或端点";
        return;
    }
    QPointF newPos = mapToScene(event->pos());
    qDebug() << "鼠标事件位置（视图坐标）：" << event->pos();
    qDebug() << "映射到场景位置：" << newPos;
    if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(draggedItem)) {
        QLineF currentLine = editableLine->line();
        QPointF sceneP1 = editableLine->mapToScene(currentLine.p1());
        QPointF sceneP2 = editableLine->mapToScene(currentLine.p2());
        qDebug() << "当前直线场景坐标：P1=" << sceneP1 << ", P2=" << sceneP2;
        if (draggedHandle == editableLine->getStartHandle()) {
            editableLine->updateLine(newPos, sceneP2);
            qDebug() << "拖动起点端点到：" << newPos;
        } else if (draggedHandle == editableLine->getEndHandle()) {
            editableLine->updateLine(sceneP1, newPos);
            qDebug() << "拖动终点端点到：" << newPos;
        } else if (draggedHandle == editableLine->getRotationHandle()) {
            QPointF midPoint = (sceneP1 + sceneP2) / 2.0;
            QPointF currentVector = midPoint - fixedRotationCenter;
            qreal currentAngle = qAtan2(currentVector.y(), currentVector.x()) * 180.0 / M_PI;
            if (currentAngle < 0) currentAngle += 360.0;
            QPointF mouseVector = newPos - fixedRotationCenter;
            qreal targetAngle = qAtan2(mouseVector.y(), mouseVector.x()) * 180.0 / M_PI;
            if (targetAngle < 0) targetAngle += 360.0;
            qreal angleDiff = targetAngle - currentAngle;
            if (angleDiff > 180) angleDiff -= 360;
            else if (angleDiff < -180) angleDiff += 360;
            qreal newRotation = targetAngle;
            editableLine->rotate(newRotation, fixedRotationCenter);
            qDebug() << "拖动旋转端点到：" << newPos << "，旋转中心：" << fixedRotationCenter;
            qDebug() << "当前角度：" << currentAngle << "，目标角度：" << targetAngle << "，角度差：" << angleDiff << "，新旋转角度：" << newRotation;
        }
    }
}

void GraphicsToolView::handleGroupMove(QMouseEvent *event)
{
    QPointF currentPos = mapToScene(event->pos());
    QPointF offset = currentPos - lastDragPos;
    for (QGraphicsItem* item : selectedItems) {
        QPointF newItemPos = item->pos() + offset;
        item->setPos(newItemPos);
    }
    lastDragPos = currentPos;
    qDebug() << "拖动选中组，偏移量：" << offset;
}

void GraphicsToolView::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "鼠标释放事件";
    if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
    } else if (currentMode == DrawingMode::None && draggedHandle) {
        handleHandleRelease();
    } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup) {
        handleGroupRelease();
    }
    if (!event->isAccepted()) {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void GraphicsToolView::handleLineModeRelease(QMouseEvent *event)
{
    if (isShiftPressed) {
        qreal dx = qAbs(endPoint.x() - startPoint.x());
        qreal dy = qAbs(endPoint.y() - startPoint.y());
        if (dx > dy) {
            endPoint.setY(startPoint.y());
        } else {
            endPoint.setX(startPoint.x());
        }
    }
}

void GraphicsToolView::handleHandleRelease()
{
    qDebug() << "端点拖动释放";
    draggedHandle = nullptr;
    draggedItem = nullptr;
}

void GraphicsToolView::handleGroupRelease()
{
    qDebug() << "选中组拖动释放";
    qDebug() << "复制时Ctrl键状态：" << isCtrlPressedForCopy;
    if (isCtrlPressedForCopy && !selectedItems.isEmpty()) {
        qDebug() << "拖动时按下Ctrl，复制选中图形";
        QList<QGraphicsItem*> newItems;
        for (QGraphicsItem* item : selectedItems) {
            if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                QGraphicsItem* newItem = editableLine->clone();
                if (newItem) {
                    newItem->setPos(item->pos() + QPointF(20, 20));
                    scene()->addItem(newItem);
                    newItems.append(newItem);
                    qDebug() << "复制图形，位置：" << newItem->pos();
                }
            }
        }
        cleanupSelection();
        for (QGraphicsItem* newItem : newItems) {
            if (EditableLineItem* newEditableLine = dynamic_cast<EditableLineItem*>(newItem)) {
                newEditableLine->setSelectedState(true);
                selectedItems.append(newItem);
            }
        }
        qDebug() << "拖动时复制完成，新选中图形数：" << selectedItems.size();
    } else {
        qDebug() << "未执行复制，Ctrl状态：" << isCtrlPressedForCopy << "，选中图形数：" << selectedItems.size();
    }
    isDraggingSelectionGroup = false;
    isCtrlPressedForCopy = false;
}
void GraphicsToolView::cleanupDrawing()
{
    qDebug() << "清理绘制状态，当前模式：" << static_cast<int>(currentMode);
    if (previewLine) { //
        scene()->removeItem(previewLine);
        delete previewLine;
        previewLine = nullptr;
    }
    if (previewClosingSegment) { // 新增清理
        scene()->removeItem(previewClosingSegment);
        delete previewClosingSegment;
        previewClosingSegment = nullptr;
    }
    startPoint = QPointF(); //
    endPoint = QPointF();   //

    if (isDrawingPolyline || !polylinePoints.isEmpty() || currentPolyline) {
        qDebug() << "清理折线绘制残留";
        polylinePoints.clear();
        if (currentPolyline) {
            scene()->removeItem(currentPolyline);
            delete currentPolyline;
            currentPolyline = nullptr;
        }
        isDrawingPolyline = false;
    }
    closePolylineOnFinish = false; // 重置闭合状态
    isDragging = false;
}

void GraphicsToolView::cleanupSelection()
{
    qDebug() << "清理选中状态";
    for (QGraphicsItem* item : selectedItems) {
        if (item) {
            if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                editableLine->setSelectedState(false);
            }
        }
    }
    selectedItems.clear();
    copiedItems.clear();
    draggedHandle = nullptr;
    draggedItem = nullptr;
    isDraggingSelectionGroup = false;
}

void GraphicsToolView::copySelectedItems()
{
    qDebug() << "复制选中图形";
    copiedItems.clear();
    for (QGraphicsItem* item : selectedItems) {
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            if (item->parentItem() == nullptr || dynamic_cast<EditableLineItem*>(item->parentItem()) == nullptr) {
                copiedItems.append(item);
            }
        }
    }
    qDebug() << "复制图形总数：" << copiedItems.size();
}

void GraphicsToolView::pasteCopiedItems()
{
    qDebug() << "粘贴复制的图形";
    if (copiedItems.isEmpty()) {
        qDebug() << "无图形可粘贴";
        return;
    }
    QPointF pasteOffset(20, 20);
    QGraphicsScene* currentScene = scene();
    if (!currentScene) {
        qDebug() << "错误：视图无关联场景";
        return;
    }
    QList<QGraphicsItem*> newlyPastedItems;
    for (QGraphicsItem* item : copiedItems) {
        QGraphicsItem* newItem = nullptr;
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            newItem = editableLine->clone();
        } else {
            if (QGraphicsRectItem* rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
                newItem = new QGraphicsRectItem(rectItem->rect());
                static_cast<QGraphicsRectItem*>(newItem)->setPen(rectItem->pen());
                static_cast<QGraphicsRectItem*>(newItem)->setBrush(rectItem->brush());
            } else {
                qDebug() << "警告：无法粘贴不支持的图形类型";
            }
        }
        if (newItem) {
            newItem->setPos(item->pos() + pasteOffset);
            currentScene->addItem(newItem);
            newlyPastedItems.append(newItem);
        }
    }
    cleanupSelection();
    for (QGraphicsItem* pastedItem : newlyPastedItems) {
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(pastedItem)) {
            editableLine->setSelectedState(true);
            selectedItems.append(editableLine);
            copiedItems.append(editableLine);
        }
    }
    qDebug() << "粘贴图形总数：" << selectedItems.size();
}
