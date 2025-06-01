#include "graphics_tool_view.h"
#include "editable_line_item.h"
#include "custom_rect_item.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QCursor>
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
    drawingColor(Qt::black),
    isDrawingPolyline(false),
    draggedHandleIndex(-1),
    drawingPenWidth(2), //  初始化线条粗细为2
    drawingLineStyle(LineStyle::SolidLine), // 初始化线条样式为实线
    rubberBand(nullptr), // 初始化 rubberBand
    isSelectingWithRubberBand(false), // 初始化 isSelectingWithRubberBand
    closePolylineOnFinish(false),
    previewClosingSegment(nullptr),
    previewRect(nullptr),
    currentArcState(ArcDrawingState::DefineCenter),
    previewArc(nullptr),
    previewPolygon(nullptr),
    drawingFillColor(Qt::transparent)
{
    setRenderHint(QPainter::Antialiasing); // 设置抗锯齿
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
    qDebug() << "选色:" << color.name();
}

void GraphicsToolView::setDrawingMode(DrawingMode mode)
{
    qDebug() << "模式设为:" << static_cast<int>(mode);
    if (currentMode == DrawingMode::Polyline && isDrawingPolyline) {
        qDebug() << "切换模式结束折线";
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
    case DrawingMode::Polygon:
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
        qDebug() << "进入多边形模式";
        break;
    case DrawingMode::Text:
        setCursor(Qt::IBeamCursor);
        setMouseTracking(false);
        qDebug() << "进入文本模式";
        break;
    case DrawingMode::Ellipse:
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
        qDebug() << "进入椭圆模式";
        break;
    case DrawingMode::Rectangle:
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
        qDebug() << "进入矩形模式";
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
    case DrawingMode::Arc:
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
        currentArcState = ArcDrawingState::DefineCenter;
        qDebug() << "进入圆弧模式";
        break;
    default:
        setCursor(Qt::ArrowCursor);
        setMouseTracking(false);
        break;
    }
}

void GraphicsToolView::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "鼠标按下:" << event->button();
    bool isCtrlPressed = event->modifiers() & Qt::ControlModifier;
    if (currentMode == DrawingMode::None) {
        qDebug() << "无模式按下";
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsItem *itemUnderMouse = scene()->itemAt(scenePos, transform());
        bool handleHit = checkHandleHit(scenePos); // 检查是否点中控制点

        if (event->button() == Qt::LeftButton && !itemUnderMouse && !handleHit) { // 只有在没有点中item且没有点中handle时才开始框选
            isSelectingWithRubberBand = true;
            rubberBandOrigin = event->pos();
            if (!rubberBand) {
                rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
            }
            rubberBand->setGeometry(QRect(rubberBandOrigin, QSize()));
            rubberBand->show();
            qDebug() << "开始框选，起点:" << rubberBandOrigin;
            event->accept(); // 事件已处理
            return; // 开始框选，不继续执行下面的 handleNoneModePress
        }
        // 如果不是开始框选，则执行原有的None模式处理
        handleNoneModePress(event);
    } else if (currentMode == DrawingMode::Line) {
        qDebug() << "直线模式按下";
        handleLineModePress(event);
    } else if (currentMode == DrawingMode::Polyline) {
        if (event->button() == Qt::LeftButton) {
            qDebug() << "折线模式左键按下";
            handlePolylineModePress(event);
        }
    } else if (currentMode == DrawingMode::Rectangle) {
        handleRectangleModePress(event);
    }else if (currentMode == DrawingMode::Ellipse) {
        handleEllipseModePress(event);
    }else if (currentMode == DrawingMode::Arc) {
        handleArcModePress(event);
    }else if (currentMode == DrawingMode::Polygon) {
        if (event->button() == Qt::LeftButton) {
            handlePolygonModePress(event);
        } else if (event->button() == Qt::RightButton && isDrawingPolyline) {
            finishPolygon();
        }
    }else if (currentMode == DrawingMode::Text) {
        handleTextModePress(event);
        event->accept();
    }
    QGraphicsView::mousePressEvent(event);
}

void GraphicsToolView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    updateCursorBasedOnPosition(scenePos);

    if (isSelectingWithRubberBand && rubberBand) {
        rubberBand->setGeometry(QRect(rubberBandOrigin, event->pos()).normalized());
        event->accept();
        return; // 框选时，不执行其他移动逻辑
    }

    if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
        handleLineModeMove(event);
    } else if (currentMode == DrawingMode::Polyline && isDrawingPolyline && isDragging) {
        handlePolylineModeMove(event);
    } else if (currentMode == DrawingMode::None && draggedHandle && draggedItem) {
        handleHandleMove(event);
    } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup && !selectedItems.isEmpty()) {
        handleGroupMove(event);
    }else if (currentMode == DrawingMode::Rectangle) {
        handleRectangleModeMove(event);
    }else if (currentMode == DrawingMode::Ellipse) {
        handleEllipseModeMove(event);
    }else if (currentMode == DrawingMode::Arc) {
        if (isDragging || currentArcState == ArcDrawingState::DefineRadiusStart || currentArcState == ArcDrawingState::DefineEnd) {
            handleArcModeMove(event);
        }
    }else if (currentMode == DrawingMode::Polygon && isDrawingPolyline) {
        handlePolygonModeMove(event);
    }
}

