#include "graphicstoolview.h"

#include "editablelineitem.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QCursor>  // 包含这个头文件

#include <QGraphicsItem>
#include <QPainterPath>
GraphicsToolView::GraphicsToolView(QGraphicsScene *scene, QWidget *parent)
    :QGraphicsView(scene, parent),
    isDraggingSelectionGroup(false),
    isDragging(false),
    previewLine(nullptr),
    draggedHandle(nullptr),
    draggedItem(nullptr),
    isShiftPressed(false),
    isCtrlPressedForCopy(false),
    drawingColor(Qt::black) ,// 默认绘图颜色为黑色,
    isDrawingPolyline(false),
    draggedHandleIndex(-1),
    closePolylineOnFinish(false)

{
    setRenderHint(QPainter::Antialiasing);

}

// 设置当前绘图颜色
void GraphicsToolView::setDrawingColor(const QColor &color)
{
    drawingColor = color;
    qDebug() << "Drawing color set to:" << color.name();
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
        // 将颜色选择器显示在视图的中心位置或鼠标位置附近
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
    qDebug() << "Color selected from popup:" << color.name();
}


void GraphicsToolView::setDrawingMode(DrawingMode mode)
{
    currentMode = mode;
    if(mode == DrawingMode::None){
        setCursor(Qt::ArrowCursor);
        setMouseTracking(false);
    }else{
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
    }
    // 切换模式时清理绘图和选中状态
    cleanupDrawing();
    cleanupSelection();


    switch (mode) {
    case DrawingMode::Line:
        qDebug()<<"Enter Line Mode";
        break;
    case DrawingMode::Polyline:
        qDebug() << "Enter Polyline Mode";
        break;
    default:
        break;
    }
}

void GraphicsToolView::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "mousePressEvent of GraphicsToolView";

    // 记录是否按住 Ctrl 键
    bool isCtrlPressed = event->modifiers() & Qt::ControlModifier;


    if (currentMode == DrawingMode::None) {
        handleNoneModePress(event);
    } else if (currentMode == DrawingMode::Line) {
        handleLineModePress(event);
    } else if (currentMode == DrawingMode::Polyline) {
        // 右键结束折线绘制
        if (event->button() == Qt::RightButton) {
            finishPolyline();
        } else {
            handlePolylineModePress(event);
        }
    }

    QGraphicsView::mousePressEvent(event);
}


void GraphicsToolView::mouseMoveEvent(QMouseEvent *event)
{

    QPointF scenePos = mapToScene(event->pos());

    // 更新光标样式
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
    // Handle mouse wheel events for zooming.
    // 获取滚轮的垂直滚动量。正值表示向上滚动（通常是放大），负值表示向下滚动（通常是缩小）。
    const qreal delta = event->angleDelta().y();

    // 定义缩放因子。可以根据需要调整这个值来控制缩放速度。
    qreal scaleFactor = 1.15; // 每次滚轮事件缩放 15%

    // 如果滚轮向上滚动，则放大；如果向下滚动，则缩小。
    if (delta > 0) {
        // Zoom in
        scale(scaleFactor, scaleFactor);
    } else {
        // Zoom out
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}


void GraphicsToolView::keyPressEvent(QKeyEvent *event)
{
    // Handle Escape key for cancelling drawing or clearing selection
    if (event->key() == Qt::Key_Escape) {
        if (currentMode != DrawingMode::None) {
            qDebug() << "Esc pressed. Cancelling drawing.";
            cleanupDrawing();
            setDrawingMode(DrawingMode::None);
        } else {
            qDebug() << "Esc pressed. Clearing selection.";
            cleanupSelection();
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Shift) {
        if (currentMode == DrawingMode::Line) {
            isShiftPressed = true;
            qDebug() << "Shift pressed. Restricting line to horizontal or vertical.";
        }
    }else if (event->key() == Qt::Key_C) {
        // C 键切换闭合折线状态
        if (currentMode == DrawingMode::Polyline && isDrawingPolyline) {
            closePolylineOnFinish = !closePolylineOnFinish;
            qDebug() << "C pressed. Polyline will" << (closePolylineOnFinish ? "close" : "not close") << "on finish.";
            event->accept();
            return;
        }
    }


    QGraphicsView::keyPressEvent(event);
}

void GraphicsToolView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift) {
        isShiftPressed = false;
        qDebug() << "Shift released. No restriction on line direction.";
    }
    QGraphicsView::keyReleaseEvent(event);
}
void GraphicsToolView::finishPolyline()
{
    if (isDrawingPolyline) {
        if (polylinePoints.size() < 2) {
            qDebug() << "Polyline has less than 2 points, discarding.";
            if (currentPolyline) {
                scene()->removeItem(currentPolyline);
                delete currentPolyline;
                currentPolyline = nullptr;
            }
        } else {
            qDebug() << "Polyline drawing finished with" << polylinePoints.size() << "points.";
            // 保留 currentPolyline 作为最终对象
        }
        // 清理状态
        polylinePoints.clear();
        isDrawingPolyline = false;
        isDragging = false;
        if (previewLine) {
            scene()->removeItem(previewLine);
            delete previewLine;
            previewLine = nullptr;
        }
        qDebug() << "Polyline drawing completed.";
        setDrawingMode(DrawingMode::None); // 可选：绘制完成后返回无模式
    }
}



