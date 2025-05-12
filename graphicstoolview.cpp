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
    draggedItem(nullptr)
{
    setRenderHint(QPainter::Antialiasing);
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
    default:
        break;
    }
}

void GraphicsToolView::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "mousePressEvent of GraphicsToolView";

    if (currentMode == DrawingMode::None) {
        handleNoneModePress(event);
    } else if (currentMode == DrawingMode::Line) {
        handleLineModePress(event);
    }

    QGraphicsView::mousePressEvent(event);
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

    // Optional: Prevent the event from being propagated further if you've handled it completely.
    // 如果不希望事件继续传播到父控件，可以调用 accept() 或 ignore()。
    // event->accept(); // 表示事件已被处理
    // event->ignore(); // 表示事件未被处理，继续传播

    // Call the base class implementation to ensure other default handling is done (if any).
    // 如果 QGraphicsView 的基类有默认的滚轮事件处理，建议调用它。
    // QGraphicsView::wheelEvent(event); // 如果 GraphicsToolView 直接继承自 QGraphicsView
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
        event->accept(); // Accept the event to prevent further processing
        return; // Exit the function after handling
    }

    // Handle Shift key for line drawing restriction
    if (event->key() == Qt::Key_Shift) {
        if (currentMode == DrawingMode::Line) {
            isShiftPressed = true;
            qDebug() << "Shift pressed. Restricting line to horizontal or vertical.";
        }
        // No event->accept() here, allow Shift to be combined with other keys
    }

    // Handle Copy (Ctrl+C) and Paste (Ctrl+V) shortcuts
    // Check for specific combinations first to avoid double handling

    // Handle Ctrl+Shift+C
    if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_C) {
        qDebug() << "Ctrl + Shift + C detected. Copying selected items.";
        copySelectedItems();
        event->accept();
        return;
    }

    // Handle Ctrl+Shift+V
    if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_V) {
        qDebug() << "Ctrl + Shift + V detected. Pasting copied items (Ctrl+Shift+V).";
        pasteCopiedItems();
        event->accept();
        return;
    }

    // Handle Ctrl+C
    // 移除排除 Shift 的条件，只检查是否按下了 Ctrl 和 C
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_C) {
        qDebug() << "Ctrl + C detected. Copying selected items.";
        copySelectedItems();
        event->accept();
        return;
    }

    // Handle Ctrl+V
    // 移除排除 Shift 的条件，只检查是否按下了 Ctrl 和 V
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_V) {
        qDebug() << "Ctrl + V detected. Pasting copied items (Ctrl+V).";
        pasteCopiedItems();
        event->accept();
        return;
    }


    // Call the base class implementation for unhandled events
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
                    }
                    qDebug() << "Item added to selection (Multi-Select) at position:" << scenePos;
                    qDebug() << "Total selected items:" << selectedItems.size();
                }
            } else {
                cleanupSelection();
                selectedItems.append(item);
                if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                    editableLine->setSelectedState(true);
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
        scene()->addItem(line);
        qDebug() << "Line created from" << startPoint << "to" << endPoint;
        qDebug() << "Drawing completed. Resetting points.";
        cleanupDrawing();
    }
}

void GraphicsToolView::mouseMoveEvent(QMouseEvent *event)
{
    if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
        handleLineModeMove(event);
    } else if (currentMode == DrawingMode::None && draggedHandle && draggedItem) {
        handleHandleMove(event);
    } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup && !selectedItems.isEmpty()) {
        handleGroupMove(event);
    }
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
        scene()->addItem(previewLine);
    }
    qDebug() << "End Point set to:" << endPoint;
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
    if (startPoint != endPoint) {
        QGraphicsLineItem *line = new EditableLineItem(startPoint, endPoint);
        scene()->addItem(line);
        cleanupDrawing();
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
    isDraggingSelectionGroup = false;
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
            // 进一步检查这个 EditableLineItem 是否是顶层项
            // 也就是说，它的父项是否为 nullptr，或者父项不是另一个 EditableLineItem
            // 这样可以避免复制作为另一个 EditableLineItem 子项的 EditableLineItem (如果存在这种情况)
            // 更重要的是，这可以确保我们不会复制 EditableLineItem 的 handles，因为 handles 的父项就是 EditableLineItem
            if (item->parentItem() == nullptr || dynamic_cast<EditableLineItem*>(item->parentItem()) == nullptr) {
                copiedItems.append(item); // 将顶层的 EditableLineItem 添加到复制列表
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