void GraphicsToolView::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "鼠标释放";
    if (isSelectingWithRubberBand && rubberBand && event->button() == Qt::LeftButton) {
        rubberBand->hide();
        QRect selectionRectView = rubberBand->geometry();
        QRectF selectionRectScene = mapToScene(selectionRectView).boundingRect();

        qDebug() << "框选结束，选择框 (视图):" << selectionRectView;
        qDebug() << "框选结束，选择框 (场景):" << selectionRectScene;

        bool isMultiSelect = event->modifiers() & Qt::ControlModifier;
        if (!isMultiSelect) {
            cleanupSelection(); // 非多选（Ctrl）时，先清空之前的选择
        }

        QList<QGraphicsItem*> itemsInRect = scene()->items(selectionRectScene, Qt::IntersectsItemShape);
        for (QGraphicsItem *item : itemsInRect) {
            if (dynamic_cast<EditableLineItem*>(item) || dynamic_cast<EditablePolylineItem*>(item) ||
                dynamic_cast<QGraphicsRectItem*>(item) || dynamic_cast<QGraphicsEllipseItem*>(item) ||
                dynamic_cast<QGraphicsPathItem*>(item) || dynamic_cast<QGraphicsPolygonItem*>(item) ||
                dynamic_cast<QGraphicsTextItem*>(item)) { // 只选择我们可操作的图形类型
                if (!selectedItems.contains(item)) {
                    selectedItems.append(item);
                    if (EditableLineItem* el = dynamic_cast<EditableLineItem*>(item)) el->setSelectedState(true);
                    else if (EditablePolylineItem* ep = dynamic_cast<EditablePolylineItem*>(item)) ep->setSelectedState(true);
                    // 对于其他类型的 QGraphicsItem，它们没有 setSelectedState，选中状态由QGraphicsView管理
                    else item->setSelected(true);
                }
            }
        }
        qDebug() << "框选选中 " << selectedItems.count() << " 个项目。";

        isSelectingWithRubberBand = false;
        event->accept(); // 事件已处理
        return;
    }

    if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
        // 直线模式释放处理
    } else if (currentMode == DrawingMode::None && draggedHandle) {
        handleHandleRelease();
    } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup) {
        handleGroupRelease();
    }else if (currentMode == DrawingMode::Rectangle) {
        handleRectangleModeRelease(event);
    }else if (currentMode == DrawingMode::Ellipse && isDragging) {
        handleEllipseModeRelease(event);
    }else if (currentMode == DrawingMode::Arc && isDragging) {
        // 圆弧模式释放
    }

    if (!event->isAccepted()) {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void GraphicsToolView::mouseDoubleClickEvent(QMouseEvent *event)
{
    qDebug() << "双击事件 按钮:" << event->button() << "模式:" << static_cast<int>(currentMode) << "是否折线:" << isDrawingPolyline;
    if (event->button() == Qt::LeftButton && currentMode == DrawingMode::Polyline && isDrawingPolyline) {
        qDebug() << "折线双击: 结束绘制";
        finishPolyline();
        event->accept();
    } else if (event->button() == Qt::LeftButton && currentMode == DrawingMode::Polygon && isDrawingPolyline) {
        qDebug() << "多边形双击: 结束绘制";
        finishPolygon();
        event->accept();
    } else {
        qDebug() << "非折线/多边形双击，传给基类";
        QGraphicsView::mouseDoubleClickEvent(event);
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
    qDebug() << "按键事件:" << event->key();
    if (event->key() == Qt::Key_Escape) {
        if (currentMode != DrawingMode::None || isDrawingPolyline) {
            qDebug() << "Esc取消绘制";
            cleanupDrawing();
            setDrawingMode(DrawingMode::None);
        } else if (!selectedItems.isEmpty()) {
            qDebug() << "Esc清除选中";
            cleanupSelection();
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Shift) {
        if (currentMode == DrawingMode::Line) {
            isShiftPressed = true;
            qDebug() << "Shift限制直线方向";
        }
    } else if (event->key() == Qt::Key_C) {
        if (currentMode == DrawingMode::Polyline && isDrawingPolyline) {
            qDebug() << "C尝试闭合折线";
            if (polylinePoints.size() >= 2) {
                closePolylineOnFinish = true;
                finishPolyline();
            } else {
                qDebug() << "点数不足无法闭合";
                closePolylineOnFinish = false;
                finishPolyline();
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
        qDebug() << "释放Shift不限制直线方向";
        if (currentMode == DrawingMode::Polyline && previewLine && !polylinePoints.isEmpty()) {
            updatePolylinePreview(previewLine->line().p2());
        }
    }
    QGraphicsView::keyReleaseEvent(event);
}

void GraphicsToolView::finishPolyline()
{
    qDebug() << "结束折线绘制, 是否绘制中:" << isDrawingPolyline << "点数:" << polylinePoints.size();
    if (isDrawingPolyline) {
        if (currentPolyline && polylinePoints.size() >= 2) {
            qDebug() << "折线绘制完成, 点数:" << polylinePoints.size();
            if (closePolylineOnFinish && polylinePoints.size() >= 3) {
                currentPolyline->setClosed(true);
                qDebug() << "折线设为闭合";
            } else {
                currentPolyline->setClosed(false);
                qDebug() << "折线设为开放";
            }
            currentPolyline = nullptr;
        } else if (currentPolyline) {
            qDebug() << "折线点数不足, 丢弃";
            scene()->removeItem(currentPolyline);
            delete currentPolyline;
            currentPolyline = nullptr;
        } else {
            qDebug() << "无折线或点数不足";
        }

        polylinePoints.clear();
        isDrawingPolyline = false;
        isDragging = false;

        if (previewLine) {
            scene()->removeItem(previewLine);
            delete previewLine;
            previewLine = nullptr;
        }
        if (previewClosingSegment) {
            scene()->removeItem(previewClosingSegment);
            delete previewClosingSegment;
            previewClosingSegment = nullptr;
        }
        closePolylineOnFinish = false;

        qDebug() << "折线绘制结束, 重置无模式";
        setDrawingMode(DrawingMode::None);
    } else {
        qDebug() << "不在折线绘制模式";
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
        qDebug() << "开始新折线:" << scenePos;
    } else {
        if (polylinePoints.isEmpty() || polylinePoints.last() != scenePos) {
            polylinePoints.append(scenePos);
            qDebug() << "添加折线点:" << scenePos;
        }
        if (polylinePoints.size() >= 2) {
            if (currentPolyline) {
                scene()->removeItem(currentPolyline);
                delete currentPolyline;
                currentPolyline = nullptr;
            }
            currentPolyline = new EditablePolylineItem(polylinePoints);
            QPen pen = currentPolyline->pen();
            pen.setColor(drawingColor);
            pen.setWidth(drawingPenWidth);
            pen.setStyle(Qt::PenStyle(drawingLineStyle));
            currentPolyline->setPen(pen);
            scene()->addItem(currentPolyline);
            qDebug() << "更新折线, 点数:" << polylinePoints.size();
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
        pen.setStyle(Qt::PenStyle(drawingLineStyle));
        pen.setWidth(drawingPenWidth);
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
    updatePolylinePreview(scenePos);
}

// 更新折线预览
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

    // 更新主预览线
    if (previewLine) {
        previewLine->setLine(lastPoint.x(), lastPoint.y(), effectiveMousePos.x(), effectiveMousePos.y());
    } else {
        previewLine = new EditableLineItem(lastPoint, effectiveMousePos);
        QPen pen = previewLine->pen();
        pen.setColor(drawingColor);
        pen.setStyle(Qt::PenStyle(drawingLineStyle));
        pen.setWidth(drawingPenWidth);
        previewLine->setPen(pen);
        scene()->addItem(previewLine);
    }
    previewLine->setVisible(true);

    // 更新闭合预览线段
    if (closePolylineOnFinish && polylinePoints.size() >= 1) {
        if (!previewClosingSegment) {
            previewClosingSegment = new EditableLineItem(effectiveMousePos, polylinePoints.first());
            QPen pen = previewClosingSegment->pen();
            pen.setColor(drawingColor);
            pen.setStyle(Qt::PenStyle(drawingLineStyle));
            pen.setWidth(drawingPenWidth);
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
    // QGraphicsItem *item = scene()->itemAt(scenePos, transform()); // 这行可以移除，itemUnderMouse 在 mousePressEvent 中获取
    qreal tolerance = 10.0;

    if (checkHandleHit(scenePos)) { // 优先检查是否点中控制点
        event->accept();
        return;
    }

    // 检查是否点中已选中的图形项以进行拖动
    // 注意：这里需要区分是单击选中还是开始拖动。
    // 如果是单击，则执行 handleItemSelection。
    // 如果是按下并准备拖动已选中的项，则 checkSelectedGroupHit 应该处理。
    // 框选的逻辑已经提前处理了。

    bool groupHit = false;
    for (QGraphicsItem* selItem : selectedItems) {
        QRectF boundingRect = selItem->boundingRect();
        // 稍微扩大一点边界，方便点击
        boundingRect.adjust(-tolerance / 2, -tolerance / 2, tolerance / 2, tolerance / 2);
        if (selItem->mapToScene(boundingRect).boundingRect().contains(scenePos)) {
            groupHit = true;
            break;
        }
    }

    if (groupHit) {
        // 点中了已选中的某个项目，准备拖动组
        if (event->button() == Qt::LeftButton) {
            isDraggingSelectionGroup = true;
            dragStartPosition = scenePos; // 记录拖动起始点（场景坐标）
            lastDragPos = scenePos;       // 初始化上次拖动位置
            this->isCtrlPressedForCopy = event->modifiers() & Qt::ControlModifier;
            qDebug() << "开始拖动选中组:" << scenePos << "Ctrl状态:" << isCtrlPressedForCopy;
            event->accept();
            return;
        }
    }

    // 如果没有点中控制点，也没有点中已选中的组（或者不是左键按下），则处理单个项目选择
    handleItemSelection(scenePos, tolerance, event->modifiers() & Qt::ControlModifier);
    event->accept(); // 标记事件已处理
}
// void GraphicsToolView::handleNoneModePress(QMouseEvent *event)
// {
//     QPointF scenePos = mapToScene(event->pos());
//     QGraphicsItem *item = scene()->itemAt(scenePos, transform());
//     qreal tolerance = 10.0;
//     if (checkHandleHit(scenePos)) {
//         return;
//     }
//     if (checkSelectedGroupHit(scenePos, tolerance)) {
//         this->isCtrlPressedForCopy = event->modifiers() & Qt::ControlModifier;
//         qDebug() << "Ctrl复制:" << isCtrlPressedForCopy;
//         return;
//     }
//     handleItemSelection(scenePos, tolerance, event->modifiers() & Qt::ControlModifier);
// }

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
                qDebug() << "选中起点端点:" << scenePos;
                return true;
            } else if (endHandle && endHandle->isVisible() && endHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = endHandle;
                draggedItem = item;
                qDebug() << "选中终点端点:" << scenePos;
                return true;
            } else if (rotationHandle && rotationHandle->isVisible() && rotationHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = rotationHandle;
                draggedItem = item;
                fixedRotationCenter = editableLine->mapToScene(editableLine->getRotationHandle()->pos());
                qDebug() << "旋转中心固定:" << fixedRotationCenter;
                qDebug() << "选中旋转端点:" << scenePos;
                return true;
            } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                QVector<HandleItem*> handles = editablePolyline->getHandles();
                for (int i = 0; i < handles.size(); ++i) {
                    HandleItem* handle = handles[i];
                    if (handle && handle->isVisible() && handle->sceneBoundingRect().contains(scenePos)) {
                        draggedHandle = handle;
                        draggedItem = item;
                        draggedHandleIndex = i;
                        qDebug() << "选中折线端点 索引:" << i << "位置:" << scenePos;
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
            qDebug() << "开始拖动选中组:" << scenePos;
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
                    qDebug() << "添加图形到多选:" << scenePos;
                    qDebug() << "选中总数:" << selectedItems.size();
                }
            } else {
                cleanupSelection();
                selectedItems.append(item);
                if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                    editableLine->setSelectedState(true);
                } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                    editablePolyline->setSelectedState(true);
                }
                qDebug() << "选中图形(单选):" << scenePos;
            }
            itemSelected = true;
            break;
        }
    }
    if (!itemSelected) {
        cleanupSelection();
        qDebug() << "点击空白清空选中";
    } else {
        qDebug() << "选中总数:" << selectedItems.size();
    }
}

void GraphicsToolView::handleLineModePress(QMouseEvent *event)
{
    qDebug()<<"选中项数量:"<< selectedItems.size();
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
        qDebug() << "直线模式: 起点:" << startPoint;
        if (previewLine) {
            scene()->removeItem(previewLine);
            delete previewLine;
            previewLine = nullptr;
        }
        previewLine = new EditableLineItem(startPoint, startPoint);
        QPen pen = previewLine->pen();
        pen.setColor(drawingColor);
        pen.setStyle(Qt::PenStyle(drawingLineStyle));
        pen.setWidth(drawingPenWidth);
        previewLine->setPen(pen);
        previewLine->setSelected(false);
        previewLine->setFlag(QGraphicsItem::ItemIsSelectable, false);
        scene()->addItem(previewLine);
    } else {
        endPoint = scenePos;
        qDebug() << "直线模式: 终点:" << endPoint;
        EditableLineItem *line = new EditableLineItem(startPoint, endPoint);
        QPen pen = line->pen();
        pen.setColor(drawingColor);
        pen.setStyle(Qt::PenStyle(drawingLineStyle));
        pen.setWidth(drawingPenWidth);
        line->setPen(pen);
        line->setSelected(false);
        scene()->addItem(previewLine);
        previewLine = nullptr;
        qDebug() << "创建直线:" << startPoint << "到" << endPoint;
        cleanupDrawing();
        setDrawingMode(DrawingMode::None); selectedItems.clear();
    }
}

void GraphicsToolView::updateCursorBasedOnPosition(const QPointF &scenePos)
{
    bool isOverSelectedItem = false;
    qreal tolerance = 10.0;
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
            pen.setStyle(Qt::PenStyle(drawingLineStyle));
            pen.setWidth(drawingPenWidth);
            editableLine->setPen(pen);
        } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
            QPen pen = editablePolyline->pen();
            pen.setColor(color);
            pen.setStyle(Qt::PenStyle(drawingLineStyle));
            pen.setWidth(drawingPenWidth);
            editablePolyline->setPen(pen);
        }
    }
}