void GraphicsToolView::handlePolylineModePress(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());

    // 如果按住 Shift 键，限制方向
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
        // 开始新的折线绘制
        polylinePoints.clear();
        qDebug() << "Starting new polyline at:" << scenePos;
        polylinePoints.append(scenePos);
        isDrawingPolyline = true;
        isDragging = true;
    }else {
        // 添加新的顶点
        qDebug() << "Adding polyline point at:" << scenePos;
        polylinePoints.append(scenePos);
        // 更新或创建折线对象
        if (currentPolyline) {
            scene()->removeItem(currentPolyline);
            delete currentPolyline;
        }
        currentPolyline = new EditablePolylineItem(polylinePoints);
        QPen pen = currentPolyline->pen();
        pen.setColor(drawingColor);
        currentPolyline->setPen(pen);
        scene()->addItem(currentPolyline);
        qDebug() << "Updated polyline with" << polylinePoints.size() << "points.";
    }
}


void GraphicsToolView::handlePolylineModeMove(QMouseEvent *event)
{
    if (!isDrawingPolyline || polylinePoints.isEmpty()) {
        return;
    }

    QPointF scenePos = mapToScene(event->pos());

    // 如果按住 Shift 键，限制方向
    if (isShiftPressed) {
        QPointF lastPoint = polylinePoints.last();
        qreal dx = qAbs(scenePos.x() - lastPoint.x());
        qreal dy = qAbs(scenePos.y() - lastPoint.y());
        if (dx > dy) {
            scenePos.setY(lastPoint.y());
        } else {
            scenePos.setX(lastPoint.x());
        }
    }

    // 更新预览线段（从最后一个顶点到鼠标当前位置）
    if (previewLine) {
        previewLine->setLine(polylinePoints.last().x(), polylinePoints.last().y(), scenePos.x(), scenePos.y());
    } else {
        previewLine = new EditableLineItem(polylinePoints.last(), scenePos);
        QPen pen = previewLine->pen();
        pen.setColor(drawingColor);
        previewLine->setPen(pen);
        scene()->addItem(previewLine);
    }
    qDebug() << "Updating polyline preview to:" << scenePos;
}

void GraphicsToolView::handleNoneModePress(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    QGraphicsItem *item = scene()->itemAt(scenePos, transform());
    qreal tolerance = 10.0; // 容差，扩展矩形区域，便于选中

    // 先检查是否点击了端点
    if (checkHandleHit(scenePos)) {
        return; // 如果点击了端点，直接返回
    }

    // 检查是否点击了已选中的图形（以便拖动整个选中组）
    if (checkSelectedGroupHit(scenePos, tolerance)) {
        // 在确认开始拖动选中组后，立即设置 Ctrl 键状态
        this->isCtrlPressedForCopy = event->modifiers() & Qt::ControlModifier;
        qDebug() << "Ctrl key state for copy during drag: " << isCtrlPressedForCopy;
        return; // 如果点击了选中组，直接返回
    }

    // 检查是否点击了线条并处理选中逻辑
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
                qDebug() << "Start handle selected for dragging at position:" << scenePos;
                return true;
            } else if (endHandle && endHandle->isVisible() && endHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = endHandle;
                draggedItem = item;
                qDebug() << "End handle selected for dragging at position:" << scenePos;
                return true;
            }else if (rotationHandle && rotationHandle->isVisible() && rotationHandle->sceneBoundingRect().contains(scenePos)) {
                // 检查是否点击了旋转端点
                draggedHandle = rotationHandle;
                draggedItem = item;
                fixedRotationCenter = editableLine->mapToScene(editableLine->getRotationHandle()->pos());
                qDebug() << "Fixed rotation center at drag start:" << fixedRotationCenter;
                qDebug() << "Rotation handle selected for dragging at position:" << scenePos;
                return true;
            }else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                QVector<HandleItem*> handles = editablePolyline->getHandles();
                for (int i = 0; i < handles.size(); ++i) {
                    HandleItem* handle = handles[i];
                    if (handle && handle->isVisible() && handle->sceneBoundingRect().contains(scenePos)) {
                        draggedHandle = handle;
                        draggedItem = item;
                        draggedHandleIndex = i; // 记录拖动的端点索引
                        qDebug() << "Polyline handle selected for dragging at index" << i << "position:" << scenePos;
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
            qDebug() << "Dragging selected group starting at position:" << scenePos;
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
                    qDebug() << "Item added to selection (Multi-Select) at position:" << scenePos;
                    qDebug() << "Total selected items:" << selectedItems.size();
                }
            } else {
                cleanupSelection();
                selectedItems.append(item);
                if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                    editableLine->setSelectedState(true);
                } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                    editablePolyline->setSelectedState(true);
                }
                qDebug() << "Item selected (Single-Select) at position:" << scenePos;
            }
            itemSelected = true;
            break;
        }
    }

    if (!itemSelected) {
        cleanupSelection();
        qDebug() << "Clicked on empty space. Selection cleared.";
    } else {
        qDebug() << "Total selected items:" << selectedItems.size();
    }
}

