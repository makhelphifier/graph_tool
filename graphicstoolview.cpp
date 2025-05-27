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
    previewClosingSegment(nullptr), // 初始化闭合预览线段,
    previewRect(nullptr),
    currentArcState(ArcDrawingState::DefineCenter), // 初始化圆弧绘制状态
    previewArc(nullptr),
    previewPolygon(nullptr) // 初始化预览多边形指针

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
    case DrawingMode::None: // 无模式
        setCursor(Qt::ArrowCursor); // 设置箭头光标
        setMouseTracking(false); // 关闭鼠标追踪
        break;
    case DrawingMode::Polygon: // 多边形模式
        setCursor(Qt::CrossCursor);
        setMouseTracking(true); // 多边形绘制通常需要鼠标追踪
        qDebug() << "Entered Polygon Mode";
        break;
    case DrawingMode::Text: // 文本模式
        setCursor(Qt::IBeamCursor); // 文本输入通常用 IBeam 光标
        setMouseTracking(false);    // 单击放置文本，不需要持续追踪
        qDebug() << "Entered Text Mode";
        break;
    case DrawingMode::Ellipse: // 椭圆模式也用十字光标
        setCursor(Qt::CrossCursor);
        setMouseTracking(true); // 椭圆绘制通常需要鼠标追踪
        qDebug() << "Entered Ellipse Mode";
        break;
    case DrawingMode::Rectangle: // 矩形模式也用十字光标
        setCursor(Qt::CrossCursor);
        setMouseTracking(true); // 矩形绘制通常需要鼠标追踪
        qDebug() << "Entered Rectangle Mode";
        break;
    case DrawingMode::Line: // 直线模式
        setCursor(Qt::CrossCursor); // 设置十字光标
        setMouseTracking(true); // 启用鼠标追踪
        qDebug() << "进入直线模式"; // 打印进入直线模式信息
        break;
    case DrawingMode::Polyline: // 折线模式
        setCursor(Qt::CrossCursor); // 设置十字光标
        setMouseTracking(true); // 启用鼠标追踪
        qDebug() << "进入折线模式"; // 打印进入折线模式信息
        break;
    case DrawingMode::Arc: // 圆弧模式
        setCursor(Qt::CrossCursor);
        setMouseTracking(true); // 圆弧绘制需要鼠标追踪
        currentArcState = ArcDrawingState::DefineCenter; // 重置圆弧绘制状态
        qDebug() << "Entered Arc Mode";
        break;
    default:
        setCursor(Qt::ArrowCursor); // 默认箭头光标
        setMouseTracking(false); // 关闭鼠标追踪
        break;
    }
}



void GraphicsToolView::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "鼠标按下事件，按钮：" << event->button(); // 打印鼠标按下信息
    bool isCtrlPressed = event->modifiers() & Qt::ControlModifier; // 检查Ctrl键状态
    if (currentMode == DrawingMode::None) {
        qDebug() << "处理无模式按下事件"; // 打印无模式处理信息
        handleNoneModePress(event); // 处理无模式按下
    } else if (currentMode == DrawingMode::Line) {
        qDebug() << "处理直线模式按下事件"; // 打印直线模式处理信息
        handleLineModePress(event); // 处理直线模式按下
    } else if (currentMode == DrawingMode::Polyline) {
        if (event->button() == Qt::LeftButton) {
            qDebug() << "处理折线模式按下事件（左键）"; // 打印折线模式左键信息
            handlePolylineModePress(event); // 处理折线模式按下
        }
    } else if (currentMode == DrawingMode::Rectangle) { // 新增
        handleRectangleModePress(event);
    }else if (currentMode == DrawingMode::Ellipse) { // 新增
        handleEllipseModePress(event);
    }else   if (currentMode == DrawingMode::Arc) {
        handleArcModePress(event); // 调用圆弧模式的按下处理
    }else if (currentMode == DrawingMode::Polygon) { // 新增多边形模式处理
        if (event->button() == Qt::LeftButton) {
            handlePolygonModePress(event);
        } else if (event->button() == Qt::RightButton && isDrawingPolyline) { // 右键结束绘制
            finishPolygon();
        }
    }else    if (currentMode == DrawingMode::Text) { // 新增文本模式处理
        handleTextModePress(event);
        event->accept(); // 标记事件已处理
    }
    QGraphicsView::mousePressEvent(event); // 调用基类处理
}