void GraphicsToolView::handleHandleMove(QMouseEvent *event)
{
    if (!draggedItem || !draggedHandle) {
        qWarning() << "无效拖动";
        return;
    }
    QPointF newPos = mapToScene(event->pos());
    qDebug() << "视图坐标:" << event->pos();
    qDebug() << "场景位置:" << newPos;
    if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(draggedItem)) {
        QLineF currentLine = editableLine->line();
        QPointF sceneP1 = editableLine->mapToScene(currentLine.p1());
        QPointF sceneP2 = editableLine->mapToScene(currentLine.p2());
        qDebug() << "直线场景坐标: P1=" << sceneP1 << ", P2=" << sceneP2;
        if (draggedHandle == editableLine->getStartHandle()) {
            editableLine->updateLine(newPos, sceneP2);
            qDebug() << "拖动起点到:" << newPos;
        } else if (draggedHandle == editableLine->getEndHandle()) {
            editableLine->updateLine(sceneP1, newPos);
            qDebug() << "拖动终点到:" << newPos;
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
            qDebug() << "拖动旋转点到:" << newPos << ", 旋转中心:" << fixedRotationCenter;
            qDebug() << "当前角度:" << currentAngle << ", 目标角度:" << targetAngle << ", 角度差:" << angleDiff << ", 新旋转角度:" << newRotation;
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
    qDebug() << "拖动选中组, 偏移量:" << offset;
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
    qDebug() << "端点释放";
    draggedHandle = nullptr;
    draggedItem = nullptr;
}

void GraphicsToolView::handleGroupRelease()
{
    qDebug() << "选中组释放";
    qDebug() << "Ctrl复制状态:" << isCtrlPressedForCopy;
    if (isCtrlPressedForCopy && !selectedItems.isEmpty()) {
        qDebug() << "拖动时Ctrl复制";
        QList<QGraphicsItem*> newItems;
        for (QGraphicsItem* item : selectedItems) {
            if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                QGraphicsItem* newItem = editableLine->clone();
                if (newItem) {
                    newItem->setPos(item->pos() + QPointF(20, 20));
                    scene()->addItem(newItem);
                    newItems.append(newItem);
                    qDebug() << "复制图形:" << newItem->pos();
                }
            }
        }
        cleanupSelection();
        for (QGraphicsItem* newItem : newItems) {
            if (EditableLineItem* newEditableLine = dynamic_cast<EditableLineItem*>(newItem)) {
                newEditableLine->setSelectedState(true);
                selectedItems.append(newItem);
                copiedItems.append(newItem);
            }
        }
        qDebug() << "复制完成, 新选中数:" << selectedItems.size();
    } else {
        qDebug() << "未复制, Ctrl状态:" << isCtrlPressedForCopy << ", 选中数:" << selectedItems.size();
    }
    isDraggingSelectionGroup = false;
    isCtrlPressedForCopy = false;
}