void GraphicsToolView::handleLineModePress(QMouseEvent *event)
{
    if (startPoint.isNull()) {
        startPoint = mapToScene(event->pos());
        qDebug() << "Start Point set to:" << startPoint;
        if (previewLine) {
            scene()->removeItem(previewLine);
            delete previewLine;
            previewLine = nullptr;
        }
        isDragging = true;
    } else {
        endPoint = mapToScene(event->pos());
        // 如果按住 Shift 键，限制方向
        if (isShiftPressed) {
            qreal dx = qAbs(endPoint.x() - startPoint.x());
            qreal dy = qAbs(endPoint.y() - startPoint.y());
            if (dx > dy) {
                endPoint.setY(startPoint.y());
            } else {
                endPoint.setX(startPoint.x());
            }
        }
        qDebug() << "End Point set to:" << endPoint;
        qDebug() << "Both points are set - Start Point:" << startPoint << ", End Point:" << endPoint;
        QGraphicsLineItem *line = new EditableLineItem(startPoint, endPoint);
        // 应用当前绘图颜色
        QPen pen = line->pen();
        pen.setColor(drawingColor);
        line->setPen(pen);
        scene()->addItem(line);
        qDebug() << "Line created from" << startPoint << "to" << endPoint;
        qDebug() << "Drawing completed. Resetting points.";
        cleanupDrawing();
    }
}


void GraphicsToolView::updateCursorBasedOnPosition(const QPointF &scenePos)
{
    bool isOverSelectedItem = false;
    qreal tolerance = 10.0; // 容差，扩展矩形区域，便于选中

    // 检查鼠标是否在选中项的矩形区域内
    for (QGraphicsItem* item : selectedItems) {
        QRectF boundingRect = item->boundingRect();
        boundingRect.adjust(-tolerance, -tolerance, tolerance, tolerance);
        QRectF sceneRect = item->mapToScene(boundingRect).boundingRect();
        if (sceneRect.contains(scenePos)) {
            isOverSelectedItem = true;
            break;
        }
    }

    // 根据是否在选中项上以及当前模式设置光标样式
    if (isOverSelectedItem && currentMode == DrawingMode::None) {
        setCursor(Qt::SizeAllCursor); // 横竖四个箭头光标
    } else if (currentMode == DrawingMode::None) {
        setCursor(Qt::ArrowCursor); // 不在选中项区域内，恢复默认光标
    }
    // 其他模式下的光标样式（如 DrawingMode::Line）保持不变
    // 如果需要，可以在这里添加其他模式的逻辑
}