void GraphicsToolView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos()); // 映射到场景坐标
    updateCursorBasedOnPosition(scenePos); // 更新光标样式
    if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
        handleLineModeMove(event); // 处理直线模式移动
    } else if (currentMode == DrawingMode::Polyline && isDrawingPolyline && isDragging) {
        handlePolylineModeMove(event); // 处理折线模式移动
    } else if (currentMode == DrawingMode::None && draggedHandle && draggedItem) {
        handleHandleMove(event); // 处理端点移动
    } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup && !selectedItems.isEmpty()) {
        handleGroupMove(event); // 处理选中组移动
    }else if (currentMode == DrawingMode::Rectangle) { // 新增
        handleRectangleModeMove(event);
    }else if (currentMode == DrawingMode::Ellipse) { // 新增
        handleEllipseModeMove(event);
    }else if (currentMode == DrawingMode::Arc) { // 修改: 检查是否在拖动或处于特定Arc状态
        if (isDragging || currentArcState == ArcDrawingState::DefineRadiusStart || currentArcState == ArcDrawingState::DefineEnd) {
            handleArcModeMove(event);
        }
    }else if (currentMode == DrawingMode::Polygon && isDrawingPolyline) { // 新增
        handlePolygonModeMove(event);
    }

}

void GraphicsToolView::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "鼠标释放事件"; // 打印释放信息
    if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
        // 直线模式释放处理（未实现具体逻辑）
    } else if (currentMode == DrawingMode::None && draggedHandle) {
        handleHandleRelease(); // 处理端点释放
    } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup) {
        handleGroupRelease(); // 处理选中组释放
    }else if (currentMode == DrawingMode::Rectangle) { // 新增
        handleRectangleModeRelease(event);
    }else if (currentMode == DrawingMode::Ellipse && isDragging) { // 新增
        handleEllipseModeRelease(event);
    }else   if (currentMode == DrawingMode::Arc && isDragging) { // 修改: 检查是否在拖动绘制圆弧
        // 如果你的设计是第三次点击完成，则这里的release可能主要用于重置isDragging
        // 如果设计是第二次点击后拖动确定第三点，则在这里调用handleArcModeRelease
        // 根据你给的描述“第三次点击确定结束点（或者拖动确定结束角度）”，
        // 我们的press已经处理了第三次点击。如果拖动确定结束角度，则需要release
        // handleArcModeRelease(event); // 暂时注释，因为press处理了三点
    }

    if (!event->isAccepted()) {
        QGraphicsView::mouseReleaseEvent(event); // 调用基类处理
    }
}

// void GraphicsToolView::mouseReleaseEvent(QMouseEvent *event)
// {
//     qDebug() << "鼠标释放事件";
//     if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
//     } else if (currentMode == DrawingMode::None && draggedHandle) {
//         handleHandleRelease();
//     } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup) {
//         handleGroupRelease();
//     }
//     if (!event->isAccepted()) {
//         QGraphicsView::mouseReleaseEvent(event);
//     }
// }


// void GraphicsToolView::mousePressEvent(QMouseEvent *event,int a )
// {
//     qDebug() << "鼠标按下事件，按钮：" << event->button();
//     bool isCtrlPressed = event->modifiers() & Qt::ControlModifier;
//     if (currentMode == DrawingMode::None) {
//         qDebug() << "处理无模式按下事件";
//         handleNoneModePress(event);
//     } else if (currentMode == DrawingMode::Line) {
//         qDebug() << "处理直线模式按下事件";
//         handleLineModePress(event);
//     } else if (currentMode == DrawingMode::Polyline) {
//         if (event->button() == Qt::LeftButton) {
//             qDebug() << "处理折线模式按下事件（左键）";
//             handlePolylineModePress(event);
//         }
//     }
//     QGraphicsView::mousePressEvent(event);
// }

void GraphicsToolView::mouseDoubleClickEvent(QMouseEvent *event)
{
    qDebug() << "双击事件，按钮：" << event->button() << "模式：" << static_cast<int>(currentMode) << "是否绘制折线：" << isDrawingPolyline;
    if (event->button() == Qt::LeftButton && currentMode == DrawingMode::Polyline && isDrawingPolyline) {
        qDebug() << "折线双击：结束折线绘制";
        finishPolyline();
        event->accept();
    } else if (event->button() == Qt::LeftButton && currentMode == DrawingMode::Polygon && isDrawingPolyline) {
        qDebug() << "多边形双击：结束多边形绘制";
        finishPolygon();
        event->accept();
    } else {
        qDebug() << "非折线或多边形结束双击，传递给基类";
        QGraphicsView::mouseDoubleClickEvent(event);
    }
}