void GraphicsToolView::cleanupDrawing()
{
    qDebug() << "清理绘制状态, 模式:" << static_cast<int>(currentMode);
    if (previewLine) {
        scene()->removeItem(previewLine);
        delete previewLine;
        previewLine = nullptr;
    }
    if (previewClosingSegment) {
        scene()->removeItem(previewClosingSegment);
        delete previewClosingSegment;
        previewClosingSegment = nullptr;
    }
    startPoint = QPointF();
    endPoint = QPointF();

    if (isDrawingPolyline || !polylinePoints.isEmpty() || currentPolyline) {
        qDebug() << "清理折线残留";
        polylinePoints.clear();
        if (currentPolyline) {
            scene()->removeItem(currentPolyline);
            delete currentPolyline;
            currentPolyline = nullptr;
        }
        isDrawingPolyline = false;
    }
    closePolylineOnFinish = false;
    isDragging = false;
}

void GraphicsToolView::cleanupSelection()
{
    qDebug() << "清理选中状态";
    if (isSelectingWithRubberBand && rubberBand) { // 如果正在框选，则取消
        rubberBand->hide();
        isSelectingWithRubberBand = false;
    }

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
    qDebug() << "复制总数:" << copiedItems.size();
}

void GraphicsToolView::pasteCopiedItems()
{
    qDebug() << "粘贴图形";
    if (copiedItems.isEmpty()) {
        qDebug() << "无图形可粘贴";
        return;
    }
    QPointF pasteOffset(20, 20);
    QGraphicsScene* currentScene = scene();
    if (!currentScene) {
        qDebug() << "错误: 视图无场景";
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
                qDebug() << "警告: 无法粘贴此类型图形";
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
    qDebug() << "粘贴总数:" << selectedItems.size();
}

// 处理椭圆模式鼠标按下
void GraphicsToolView::handleEllipseModePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        ellipseStartPoint = mapToScene(event->pos());
        isDragging = true;

        if (previewEllipse) {
            scene()->removeItem(previewEllipse);
            delete previewEllipse;
            previewEllipse = nullptr;
        }
        previewEllipse = new QGraphicsEllipseItem(QRectF(ellipseStartPoint, ellipseStartPoint));
        QPen pen(drawingColor, 1, Qt::DashLine);
        pen.setWidth(drawingPenWidth);
        previewEllipse->setPen(pen);
        scene()->addItem(previewEllipse);
        qDebug() << "椭圆绘制开始于:" << ellipseStartPoint;
    }
}

