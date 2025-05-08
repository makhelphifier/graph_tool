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

}




void GraphicsToolView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (currentMode != DrawingMode::None) {
            qDebug() << "Esc pressed. Cancelling drawing.";
            cleanupDrawing();
            setDrawingMode(DrawingMode::None);
        } else {
            qDebug() << "Esc pressed. Clearing selection.";
            cleanupSelection();
        }
    }
    if (event->key() == Qt::Key_Shift) {
        if (currentMode == DrawingMode::Line) {
            isShiftPressed = true;
            qDebug() << "Shift pressed. Restricting line to horizontal or vertical.";
        }
    }
    // 添加复制 (Ctrl+C) 和粘贴 (Ctrl+V) 快捷键
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_C) {
            copySelectedItems();
        } else if (event->key() == Qt::Key_V) {
            pasteCopiedItems();
        }
    }

    if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
        if (event->key() == Qt::Key_C) {
            qDebug() << "Ctrl + Shift + C detected. Copying selected items.";
            copySelectedItems();
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
    QPointF newPos = mapToScene(event->pos());
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
    draggedHandle = nullptr; // 重置拖动端点状态
    draggedItem = nullptr; // 重置拖动项状态
    isDraggingSelectionGroup = false; // 重置拖动组状态

}



void GraphicsToolView::copySelectedItems()
{
    qDebug() << "Copying selected items.";
    copiedItems.clear(); // 清空之前的复制内容

    for (QGraphicsItem* item : selectedItems) {
        // 可以扩展支持其他类型的图形项
        copiedItems.append(item); // 存储复制的项
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
    const QPointF offset =QPointF(20.0,20.0) ;
    for (QGraphicsItem* item : selectedItems) {
        QGraphicsItem* newItem =item;
        QPointF newItemPos = item->pos() + offset;
        newItem->setPos(newItemPos);
        scene()->addItem(newItem);
        selectedItems.append(newItem);
        // newItem->setSelectedState(true);


    }

    // 清空当前选中项，并将粘贴的项设为选中状态
    cleanupSelection();
    qDebug() << "Total pasted items:" << selectedItems.size();
}