// void GraphicsToolView::mouseMoveEvent(QMouseEvent *event)
// {
//     QPointF scenePos = mapToScene(event->pos());
//     updateCursorBasedOnPosition(scenePos);
//     if (currentMode == DrawingMode::Line && !startPoint.isNull() && isDragging) {
//         handleLineModeMove(event);
//     } else if (currentMode == DrawingMode::Polyline && isDrawingPolyline && isDragging) {
//         handlePolylineModeMove(event);
//     } else if (currentMode == DrawingMode::None && draggedHandle && draggedItem) {
//         handleHandleMove(event);
//     } else if (currentMode == DrawingMode::None && isDraggingSelectionGroup && !selectedItems.isEmpty()) {
//         handleGroupMove(event);
//     }
// }

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
        pen.setStyle(Qt::SolidLine);
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
        pen.setStyle(Qt::SolidLine);
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
    // cleanupSelection();
    qDebug()<<"selectedItems.size "<< selectedItems.size();
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
        scene()->addItem(previewLine);
        previewLine = nullptr;
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



// 处理椭圆模式下的鼠标按下事件
void GraphicsToolView::handleEllipseModePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        ellipseStartPoint = mapToScene(event->pos()); // 记录椭圆绘制的起始点
        isDragging = true; // 开始拖动绘制椭圆

        // 初始化预览椭圆
        if (previewEllipse) {
            scene()->removeItem(previewEllipse);
            delete previewEllipse;
            previewEllipse = nullptr;
        }
        // 创建一个大小为0的预览椭圆
        previewEllipse = new QGraphicsEllipseItem(QRectF(ellipseStartPoint, ellipseStartPoint));
        QPen pen(drawingColor, 1, Qt::DashLine); // 使用虚线预览
        previewEllipse->setPen(pen);
        scene()->addItem(previewEllipse);
        qDebug() << "Ellipse drawing started at:" << ellipseStartPoint;
    }
}

// 处理椭圆模式下的鼠标移动事件
void GraphicsToolView::handleEllipseModeMove(QMouseEvent *event)
{
    if (isDragging && previewEllipse) {
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF newRect = QRectF(ellipseStartPoint, currentScenePos).normalized(); // 椭圆也由矩形边界定义
        previewEllipse->setRect(newRect);
        qDebug() << "Ellipse preview updated to rect:" << newRect;
    }
}

// 处理椭圆模式下的鼠标释放事件
void GraphicsToolView::handleEllipseModeRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF finalRect = QRectF(ellipseStartPoint, currentScenePos).normalized();

        // 移除预览椭圆
        if (previewEllipse) {
            scene()->removeItem(previewEllipse);
            delete previewEllipse;
            previewEllipse = nullptr;
        }

        // 创建最终的椭圆项
        if (finalRect.width() > 0 && finalRect.height() > 0) { // 确保椭圆有效
            QGraphicsEllipseItem *ellipseItem = new QGraphicsEllipseItem(finalRect);
            QPen pen(drawingColor, 2, Qt::SolidLine); // 最终椭圆用实线
            ellipseItem->setPen(pen);
            // ellipseItem->setBrush(drawingColor); // 如果需要填充颜色
            scene()->addItem(ellipseItem);
            qDebug() << "Ellipse drawn with bounding rect:" << finalRect << "with color" << drawingColor;
        } else {
            qDebug() << "Ellipse too small, not drawn.";
        }

        isDragging = false;
        ellipseStartPoint = QPointF(); // 重置起始点
        // setDrawingMode(DrawingMode::None); // 绘制完毕后可以切换回None模式
    }
}