// 处理椭圆模式鼠标移动
void GraphicsToolView::handleEllipseModeMove(QMouseEvent *event)
{
    if (isDragging && previewEllipse) {
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF newRect = QRectF(ellipseStartPoint, currentScenePos).normalized();
        previewEllipse->setRect(newRect);
        qDebug() << "椭圆预览更新为:" << newRect;
    }
}

// 处理椭圆模式鼠标释放
void GraphicsToolView::handleEllipseModeRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF finalRect = QRectF(ellipseStartPoint, currentScenePos).normalized();

        if (previewEllipse) {
            scene()->removeItem(previewEllipse);
            delete previewEllipse;
            previewEllipse = nullptr;
        }

        if (finalRect.width() > 0 && finalRect.height() > 0) {
            QGraphicsEllipseItem *ellipseItem = new QGraphicsEllipseItem(finalRect);
            QPen pen(drawingColor);
            pen.setWidth(drawingPenWidth);
            pen.setStyle(Qt::PenStyle(drawingLineStyle));
            ellipseItem->setPen(pen);
            ellipseItem->setBrush(drawingFillColor);
            scene()->addItem(ellipseItem);
            qDebug() << "椭圆绘制完成:" << finalRect << ", 颜色:" << drawingColor;
        } else {
            qDebug() << "椭圆过小, 未绘制";
        }

        isDragging = false;
        ellipseStartPoint = QPointF();
    }
}