void GraphicsToolView::handleLineModeMove(QMouseEvent *event)
{
    endPoint = mapToScene(event->pos());
    // 如果按住 Shift 键，限制直线方向为水平或垂直
    if (isShiftPressed) {
        QPointF currentPoint = mapToScene(event->pos());
        qreal dx = qAbs(currentPoint.x() - startPoint.x());
        qreal dy = qAbs(currentPoint.y() - startPoint.y());
        if (dx > dy) {
            // 水平方向移动更大，限制为水平线
            endPoint.setY(startPoint.y());
            qDebug() << "Restricting to horizontal line. End point:" << endPoint;
        } else {
            // 垂直方向移动更大，限制为垂直线
            endPoint.setX(startPoint.x());
            qDebug() << "Restricting to vertical line. End point:" << endPoint;
        }
    }
    qDebug() << "Both points are set - Start Point:" << startPoint << ", End Point:" << endPoint;
    if (previewLine) {
        previewLine->setLine(startPoint.x(), startPoint.y(), endPoint.x(), endPoint.y());
    } else {
        previewLine = new EditableLineItem(startPoint, endPoint);
        // 应用当前绘图颜色到预览线段
        QPen pen = previewLine->pen();
        pen.setColor(drawingColor);
        previewLine->setPen(pen);
        scene()->addItem(previewLine);
    }
    qDebug() << "End Point set to:" << endPoint;
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
        qWarning() << "Invalid dragged item or handle";
        return;
    }

    QPointF newPos = mapToScene(event->pos());
    // 调试输出，确认鼠标位置
    qDebug() << "Mouse event pos (view coords):" << event->pos();
    qDebug() << "Mapped to scene pos:" << newPos;

    if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(draggedItem)) {
        QLineF currentLine = editableLine->line();
        QPointF sceneP1 = editableLine->mapToScene(currentLine.p1());
        QPointF sceneP2 = editableLine->mapToScene(currentLine.p2());
        qDebug() << "Current line in scene coords: P1=" << sceneP1 << ", P2=" << sceneP2;

        if (draggedHandle == editableLine->getStartHandle()) {
            editableLine->updateLine(newPos, sceneP2);
            qDebug() << "Dragging start handle to:" << newPos;
        } else if (draggedHandle == editableLine->getEndHandle()) {
            editableLine->updateLine(sceneP1, newPos);
            qDebug() << "Dragging end handle to:" << newPos;
        } else if (draggedHandle == editableLine->getRotationHandle()) {
            // 使用固定的旋转中心（假设 fixedRotationCenter 已定义并初始化）
            QPointF midPoint = (sceneP1 + sceneP2) / 2.0;

            // 计算当前线段中点到旋转中心的向量角度
            QPointF currentVector = midPoint - fixedRotationCenter;
            qreal currentAngle = qAtan2(currentVector.y(), currentVector.x()) * 180.0 / M_PI;
            if (currentAngle < 0) currentAngle += 360.0;

            // 计算鼠标位置到旋转中心的向量角度
            QPointF mouseVector = newPos - fixedRotationCenter;
            qreal targetAngle = qAtan2(mouseVector.y(), mouseVector.x()) * 180.0 / M_PI;
            if (targetAngle < 0) targetAngle += 360.0;

            // 计算角度差，处理跨界问题
            qreal angleDiff = targetAngle - currentAngle;
            if (angleDiff > 180) angleDiff -= 360;
            else if (angleDiff < -180) angleDiff += 360;

            // 计算新的旋转角度（直接使用目标角度，而不是累加）
            qreal newRotation = targetAngle;
            editableLine->rotate(newRotation, fixedRotationCenter);

            qDebug() << "Dragging rotation handle to:" << newPos << ", rotation center:" << fixedRotationCenter;
            qDebug() << "Current angle:" << currentAngle << ", Target angle:" << targetAngle
                     << ", Angle diff:" << angleDiff << ", New rotation:" << newRotation;
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
    qDebug() << "Dragging selected group with offset:" << offset;
}

void GraphicsToolView::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "mouseReleaseEvent of GraphicsToolView";
    if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
        handleLineModeRelease(event);
    } else if (currentMode == DrawingMode::None && draggedHandle) {
        handleHandleRelease();
    } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup) {
        handleGroupRelease();
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsToolView::handleLineModeRelease(QMouseEvent *event)
{
    // 如果按住 Shift 键，限制方向
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
    qDebug() << "Handle drag released.";
    draggedHandle = nullptr;
    draggedItem = nullptr;
}

void GraphicsToolView::handleGroupRelease()
{
    qDebug() << "Selected group drag released.";
    qDebug() << "Ctrl key state for copy: " << isCtrlPressedForCopy;
    if (isCtrlPressedForCopy && !selectedItems.isEmpty()) {
        qDebug() << "Ctrl key was pressed during drag. Copying selected items.";
        QList<QGraphicsItem*> newItems;
        for (QGraphicsItem* item : selectedItems) {
            if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                QGraphicsItem* newItem = editableLine->clone();
                if (newItem) {
                    // 添加一个偏移量，避免新复制的项与原项完全重叠
                    newItem->setPos(item->pos() + QPointF(20, 20));
                    scene()->addItem(newItem);
                    newItems.append(newItem);
                    qDebug() << "Copied item at position: " << newItem->pos();
                }
            }
            // 可以添加对其他类型的图形项的支持
        }
        // 更新选中项为新复制的项（可选）
        cleanupSelection();
        for (QGraphicsItem* newItem : newItems) {
            if (EditableLineItem* newEditableLine = dynamic_cast<EditableLineItem*>(newItem)) {
                newEditableLine->setSelectedState(true);
                selectedItems.append(newItem);
            }
        }
        qDebug() << "Copied items during drag. Total new selected items: " << selectedItems.size();
    } else {
        qDebug() << "No copy performed. Ctrl pressed: " << isCtrlPressedForCopy << ", Selected items count: " << selectedItems.size();
    }
    isDraggingSelectionGroup = false;
    isCtrlPressedForCopy = false; // 重置状态
}