// 处理圆弧模式下的鼠标按下事件
void GraphicsToolView::handleArcModePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    QPointF scenePos = mapToScene(event->pos());

    if (currentArcState == ArcDrawingState::DefineCenter) {
        arcCenterPoint = scenePos;
        currentArcState = ArcDrawingState::DefineRadiusStart;
        qDebug() << "Arc center set at:" << arcCenterPoint;

        // 初始化预览圆弧 (一个点，或者暂时不显示)
        if (previewArc) {
            scene()->removeItem(previewArc);
            delete previewArc;
        }
        previewArc = new QGraphicsPathItem();
        QPen pen(drawingColor, 1, Qt::DashLine);
        previewArc->setPen(pen);
        scene()->addItem(previewArc);

    } else if (currentArcState == ArcDrawingState::DefineRadiusStart) {
        arcRadiusStartPoint = scenePos;
        currentArcState = ArcDrawingState::DefineEnd;
        qDebug() << "Arc radius/start point set at:" << arcRadiusStartPoint;
        // 此时可以开始绘制预览圆弧的起始部分，或等待第三点
    } else if (currentArcState == ArcDrawingState::DefineEnd) {
        // 第三次点击完成绘制
        QPointF arcEndPoint = scenePos;
        qreal radius = QLineF(arcCenterPoint, arcRadiusStartPoint).length();
        if (radius <= 0) {
            cleanupDrawing();
            return;
        }

        qreal startAngleDegrees = QLineF(arcCenterPoint, arcRadiusStartPoint).angle(); // Qt角度定义
        qreal endAngleDegrees = QLineF(arcCenterPoint, arcEndPoint).angle();

        qreal spanAngleDegrees = endAngleDegrees - startAngleDegrees;
        // 确保 sweepLength 为正，并调整 startAngle (如果需要逆时针)
        // Qt 的 QPainterPath::arcTo sweepLength 为正数时通常是逆时针
        // 如果需要顺时针，可能需要调整或者使用负的 sweepLength (但通常用正值)
        // 我们这里让 startAngle 总是较小的那个，spanAngle 为正
        if (spanAngleDegrees < 0) {
            spanAngleDegrees += 360.0;
        }
        if (spanAngleDegrees > 360.0) { // 如果跨越超过360度，一般限制为360
            spanAngleDegrees = 360.0;
        }


        // 移除预览
        if (previewArc) {
            scene()->removeItem(previewArc);
            delete previewArc;
            previewArc = nullptr;
        }

        // 创建最终的圆弧项
        QPainterPath finalPath;
        QRectF boundingRect(arcCenterPoint.x() - radius, arcCenterPoint.y() - radius, 2 * radius, 2 * radius);
        // QPainterPath::arcTo 需要角度单位为1/16度，所以乘以16
        finalPath.arcTo(boundingRect, startAngleDegrees, spanAngleDegrees);

        QGraphicsPathItem *arcItem = new QGraphicsPathItem(finalPath);
        QPen pen(drawingColor, 2, Qt::SolidLine);
        arcItem->setPen(pen);
        scene()->addItem(arcItem);

        qDebug() << "Arc drawn: Center" << arcCenterPoint << "Radius" << radius
                 << "StartAngle" << startAngleDegrees << "SpanAngle" << spanAngleDegrees;

        cleanupDrawing(); // 重置状态
    }
}

// 处理圆弧模式下的鼠标移动事件
void GraphicsToolView::handleArcModeMove(QMouseEvent *event)
{
    if (!isDragging && currentArcState == ArcDrawingState::DefineCenter) return; // 还没开始画
    if (!previewArc) return;

    QPointF currentScenePos = mapToScene(event->pos());
    QPainterPath previewPath;

    if (currentArcState == ArcDrawingState::DefineRadiusStart) {
        // 正在定义半径和起始点，可以画一条从圆心到鼠标的直线作为预览
        previewPath.moveTo(arcCenterPoint);
        previewPath.lineTo(currentScenePos);
        previewArc->setPath(previewPath);
        qDebug() << "Arc preview: Radius line to" << currentScenePos;
    } else if (currentArcState == ArcDrawingState::DefineEnd) {
        // 正在定义结束点/角度
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
        qDebug() << "Arc preview: End angle at" << currentScenePos << "Span" << spanAngleDegrees;
    }
}