// 处理圆弧模式鼠标按下
void GraphicsToolView::handleArcModePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    QPointF scenePos = mapToScene(event->pos());

    if (currentArcState == ArcDrawingState::DefineCenter) {
        arcCenterPoint = scenePos;
        currentArcState = ArcDrawingState::DefineRadiusStart;
        qDebug() << "圆弧中心点:" << arcCenterPoint;

        if (previewArc) {
            scene()->removeItem(previewArc);
            delete previewArc;
        }
        previewArc = new QGraphicsPathItem();
        QPen pen(drawingColor, 1, Qt::DashLine);
        pen.setWidth(drawingPenWidth);
        pen.setStyle(Qt::PenStyle(drawingLineStyle));

        previewArc->setPen(pen);
        scene()->addItem(previewArc);

    } else if (currentArcState == ArcDrawingState::DefineRadiusStart) {
        arcRadiusStartPoint = scenePos;
        currentArcState = ArcDrawingState::DefineEnd;
        qDebug() << "圆弧半径/起点:" << arcRadiusStartPoint;
    } else if (currentArcState == ArcDrawingState::DefineEnd) {
        QPointF arcEndPoint = scenePos;
        qreal radius = QLineF(arcCenterPoint, arcRadiusStartPoint).length();
        if (radius <= 0) {
            cleanupDrawing();
            return;
        }

        qreal startAngleDegrees = QLineF(arcCenterPoint, arcRadiusStartPoint).angle();
        qreal endAngleDegrees = QLineF(arcCenterPoint, arcEndPoint).angle();

        qreal spanAngleDegrees = endAngleDegrees - startAngleDegrees;
        if (spanAngleDegrees < 0) {
            spanAngleDegrees += 360.0;
        }
        if (spanAngleDegrees > 360.0) {
            spanAngleDegrees = 360.0;
        }

        if (previewArc) {
            scene()->removeItem(previewArc);
            delete previewArc;
            previewArc = nullptr;
        }

        QPainterPath finalPath;
        QRectF boundingRect(arcCenterPoint.x() - radius, arcCenterPoint.y() - radius, 2 * radius, 2 * radius);
        finalPath.arcTo(boundingRect, startAngleDegrees, spanAngleDegrees);

        QGraphicsPathItem *arcItem = new QGraphicsPathItem(finalPath);
        QPen pen(drawingColor, 2);
        pen.setWidth(drawingPenWidth);
        pen.setStyle(Qt::PenStyle(drawingLineStyle));

        arcItem->setPen(pen);
        scene()->addItem(arcItem);

        qDebug() << "圆弧绘制: 中心" << arcCenterPoint << "半径" << radius
                 << "起始角" << startAngleDegrees << "跨越角" << spanAngleDegrees;

        cleanupDrawing();
    }
}

// 处理圆弧模式鼠标移动
void GraphicsToolView::handleArcModeMove(QMouseEvent *event)
{
    if (!isDragging && currentArcState == ArcDrawingState::DefineCenter) return;
    if (!previewArc) return;

    QPointF currentScenePos = mapToScene(event->pos());
    QPainterPath previewPath;

    if (currentArcState == ArcDrawingState::DefineRadiusStart) {
        previewPath.moveTo(arcCenterPoint);
        previewPath.lineTo(currentScenePos);
        previewArc->setPath(previewPath);
        qDebug() << "圆弧预览: 半径线到" << currentScenePos;
    } else if (currentArcState == ArcDrawingState::DefineEnd) {
        qreal radius = QLineF(arcCenterPoint, arcRadiusStartPoint).length();
        if (radius <= 0) return;

        qreal startAngleDegrees = QLineF(arcCenterPoint, arcRadiusStartPoint).angle();
        qreal endAngleDegrees = QLineF(arcCenterPoint, currentScenePos).angle();

        qreal spanAngleDegrees = endAngleDegrees - startAngleDegrees;
        if (spanAngleDegrees < 0) {
            spanAngleDegrees += 360.0;
        }
        if (spanAngleDegrees > 360.0) {
            spanAngleDegrees = 360.0;
        }

        QRectF boundingRect(arcCenterPoint.x() - radius, arcCenterPoint.y() - radius, 2 * radius, 2 * radius);
        previewPath.arcTo(boundingRect, startAngleDegrees, spanAngleDegrees);
        previewArc->setPath(previewPath);
        qDebug() << "圆弧预览: 结束角于" << currentScenePos << "跨度" << spanAngleDegrees;
    }
}

// 圆弧模式鼠标释放
void GraphicsToolView::handleArcModeRelease(QMouseEvent *event)
{
    if (currentArcState == ArcDrawingState::DefineEnd && isDragging) {
        QPointF arcEndPoint = mapToScene(event->pos());
        qreal radius = QLineF(arcCenterPoint, arcRadiusStartPoint).length();
        if (radius <= 0) {
            cleanupDrawing();
            return;
        }

        qreal startAngleDegrees = QLineF(arcCenterPoint, arcRadiusStartPoint).angle();
        qreal endAngleDegrees = QLineF(arcCenterPoint, arcEndPoint).angle();

        qreal spanAngleDegrees = endAngleDegrees - startAngleDegrees;
        if (spanAngleDegrees < 0) {
            spanAngleDegrees += 360.0;
        }
        if (spanAngleDegrees > 360.0) {
            spanAngleDegrees = 360.0;
        }

        if (previewArc) {
            scene()->removeItem(previewArc);
            delete previewArc;
            previewArc = nullptr;
        }

        QPainterPath finalPath;
        QRectF boundingRect(arcCenterPoint.x() - radius, arcCenterPoint.y() - radius, 2 * radius, 2 * radius);
        finalPath.arcTo(boundingRect, startAngleDegrees, spanAngleDegrees);

        QGraphicsPathItem *arcItem = new QGraphicsPathItem(finalPath);
        QPen pen(drawingColor, 2);
        pen.setWidth(drawingPenWidth);
        pen.setStyle(Qt::PenStyle(drawingLineStyle));

        arcItem->setPen(pen);
        scene()->addItem(arcItem);
        qDebug() << "圆弧绘制(释放): 中心" << arcCenterPoint << "半径" << radius
                 << "起始角" << startAngleDegrees << "跨越角" << spanAngleDegrees;

        cleanupDrawing();
        isDragging = false;
    }
}