void GraphicsToolView::cleanupDrawing()
{
    qDebug() << "Cleaning up drawing state.";
    // 清理预览线条
    if (previewLine) {
        scene()->removeItem(previewLine);
        delete previewLine;
        previewLine = nullptr;
    }
    // 重置起点和终点
    startPoint = QPointF();
    endPoint = QPointF();
    // 重置折线状态
    polylinePoints.clear();
    if (currentPolyline) {
        scene()->removeItem(currentPolyline);
        delete currentPolyline;
        currentPolyline = nullptr;
    }
    isDrawingPolyline = false;
    // 重置拖动状态
    isDragging = false;
}

void GraphicsToolView::cleanupSelection()
{
    qDebug() << "Cleaning up selection state.";
    // 取消所有选中项的选中状态并恢复默认样式
    for (QGraphicsItem* item : selectedItems) {
        if (item) {
            if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                editableLine->setSelectedState(false); // 隐藏端点
            }
        }
    }
    selectedItems.clear(); // 清空选中列表
    copiedItems.clear(); // 清空选中列表
    draggedHandle = nullptr; // 重置拖动端点状态
    draggedItem = nullptr; // 重置拖动项状态
    isDraggingSelectionGroup = false; // 重置拖动组状态

}



void GraphicsToolView::copySelectedItems()
{
    qDebug() << "Copying selected items.";
    copiedItems.clear(); // 清空之前的复制内容
    for (QGraphicsItem* item : selectedItems) {
        // 检查当前项是否是 EditableLineItem
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            if (item->parentItem() == nullptr || dynamic_cast<EditableLineItem*>(item->parentItem()) == nullptr) {
                copiedItems.append(item);
            }
        }
    }
    qDebug() << "Total copied items:" << copiedItems.size();
}

void GraphicsToolView::pasteCopiedItems()
{
    qDebug() << "Pasting copied items.";
    if (copiedItems.isEmpty()) {
        qDebug() << "No items to paste.";
        return;
    }


    // 偏移量，用于避免粘贴的图形与原图形完全重叠
    QPointF pasteOffset(20, 20);
    QGraphicsScene* currentScene = scene();
    if (!currentScene) {
        qDebug() << "错误: 视图没有关联的场景.";
        return;
    }
    QList<QGraphicsItem*> newlyPastedItems;

    for (QGraphicsItem* item : copiedItems) {
        QGraphicsItem* newItem = nullptr;

        // 尝试将 item 转换为 EditableLineItem
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            newItem = editableLine->clone();
        } else {
            if (QGraphicsRectItem* rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
                newItem = new QGraphicsRectItem(rectItem->rect());
                static_cast<QGraphicsRectItem*>(newItem)->setPen(rectItem->pen());
                static_cast<QGraphicsRectItem*>(newItem)->setBrush(rectItem->brush());
            }
            // 添加其他标准类型的复制逻辑...
            else {
                qDebug() << "警告: 无法粘贴不受支持或无法克隆的图形项类型.";
            }
        }
        if (newItem) {
            // 应用偏移量并添加到场景
            newItem->setPos(item->pos() + pasteOffset);
            currentScene->addItem(newItem);
            newlyPastedItems.append(newItem);
        }

    }

    // 清空当前选中项，并将粘贴的项设为选中状态
    cleanupSelection();
    // 可选：清除原图形项的选中状态，并选中新粘贴的图形项
    for (QGraphicsItem* pastedItem : newlyPastedItems) {
        if(EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(pastedItem)){
            editableLine->setSelectedState(true);
            selectedItems.append(editableLine);
            copiedItems.append(editableLine);
        }
    }
    qDebug() << "Total pasted items:" << selectedItems.size();
}