// 圆弧模式下的鼠标释放事件 (如果你的逻辑是点击完成，那么这个函数可能不需要特别处理)
// 但如果第二次点击后是拖拽确定第三点，那么释放时完成。
// 目前我们是三次点击完成。
void GraphicsToolView::handleArcModeRelease(QMouseEvent *event)
{
    // 对于三点确定圆弧，主要逻辑在 mousePressEvent 中
    // 如果是拖拽确定结束角度，则在这里完成
    if (currentArcState == ArcDrawingState::DefineEnd && isDragging) { // 假设 isDragging 代表正在确定第三点
        // 与 handleArcModePress 中第三次点击的逻辑类似
        // 为了避免代码重复，可以将最终绘制逻辑封装成一个函数
        // 这里简单示意，实际应该复用
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
        QPen pen(drawingColor, 2, Qt::SolidLine);
        arcItem->setPen(pen);
        scene()->addItem(arcItem);
        qDebug() << "Arc drawn (on release): Center" << arcCenterPoint << "Radius" << radius
                 << "StartAngle" << startAngleDegrees << "SpanAngle" << spanAngleDegrees;

        cleanupDrawing();
        isDragging = false; // 确保重置拖动状态
    }
}

// 处理多边形模式下的鼠标按下事件
void GraphicsToolView::handlePolygonModePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    QPointF scenePos = mapToScene(event->pos());

    if (!isDrawingPolyline) { // 复用 isDrawingPolyline 状态
        polylinePoints.clear(); // 复用 polylinePoints
        polylinePoints.append(scenePos);
        isDrawingPolyline = true; // 标记开始绘制
        isDragging = true; // 代表鼠标按下状态

        if (previewPolygon) { // 清理旧的预览
            scene()->removeItem(previewPolygon);
            delete previewPolygon;
            previewPolygon = nullptr;
        }
        // 创建预览多边形，初始只有一个点，或者可以暂时不创建预览，在move中创建
        previewPolygon = new QGraphicsPolygonItem();
        QPen pen(drawingColor, 1, Qt::DashLine);
        previewPolygon->setPen(pen);
        scene()->addItem(previewPolygon);

        qDebug() << "Polygon drawing started at:" << scenePos;
    } else {
        polylinePoints.append(scenePos);
        qDebug() << "Polygon point added at:" << scenePos;
    }

    // 更新预览 (至少需要两个点才能形成有效的预览多边形)
    if (polylinePoints.size() >= 1 && previewPolygon) { // 至少一个点就可以开始画预览线到鼠标
        QPolygonF currentQPolygon(polylinePoints);
        if (isDrawingPolyline && polylinePoints.size() > 0) { // 动态预览线
            QPointF tempEndPoint = mapToScene(mapFromGlobal(QCursor::pos())); // 获取当前鼠标位置
            QPolygonF previewQPolygon = currentQPolygon;
            previewQPolygon.append(tempEndPoint); // 添加当前鼠标位置作为临时终点
            if (polylinePoints.size() >=2) { // 如果点数多于2个，预览时可以尝试闭合到起点
                previewQPolygon.append(polylinePoints.first());
            }
            previewPolygon->setPolygon(previewQPolygon);
        }
    }
}

// 处理多边形模式下的鼠标移动事件
void GraphicsToolView::handlePolygonModeMove(QMouseEvent *event)
{
    if (!isDrawingPolyline || polylinePoints.isEmpty() || !previewPolygon) return;

    QPointF currentScenePos = mapToScene(event->pos());
    QPolygonF currentQPolygon(polylinePoints);
    currentQPolygon.append(currentScenePos); // 添加当前鼠标位置用于预览下一段

    // 为了预览闭合效果，可以临时连接到起点
    if (polylinePoints.size() >= 2) { // 至少有2个已确定的点，才能预览闭合
        currentQPolygon.append(polylinePoints.first());
    }
    previewPolygon->setPolygon(currentQPolygon);
    qDebug() << "Polygon preview updated with mouse at:" << currentScenePos;
}

// 结束多边形绘制
void GraphicsToolView::finishPolygon()
{
    if (isDrawingPolyline) {
        // 移除预览多边形
        if (previewPolygon) {
            scene()->removeItem(previewPolygon);
            delete previewPolygon;
            previewPolygon = nullptr;
        }

        if (polylinePoints.size() >= 3) { // 多边形至少需要3个顶点
            QPolygonF finalPolygon(polylinePoints);
            // QGraphicsPolygonItem 默认就是闭合的，它会自动连接最后一个点和第一个点
            QGraphicsPolygonItem *polygonItem = new QGraphicsPolygonItem(finalPolygon);
            QPen pen(drawingColor, 2, Qt::SolidLine);
            polygonItem->setPen(pen);
            QBrush brush(drawingColor, Qt::SolidPattern); // 可以设置填充
            brush.setColor(QColor(drawingColor.red(), drawingColor.green(), drawingColor.blue(), 100)); // 半透明填充
            polygonItem->setBrush(brush);
            scene()->addItem(polygonItem);
            qDebug() << "Polygon drawn with" << polylinePoints.size() << "vertices.";
        } else {
            qDebug() << "Polygon has less than 3 vertices, discarding.";
        }

        isDrawingPolyline = false; // 重置状态
        polylinePoints.clear();
        isDragging = false;
        // setDrawingMode(DrawingMode::None); // 绘制完毕后可以切换回None模式
    }
}