// 处理多边形模式鼠标按下
void GraphicsToolView::handlePolygonModePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    QPointF scenePos = mapToScene(event->pos());

    if (!isDrawingPolyline) {
        polylinePoints.clear();
        polylinePoints.append(scenePos);
        isDrawingPolyline = true;
        isDragging = true;

        if (previewPolygon) {
            scene()->removeItem(previewPolygon);
            delete previewPolygon;
            previewPolygon = nullptr;
        }
        previewPolygon = new QGraphicsPolygonItem();
        QPen pen(drawingColor, 1, Qt::DashLine);
        pen.setWidth(drawingPenWidth);
        pen.setStyle(Qt::PenStyle(drawingLineStyle));

        previewPolygon->setPen(pen);
        scene()->addItem(previewPolygon);

        qDebug() << "多边形绘制开始于:" << scenePos;
    } else {
        polylinePoints.append(scenePos);
        qDebug() << "添加多边形点于:" << scenePos;
    }

    if (polylinePoints.size() >= 1 && previewPolygon) {
        QPolygonF currentQPolygon(polylinePoints);
        if (isDrawingPolyline && polylinePoints.size() > 0) {
            QPointF tempEndPoint = mapToScene(mapFromGlobal(QCursor::pos()));
            QPolygonF previewQPolygon = currentQPolygon;
            previewQPolygon.append(tempEndPoint);
            if (polylinePoints.size() >=2) {
                previewQPolygon.append(polylinePoints.first());
            }
            previewPolygon->setPolygon(previewQPolygon);
        }
    }
}

// 处理多边形模式鼠标移动
void GraphicsToolView::handlePolygonModeMove(QMouseEvent *event)
{
    if (!isDrawingPolyline || polylinePoints.isEmpty() || !previewPolygon) return;

    QPointF currentScenePos = mapToScene(event->pos());
    QPolygonF currentQPolygon(polylinePoints);
    currentQPolygon.append(currentScenePos);

    if (polylinePoints.size() >= 2) {
        currentQPolygon.append(polylinePoints.first());
    }
    previewPolygon->setPolygon(currentQPolygon);
    qDebug() << "多边形预览更新, 鼠标于:" << currentScenePos;
}

// 结束多边形绘制
void GraphicsToolView::finishPolygon()
{
    if (isDrawingPolyline) {
        if (previewPolygon) {
            scene()->removeItem(previewPolygon);
            delete previewPolygon;
            previewPolygon = nullptr;
        }

        if (polylinePoints.size() >= 3) {
            QPolygonF finalPolygon(polylinePoints);
            QGraphicsPolygonItem *polygonItem = new QGraphicsPolygonItem(finalPolygon);
            QPen pen(drawingColor, 2);
            pen.setWidth(drawingPenWidth);
            pen.setStyle(Qt::PenStyle(drawingLineStyle));

            polygonItem->setPen(pen);
            QBrush brush(drawingFillColor, Qt::SolidPattern);
            polygonItem->setBrush(brush);
            scene()->addItem(polygonItem);
            qDebug() << "多边形绘制完成, 顶点数:" << polylinePoints.size();
        } else {
            qDebug() << "多边形少于3顶点, 丢弃";
        }

        isDrawingPolyline = false;
        polylinePoints.clear();
        isDragging = false;
    }
}

// 处理文本模式鼠标按下
void GraphicsToolView::handleTextModePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());

        QGraphicsTextItem *textItem = new QGraphicsTextItem();
        textItem->setPlainText(QStringLiteral("测试文字"));
        textItem->setPos(scenePos);
        textItem->setDefaultTextColor(drawingColor);

        QFont font = textItem->font();
        font.setPointSize(12);
        textItem->setFont(font);

        textItem->setFlag(QGraphicsItem::ItemIsMovable);
        textItem->setFlag(QGraphicsItem::ItemIsSelectable);

        scene()->addItem(textItem);
        qDebug() << "文本项添加于:" << scenePos << ", 文字: '测试文字'";
    }
}

// 处理矩形模式鼠标按下
void GraphicsToolView::handleRectangleModePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        rectStartPoint = mapToScene(event->pos());
        isDragging = true;

        if (previewRect) {
            scene()->removeItem(previewRect);
            delete previewRect;
            previewRect = nullptr;
        }
        previewRect = new QGraphicsRectItem(QRectF(rectStartPoint, rectStartPoint));
        QPen pen(drawingColor, 1, Qt::DashLine);
        pen.setWidth(drawingPenWidth);
        pen.setStyle(Qt::PenStyle(drawingLineStyle));

        previewRect->setPen(pen);
        scene()->addItem(previewRect);
        qDebug() << "矩形绘制开始于:" << rectStartPoint;
    }
}

// 处理矩形模式鼠标移动
void GraphicsToolView::handleRectangleModeMove(QMouseEvent *event)
{
    if (isDragging && previewRect) {
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF newRect = QRectF(rectStartPoint, currentScenePos).normalized();
        previewRect->setRect(newRect);
        qDebug() << "矩形预览更新为:" << newRect;
    }
}

// 处理矩形模式鼠标释放
void GraphicsToolView::handleRectangleModeRelease(QMouseEvent *event)
{

    qDebug()<<"handleRectangleModeRelease";
    qDebug()<<fillImagePath;

    qDebug() << "handleRectangleModeRelease";
    qDebug() << fillImagePath << "===fillImagePath";

    if (event->button() == Qt::LeftButton && isDragging) {
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF finalRect = QRectF(rectStartPoint, currentScenePos).normalized();

        if (previewRect) {
            scene()->removeItem(previewRect);
            delete previewRect;
            previewRect = nullptr;
        }

        if (finalRect.width() > 0 && finalRect.height() > 0) {
            CustomRectItem *rectItem = new CustomRectItem(finalRect); // 使用自定义矩形项

            QPen pen(drawingColor, 2);
            pen.setWidth(drawingPenWidth);
            pen.setStyle(Qt::PenStyle(drawingLineStyle));

            rectItem->setPen(pen);
            // 优先使用图片填充，如果没有则使用颜色填充
            if (!fillImagePath.isEmpty() && !fillPixmap.isNull()) {
                // 缩放图片到矩形大小
                QPixmap scaledPixmap = fillPixmap.scaled(finalRect.size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                rectItem->setFillPixmap(scaledPixmap); // 设置填充图片
                qDebug() << "使用图片填充，矩形大小:" << finalRect.size() << "图片大小:" << scaledPixmap.size();
            } else {
                rectItem->setBrush(QBrush(drawingFillColor)); // 否则使用颜色填充
            }
            scene()->addItem(rectItem);
        } else {
            qDebug() << "矩形过小, 未绘制";
        }

        isDragging = false;
        rectStartPoint = QPointF();
    }
}

// 设置填充颜色
void GraphicsToolView::setDrawingFillColor(const QColor &color)
{
    drawingFillColor = color;
    qDebug() << "填充颜色设为:" << color.name();
}

// 获取当前填充颜色
QColor GraphicsToolView::currentDrawingFillColor() const
{
    return drawingFillColor;
}

// 设置绘图颜色
void GraphicsToolView::setDrawingColor(const QColor &color)
{
    drawingColor = color;
    qDebug() << "绘图颜色设为:" << color.name();
}

// 获取当前绘图颜色
QColor GraphicsToolView::currentDrawingColor() const
{
    return drawingColor;
}

void GraphicsToolView::setDrawingFillImage(const QString &imagePath)
{
    fillImagePath = imagePath;
    if (!imagePath.isEmpty()) {
        fillPixmap.load(imagePath);
        if (fillPixmap.isNull()) {
            qDebug() << "Failed to load image for filling:" << imagePath;
            fillImagePath.clear(); // 清空路径以避免使用无效图片
        }
    } else {
        fillPixmap = QPixmap(); // 清空图片数据
    }
}


// 设置线条粗细
void GraphicsToolView::setDrawingPenWidth(int width)
{
    drawingPenWidth = width;
    qDebug() << "线条粗细设为:" << width;
}

// 设置线条样式
void GraphicsToolView::setDrawingLineStyle(LineStyle style)
{
    drawingLineStyle = style;
}


//  对齐选定项
void GraphicsToolView::alignSelectedItems(AlignmentType type)
{
    if (selectedItems.isEmpty()) {
        qDebug() << "没有选中的图形项进行对齐。";
        return;
    }

    // 计算选中项的共同边界矩形
    QRectF unionRect;
    for (QGraphicsItem *item : selectedItems) {
        if (unionRect.isNull()) {
            unionRect = item->sceneBoundingRect();
        } else {
            unionRect = unionRect.united(item->sceneBoundingRect());
        }
    }

    qDebug() << "对齐类型:" << type;
    qDebug() << "选中项共同边界矩形:" << unionRect;

    for (QGraphicsItem *item : selectedItems) {
        // 将项的当前位置转换为场景坐标
        QPointF currentScenePos = item->scenePos();
        QRectF itemRect = item->sceneBoundingRect();
        QPointF newScenePos = currentScenePos;

        switch (type) {
        case AlignLeft:
            newScenePos.setX(unionRect.left() - (itemRect.left() - currentScenePos.x()));
            break;
        case AlignRight:
            newScenePos.setX(unionRect.right() - itemRect.width() - (itemRect.left() - currentScenePos.x()));
            break;
        case AlignTop:
            newScenePos.setY(unionRect.top() - (itemRect.top() - currentScenePos.y()));
            break;
        case AlignBottom:
            newScenePos.setY(unionRect.bottom() - itemRect.height() - (itemRect.top() - currentScenePos.y()));
            break;
        case AlignCenterVertical:
            newScenePos.setY(unionRect.center().y() - itemRect.height() / 2.0 - (itemRect.top() - currentScenePos.y()));
            break;
        case AlignCenterHorizontal:
            newScenePos.setX(unionRect.center().x() - itemRect.width() / 2.0 - (itemRect.left() - currentScenePos.x()));
            break;
        }

        // 设置项的新位置 (场景坐标)
        item->setPos(newScenePos);
        qDebug() << "项" << item << "新位置:" << newScenePos;
    }
    scene()->update(); // 强制场景更新
}