// 处理文本模式下的鼠标按下事件
void GraphicsToolView::handleTextModePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos()); // 获取点击的场景位置

        QGraphicsTextItem *textItem = new QGraphicsTextItem();
        textItem->setPlainText(QStringLiteral("测试文字")); // 设置默认文字
        textItem->setPos(scenePos); // 设置文本框位置
        textItem->setDefaultTextColor(drawingColor); // 使用当前绘图颜色

        // 设置字体 (可选)
        QFont font = textItem->font();
        font.setPointSize(12); // 设置字号
        textItem->setFont(font);

        // 使文本项可移动和可选择 (可选)
        textItem->setFlag(QGraphicsItem::ItemIsMovable);
        textItem->setFlag(QGraphicsItem::ItemIsSelectable);
        // 如果需要创建后即可编辑文本，可以设置：
        // textItem->setTextInteractionFlags(Qt::TextEditorInteraction);


        scene()->addItem(textItem);
        qDebug() << "Text item added at:" << scenePos << "with text: '测试文字'";

        // 绘制完一个文本框后，可以考虑切换回None模式
        // setDrawingMode(DrawingMode::None);
    }
}




// 处理矩形模式下的鼠标按下事件
void GraphicsToolView::handleRectangleModePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        rectStartPoint = mapToScene(event->pos()); // 记录矩形绘制的起始点
        isDragging = true; // 开始拖动绘制矩形

        // 初始化预览矩形
        if (previewRect) {
            scene()->removeItem(previewRect);
            delete previewRect;
            previewRect = nullptr;
        }
        // 创建一个大小为0的预览矩形，颜色稍后在move中设置
        previewRect = new QGraphicsRectItem(QRectF(rectStartPoint, rectStartPoint));
        QPen pen(drawingColor, 1, Qt::DashLine); // 使用虚线预览
        previewRect->setPen(pen);
        // previewRect->setBrush(Qt::transparent); // 可以设置透明画刷或不设置
        scene()->addItem(previewRect);
        qDebug() << "Rectangle drawing started at:" << rectStartPoint;
    }
}

// 处理矩形模式下的鼠标移动事件
void GraphicsToolView::handleRectangleModeMove(QMouseEvent *event)
{
    if (isDragging && previewRect) {
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF newRect = QRectF(rectStartPoint, currentScenePos).normalized(); // 标准化矩形，确保左上角和右下角正确
        previewRect->setRect(newRect);
        qDebug() << "Rectangle preview updated to:" << newRect;
    }
}

// 处理矩形模式下的鼠标释放事件
void GraphicsToolView::handleRectangleModeRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF finalRect = QRectF(rectStartPoint, currentScenePos).normalized();

        // 移除预览矩形
        if (previewRect) {
            scene()->removeItem(previewRect);
            delete previewRect;
            previewRect = nullptr;
        }

        // 创建最终的矩形项
        if (finalRect.width() > 0 && finalRect.height() > 0) { // 确保矩形有效
            QGraphicsRectItem *rectItem = new QGraphicsRectItem(finalRect);
            QPen pen(drawingColor, 2, Qt::SolidLine); // 最终矩形用实线，颜色和粗细可调
            rectItem->setPen(pen);
            // rectItem->setBrush(drawingColor); // 如果需要填充颜色
            scene()->addItem(rectItem);
            qDebug() << "Rectangle drawn:" << finalRect << "with color" << drawingColor;
        } else {
            qDebug() << "Rectangle too small, not drawn.";
        }

        isDragging = false;
        rectStartPoint = QPointF(); // 重置起始点
        // setDrawingMode(DrawingMode::None); // 绘制完毕后可以切换回None模式
    }
}
