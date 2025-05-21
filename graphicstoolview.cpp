#include "graphicstoolview.h"
#include "editablelineitem.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QCursor> // 包含光标头文件
#include <QGraphicsItem>
#include <QPainterPath>

GraphicsToolView::GraphicsToolView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent),
    isDraggingSelectionGroup(false), // 初始化拖动选中组状态
    isDragging(false), // 初始化拖动状态
    previewLine(nullptr), // 初始化预览线
    draggedHandle(nullptr), // 初始化拖动端点
    draggedItem(nullptr), // 初始化拖动项
    isShiftPressed(false), // 初始化Shift键状态
    isCtrlPressedForCopy(false), // 初始化Ctrl键复制状态
    drawingColor(Qt::black), // 默认绘图颜色为黑色
    isDrawingPolyline(false), // 初始化折线绘制状态
    draggedHandleIndex(-1), // 初始化拖动端点索引
    closePolylineOnFinish(false), // 初始化折线闭合状态
    previewClosingSegment(nullptr) // 初始化闭合预览线段
{
    setRenderHint(QPainter::Antialiasing); // 设置抗锯齿渲染
}

// 设置绘图颜色
void GraphicsToolView::setDrawingColor(const QColor &color)
{
    drawingColor = color;
    qDebug() << "绘图颜色设置为：" << color.name(); // 打印颜色设置信息
}

// 获取当前绘图颜色
QColor GraphicsToolView::currentDrawingColor() const
{
    return drawingColor; // 返回当前颜色
}

// 显示颜色选择器
void GraphicsToolView::showColorSelector()
{
    if (colorSelector) {
        QPoint viewCenter = viewport()->rect().center(); // 获取视图中心
        QPoint globalPos = mapToGlobal(viewCenter); // 映射到全局坐标
        colorSelector->move(globalPos); // 移动选择器位置
        colorSelector->show(); // 显示选择器
    }
}

// 处理颜色选择信号
void GraphicsToolView::onColorSelected(const QColor &color)
{
    setDrawingColor(color); // 设置选择的颜色
    qDebug() << "选择的颜色：" << color.name(); // 打印选择颜色信息
}

void GraphicsToolView::setDrawingMode(DrawingMode mode)
{
    qDebug() << "设置绘图模式为：" << static_cast<int>(mode); // 打印模式设置信息
    if (currentMode == DrawingMode::Polyline && isDrawingPolyline) {
        qDebug() << "切换模式时正在绘制折线，结束当前折线。"; // 打印折线结束信息
        finishPolyline(); // 结束折线绘制
    }
    currentMode = mode; // 设置当前模式
    cleanupDrawing(); // 清理绘制状态
    cleanupSelection(); // 清理选中状态
    switch (mode) {
    case DrawingMode::None: // 无模式
        setCursor(Qt::ArrowCursor); // 设置箭头光标
        setMouseTracking(false); // 关闭鼠标追踪
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
    }
    QGraphicsView::mousePressEvent(event); // 调用基类处理
}

void GraphicsToolView::mouseDoubleClickEvent(QMouseEvent *event)
{
    qDebug() << "双击事件，按钮：" << event->button() << "模式：" << static_cast<int>(currentMode) << "是否绘制折线：" << isDrawingPolyline; // 打印双击信息
    if (event->button() == Qt::LeftButton && currentMode == DrawingMode::Polyline && isDrawingPolyline) {
        qDebug() << "折线双击：结束折线绘制"; // 打印折线结束信息
        finishPolyline(); // 结束折线绘制
        event->accept(); // 接受事件
    } else {
        qDebug() << "非折线结束双击，传递给基类"; // 打印非折线双击信息
        QGraphicsView::mouseDoubleClickEvent(event); // 调用基类处理
    }
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
    }
}

void GraphicsToolView::wheelEvent(QWheelEvent *event)
{
    const qreal delta = event->angleDelta().y(); // 获取滚轮增量
    qreal scaleFactor = 1.15; // 缩放因子
    if (delta > 0) {
        scale(scaleFactor, scaleFactor); // 放大
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor); // 缩小
    }
}

void GraphicsToolView::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "按键事件，按键：" << event->key(); // 打印按键信息
    if (event->key() == Qt::Key_Escape) {
        if (currentMode != DrawingMode::None || isDrawingPolyline) {
            qDebug() << "按下Esc，取消绘制"; // 打印取消绘制信息
            cleanupDrawing(); // 清理绘制状态
            setDrawingMode(DrawingMode::None); // 设置为无模式
        } else if (!selectedItems.isEmpty()) {
            qDebug() << "按下Esc，清除选中"; // 打印清除选中信息
            cleanupSelection(); // 清理选中状态
        }
        event->accept(); // 接受事件
        return;
    }
    if (event->key() == Qt::Key_Shift) {
        if (currentMode == DrawingMode::Line) {
            isShiftPressed = true; // 设置Shift键按下状态
            qDebug() << "按下Shift，限制直线方向"; // 打印Shift限制信息
        }
    } else if (event->key() == Qt::Key_C) {
        if (currentMode == DrawingMode::Polyline && isDrawingPolyline) {
            qDebug() << "按下C，尝试闭合并完成折线"; // 打印闭合折线信息
            if (polylinePoints.size() >= 2) {
                closePolylineOnFinish = true; // 设置闭合标志
                finishPolyline(); // 完成折线绘制
            } else {
                qDebug() << "点数不足 (<2)，无法闭合折线。将作为开放折线结束（如果可能）或丢弃。"; // 打印点数不足信息
                closePolylineOnFinish = false; // 不闭合
                finishPolyline(); // 结束绘制
            }
            event->accept(); // 接受事件
            return;
        }
    }
    if (!event->isAccepted()) {
        QGraphicsView::keyPressEvent(event); // 调用基类处理
    }
}

void GraphicsToolView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift) {
        isShiftPressed = false; // 释放Shift键
        qDebug() << "释放Shift，不限制直线方向"; // 打印释放Shift信息
        if (currentMode == DrawingMode::Polyline && previewLine && !polylinePoints.isEmpty()) {
            updatePolylinePreview(previewLine->line().p2()); // 更新预览线
        }
    }
    QGraphicsView::keyReleaseEvent(event); // 调用基类处理
}

void GraphicsToolView::finishPolyline()
{
    qDebug() << "结束折线绘制，是否绘制中：" << isDrawingPolyline << "点数：" << polylinePoints.size(); // 打印结束折线信息
    if (isDrawingPolyline) {
        if (currentPolyline && polylinePoints.size() >= 2) { // 至少需要2个点
            qDebug() << "折线绘制完成，点数：" << polylinePoints.size(); // 打印完成信息
            if (closePolylineOnFinish && polylinePoints.size() >= 3) { // 闭合需至少3个点
                currentPolyline->setClosed(true); // 设置闭合
                qDebug() << "折线设置为闭合"; // 打印闭合信息
            } else {
                currentPolyline->setClosed(false); // 设置不闭合
                qDebug() << "折线设置为开放 (或点数不足以闭合)"; // 打印不闭合信息
            }
            currentPolyline = nullptr; // 重置指针，对象由场景管理
        } else if (currentPolyline) { // 点数不足
            qDebug() << "折线点数不足，丢弃图形"; // 打印丢弃信息
            scene()->removeItem(currentPolyline); // 移除对象
            delete currentPolyline; // 删除对象
            currentPolyline = nullptr; // 重置指针
        } else {
            qDebug() << "无折线图形或点数不足"; // 打印无图形信息
        }

        // 清理状态
        polylinePoints.clear(); // 清空折线点
        isDrawingPolyline = false; // 重置绘制状态
        isDragging = false; // 重置拖动状态

        if (previewLine) {
            scene()->removeItem(previewLine); // 移除预览线
            delete previewLine; // 删除预览线
            previewLine = nullptr; // 重置指针
        }
        if (previewClosingSegment) { // 清理闭合预览线
            scene()->removeItem(previewClosingSegment); // 移除闭合预览线
            delete previewClosingSegment; // 删除闭合预览线
            previewClosingSegment = nullptr; // 重置指针
        }
        closePolylineOnFinish = false; // 重置闭合标志

        qDebug() << "折线绘制结束，重置为无模式"; // 打印结束信息
        setDrawingMode(DrawingMode::None); // 返回无模式
    } else {
        qDebug() << "调用结束折线，但不在绘制模式"; // 打印不在绘制模式信息
    }
}

void GraphicsToolView::handlePolylineModePress(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos()); // 映射到场景坐标
    if (isShiftPressed && !polylinePoints.isEmpty()) {
        QPointF lastPoint = polylinePoints.last(); // 获取上一个点
        qreal dx = qAbs(scenePos.x() - lastPoint.x()); // 计算x差值
        qreal dy = qAbs(scenePos.y() - lastPoint.y()); // 计算y差值
        if (dx > dy) {
            scenePos.setY(lastPoint.y()); // 限制y坐标
        } else {
            scenePos.setX(lastPoint.x()); // 限制x坐标
        }
    }
    if (!isDrawingPolyline) {
        polylinePoints.clear(); // 清空折线点
        if (currentPolyline) {
            scene()->removeItem(currentPolyline); // 移除当前折线
            delete currentPolyline; // 删除当前折线
            currentPolyline = nullptr; // 重置指针
        }
        if (previewLine) {
            scene()->removeItem(previewLine); // 移除预览线
            delete previewLine; // 删除预览线
            previewLine = nullptr; // 重置指针
        }
        polylinePoints.append(scenePos); // 添加起点
        isDrawingPolyline = true; // 设置绘制状态
        isDragging = true; // 设置拖动状态
        qDebug() << "开始新折线，位置：" << scenePos; // 打印开始信息
    } else {
        if (polylinePoints.isEmpty() || polylinePoints.last() != scenePos) {
            polylinePoints.append(scenePos); // 添加新点
            qDebug() << "添加折线点，位置：" << scenePos; // 打印添加点信息
        }
        if (polylinePoints.size() >= 2) {
            if (currentPolyline) {
                scene()->removeItem(currentPolyline); // 移除旧折线
                delete currentPolyline; // 删除旧折线
            }
            currentPolyline = new EditablePolylineItem(polylinePoints); // 创建新折线
            QPen pen = currentPolyline->pen(); // 获取画笔
            pen.setColor(drawingColor); // 设置颜色
            currentPolyline->setPen(pen); // 应用画笔
            scene()->addItem(currentPolyline); // 添加到场景
            qDebug() << "更新折线，点数：" << polylinePoints.size(); // 打印更新信息
        }
    }
    if (isDrawingPolyline && !polylinePoints.isEmpty()) {
        if (previewLine) {
            scene()->removeItem(previewLine); // 移除旧预览线
            delete previewLine; // 删除旧预览线
        }
        previewLine = new EditableLineItem(polylinePoints.last(), scenePos); // 创建新预览线
        QPen pen = previewLine->pen(); // 获取画笔
        pen.setColor(drawingColor); // 设置颜色
        pen.setStyle(Qt::SolidLine); // 设置样式
        previewLine->setPen(pen); // 应用画笔
        scene()->addItem(previewLine); // 添加到场景
    }
}

void GraphicsToolView::handlePolylineModeMove(QMouseEvent *event)
{
    if (!isDrawingPolyline || polylinePoints.isEmpty() || !isDragging) {
        return; // 未在绘制或无点时返回
    }
    QPointF scenePos = mapToScene(event->pos()); // 映射到场景坐标
    updatePolylinePreview(scenePos); // 更新预览线
}

// 更新折线预览线
void GraphicsToolView::updatePolylinePreview(const QPointF& currentMousePos)
{
    if (!isDrawingPolyline || polylinePoints.isEmpty()) {
        return; // 未在绘制或无点时返回
    }

    QPointF lastPoint = polylinePoints.last(); // 获取上一个点
    QPointF effectiveMousePos = currentMousePos; // 有效鼠标位置

    if (isShiftPressed) {
        qreal dx = qAbs(effectiveMousePos.x() - lastPoint.x()); // 计算x差值
        qreal dy = qAbs(effectiveMousePos.y() - lastPoint.y()); // 计算y差值
        if (dx > dy) {
            effectiveMousePos.setY(lastPoint.y()); // 限制y坐标
        } else {
            effectiveMousePos.setX(lastPoint.x()); // 限制x坐标
        }
    }

    // 更新主预览线
    if (previewLine) {
        previewLine->setLine(lastPoint.x(), lastPoint.y(), effectiveMousePos.x(), effectiveMousePos.y()); // 更新线段
    } else {
        previewLine = new EditableLineItem(lastPoint, effectiveMousePos); // 创建新预览线
        QPen pen = previewLine->pen(); // 获取画笔
        pen.setColor(drawingColor); // 设置颜色
        pen.setStyle(Qt::SolidLine); // 设置样式
        previewLine->setPen(pen); // 应用画笔
        scene()->addItem(previewLine); // 添加到场景
    }
    previewLine->setVisible(true); // 设置可见

    // 更新闭合预览线段
    if (closePolylineOnFinish && polylinePoints.size() >= 1) {
        if (!previewClosingSegment) {
            previewClosingSegment = new EditableLineItem(effectiveMousePos, polylinePoints.first()); // 创建闭合预览线
            QPen pen = previewClosingSegment->pen(); // 获取画笔
            pen.setColor(drawingColor); // 设置颜色
            pen.setStyle(Qt::DotLine); // 设置点线样式
            previewClosingSegment->setPen(pen); // 应用画笔
            scene()->addItem(previewClosingSegment); // 添加到场景
        }
        previewClosingSegment->setLine(effectiveMousePos.x(), effectiveMousePos.y(), polylinePoints.first().x(), polylinePoints.first().y()); // 更新线段
        previewClosingSegment->setVisible(true); // 设置可见
    } else {
        if (previewClosingSegment) {
            previewClosingSegment->setVisible(false); // 隐藏闭合预览线
        }
    }
}

void GraphicsToolView::handleNoneModePress(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos()); // 映射到场景坐标
    QGraphicsItem *item = scene()->itemAt(scenePos, transform()); // 获取点击项
    qreal tolerance = 10.0; // 容差，扩展选中区域
    if (checkHandleHit(scenePos)) {
        return; // 端点点击直接返回
    }
    if (checkSelectedGroupHit(scenePos, tolerance)) {
        this->isCtrlPressedForCopy = event->modifiers() & Qt::ControlModifier; // 检查Ctrl键状态
        qDebug() << "复制时Ctrl键状态：" << isCtrlPressedForCopy; // 打印Ctrl状态
        return;
    }
    handleItemSelection(scenePos, tolerance, event->modifiers() & Qt::ControlModifier); // 处理项选择
}

bool GraphicsToolView::checkHandleHit(const QPointF &scenePos)
{
    for (QGraphicsItem* item : selectedItems) {
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            HandleItem* startHandle = editableLine->getStartHandle(); // 获取起点端点
            HandleItem* endHandle = editableLine->getEndHandle(); // 获取终点端点
            HandleItem* rotationHandle = editableLine->getRotationHandle(); // 获取旋转端点
            if (startHandle && startHandle->isVisible() && startHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = startHandle; // 设置拖动端点
                draggedItem = item; // 设置拖动项
                qDebug() << "选中起点端点，位置：" << scenePos; // 打印选中信息
                return true;
            } else if (endHandle && endHandle->isVisible() && endHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = endHandle; // 设置拖动端点
                draggedItem = item; // 设置拖动项
                qDebug() << "选中终点端点，位置：" << scenePos; // 打印选中信息
                return true;
            } else if (rotationHandle && rotationHandle->isVisible() && rotationHandle->sceneBoundingRect().contains(scenePos)) {
                draggedHandle = rotationHandle; // 设置拖动端点
                draggedItem = item; // 设置拖动项
                fixedRotationCenter = editableLine->mapToScene(editableLine->getRotationHandle()->pos()); // 固定旋转中心
                qDebug() << "旋转中心固定，位置：" << fixedRotationCenter; // 打印旋转中心信息
                qDebug() << "选中旋转端点，位置：" << scenePos; // 打印选中信息
                return true;
            }
        } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
            QVector<HandleItem*> handles = editablePolyline->getHandles(); // 获取折线端点
            for (int i = 0; i < handles.size(); ++i) {
                HandleItem* handle = handles[i];
                if (handle && handle->isVisible() && handle->sceneBoundingRect().contains(scenePos)) {
                    draggedHandle = handle; // 设置拖动端点
                    draggedItem = item; // 设置拖动项
                    draggedHandleIndex = i; // 设置端点索引
                    qDebug() << "选中折线端点，索引：" << i << "位置：" << scenePos; // 打印选中信息
                    return true;
                }
            }
        }
    }
    return false; // 未点击端点
}

bool GraphicsToolView::checkSelectedGroupHit(const QPointF &scenePos, qreal tolerance)
{
    for (QGraphicsItem* item : selectedItems) {
        QRectF boundingRect = item->boundingRect(); // 获取边界矩形
        boundingRect.adjust(-tolerance, -tolerance, tolerance, tolerance); // 调整容差
        QRectF sceneRect = item->mapToScene(boundingRect).boundingRect(); // 映射到场景坐标
        if (sceneRect.contains(scenePos)) {
            isDraggingSelectionGroup = true; // 设置拖动选中组状态
            dragStartPosition = scenePos; // 设置拖动起始位置
            lastDragPos = scenePos; // 设置上次拖动位置
            qDebug() << "开始拖动选中组，位置：" << scenePos; // 打印开始拖动信息
            return true;
        }
    }
    return false; // 未点击选中组
}

void GraphicsToolView::handleItemSelection(const QPointF &scenePos, qreal tolerance, bool isMultiSelect)
{
    QList<QGraphicsItem*> items = scene()->items(); // 获取场景所有项
    bool itemSelected = false; // 是否选中项标志
    for (QGraphicsItem* item : items) {
        QRectF boundingRect = item->boundingRect(); // 获取边界矩形
        boundingRect.adjust(-tolerance, -tolerance, tolerance, tolerance); // 调整容差
        QRectF sceneRect = item->mapToScene(boundingRect).boundingRect(); // 映射到场景坐标
        if (sceneRect.contains(scenePos)) {
            if (isMultiSelect) {
                if (!selectedItems.contains(item)) {
                    selectedItems.append(item); // 添加到选中列表
                    if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                        editableLine->setSelectedState(true); // 设置选中状态
                    } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                        editablePolyline->setSelectedState(true); // 设置选中状态
                    }
                    qDebug() << "添加图形到选中（多选），位置：" << scenePos; // 打印多选信息
                    qDebug() << "选中图形总数：" << selectedItems.size(); // 打印选中总数
                }
            } else {
                cleanupSelection(); // 清理现有选中
                selectedItems.append(item); // 添加新选中项
                if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                    editableLine->setSelectedState(true); // 设置选中状态
                } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
                    editablePolyline->setSelectedState(true); // 设置选中状态
                }
                qDebug() << "选中图形（单选），位置：" << scenePos; // 打印单选信息
            }
            itemSelected = true; // 标记已选中
            break;
        }
    }
    if (!itemSelected) {
        cleanupSelection(); // 未选中则清理状态
        qDebug() << "点击空白区域，清除选中"; // 打印清除选中信息
    } else {
        qDebug() << "选中图形总数：" << selectedItems.size(); // 打印选中总数
    }
}

void GraphicsToolView::handleLineModePress(QMouseEvent *event)
{
    qDebug() << "selectedItems.size " << selectedItems.size(); // 打印选中项数量
    if (event->button() != Qt::LeftButton) return; // 仅处理左键
    QPointF scenePos = mapToScene(event->pos()); // 映射到场景坐标
    if (isShiftPressed && !startPoint.isNull()) {
        qreal dx = qAbs(scenePos.x() - startPoint.x()); // 计算x差值
        qreal dy = qAbs(scenePos.y() - startPoint.y()); // 计算y差值
        if (dx > dy) scenePos.setY(startPoint.y()); // 限制y坐标
        else scenePos.setX(startPoint.x()); // 限制x坐标
    }
    if (startPoint.isNull()) {
        startPoint = scenePos; // 设置起点
        isDragging = true; // 设置拖动状态
        qDebug() << "直线模式：起点设置为：" << startPoint; // 打印起点信息
        if (previewLine) {
            scene()->removeItem(previewLine); // 移除旧预览线
            delete previewLine; // 删除旧预览线
            previewLine = nullptr; // 重置指针
        }
        previewLine = new EditableLineItem(startPoint, startPoint); // 创建新预览线
        QPen pen = previewLine->pen(); // 获取画笔
        pen.setColor(drawingColor); // 设置颜色
        pen.setStyle(Qt::SolidLine); // 设置样式
        previewLine->setPen(pen); // 应用画笔
        previewLine->setSelected(false); // 取消选中状态
        previewLine->setFlag(QGraphicsItem::ItemIsSelectable, false); // 不可选择
        scene()->addItem(previewLine); // 添加到场景
    } else {
        endPoint = scenePos; // 设置终点
        qDebug() << "直线模式：终点设置为：" << endPoint; // 打印终点信息
        EditableLineItem *line = new EditableLineItem(startPoint, endPoint); // 创建直线
        QPen pen = line->pen(); // 获取画笔
        pen.setColor(drawingColor); // 设置颜色
        line->setPen(pen); // 应用画笔
        line->setSelected(false); // 取消选中状态
        scene()->addItem(previewLine); // 添加预览线（可能错误，应添加line）
        previewLine = nullptr; // 重置预览线
        qDebug() << "创建直线，从" << startPoint << "到" << endPoint; // 打印创建信息
        cleanupDrawing(); // 清理绘制状态
        setDrawingMode(DrawingMode::None); // 设置为无模式
        selectedItems.clear(); // 清空选中项
    }
}

void GraphicsToolView::updateCursorBasedOnPosition(const QPointF &scenePos)
{
    bool isOverSelectedItem = false; // 是否在选中项上方标志
    qreal tolerance = 10.0; // 容差，扩展选中区域
    for (QGraphicsItem* item : selectedItems) {
        QRectF boundingRect = item->boundingRect(); // 获取边界矩形
        boundingRect.adjust(-tolerance, -tolerance, tolerance, tolerance); // 调整容差
        QRectF sceneRect = item->mapToScene(boundingRect).boundingRect(); // 映射到场景坐标
        if (sceneRect.contains(scenePos)) {
            isOverSelectedItem = true; // 标记在选中项上方
            break;
        }
    }
    if (isOverSelectedItem && currentMode == DrawingMode::None) {
        setCursor(Qt::SizeAllCursor); // 设置移动光标
    } else if (currentMode == DrawingMode::None) {
        setCursor(Qt::ArrowCursor); // 设置箭头光标
    }
}

void GraphicsToolView::handleLineModeMove(QMouseEvent *event)
{
    if (startPoint.isNull() || !isDragging || !previewLine) return; // 未开始拖动或无预览线则返回
    QPointF scenePos = mapToScene(event->pos()); // 映射到场景坐标
    if (isShiftPressed) {
        qreal dx = qAbs(scenePos.x() - startPoint.x()); // 计算x差值
        qreal dy = qAbs(scenePos.y() - startPoint.y()); // 计算y差值
        if (dx > dy) scenePos.setY(startPoint.y()); // 限制y坐标
        else scenePos.setX(startPoint.x()); // 限制x坐标
    }
    previewLine->setLine(QLineF(startPoint, scenePos)); // 更新预览线
}

void GraphicsToolView::applyColorToSelectedItems(const QColor &color)
{
    for (QGraphicsItem* item : selectedItems) {
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            QPen pen = editableLine->pen(); // 获取画笔
            pen.setColor(color); // 设置颜色
            editableLine->setPen(pen); // 应用画笔
        } else if (EditablePolylineItem* editablePolyline = dynamic_cast<EditablePolylineItem*>(item)) {
            QPen pen = editablePolyline->pen(); // 获取画笔
            pen.setColor(color); // 设置颜色
            editablePolyline->setPen(pen); // 应用画笔
        }
    }
}

void GraphicsToolView::handleHandleMove(QMouseEvent *event)
{
    if (!draggedItem || !draggedHandle) {
        qWarning() << "无效的拖动图形或端点"; // 打印无效警告
        return;
    }
    QPointF newPos = mapToScene(event->pos()); // 映射到场景坐标
    qDebug() << "鼠标事件位置（视图坐标）：" << event->pos(); // 打印鼠标位置
    qDebug() << "映射到场景位置：" << newPos; // 打印场景位置
    if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(draggedItem)) {
        QLineF currentLine = editableLine->line(); // 获取当前线段
        QPointF sceneP1 = editableLine->mapToScene(currentLine.p1()); // 映射起点
        QPointF sceneP2 = editableLine->mapToScene(currentLine.p2()); // 映射终点
        qDebug() << "当前直线场景坐标：P1=" << sceneP1 << ", P2=" << sceneP2; // 打印线段信息
        if (draggedHandle == editableLine->getStartHandle()) {
            editableLine->updateLine(newPos, sceneP2); // 更新起点
            qDebug() << "拖动起点端点到：" << newPos; // 打印拖动信息
        } else if (draggedHandle == editableLine->getEndHandle()) {
            editableLine->updateLine(sceneP1, newPos); // 更新终点
            qDebug() << "拖动终点端点到：" << newPos; // 打印拖动信息
        } else if (draggedHandle == editableLine->getRotationHandle()) {
            QPointF midPoint = (sceneP1 + sceneP2) / 2.0; // 计算中点
            QPointF currentVector = midPoint - fixedRotationCenter; // 当前向量
            qreal currentAngle = qAtan2(currentVector.y(), currentVector.x()) * 180.0 / M_PI; // 当前角度
            if (currentAngle < 0) currentAngle += 360.0; // 角度校正
            QPointF mouseVector = newPos - fixedRotationCenter; // 鼠标向量
            qreal targetAngle = qAtan2(mouseVector.y(), mouseVector.x()) * 180.0 / M_PI; // 目标角度
            if (targetAngle < 0) targetAngle += 360.0; // 角度校正
            qreal angleDiff = targetAngle - currentAngle; // 角度差
            if (angleDiff > 180) angleDiff -= 360; // 角度差校正
            else if (angleDiff < -180) angleDiff += 360; // 角度差校正
            qreal newRotation = targetAngle; // 新旋转角度
            editableLine->rotate(newRotation, fixedRotationCenter); // 应用旋转
            qDebug() << "拖动旋转端点到：" << newPos << "，旋转中心：" << fixedRotationCenter; // 打印旋转信息
            qDebug() << "当前角度：" << currentAngle << "，目标角度：" << targetAngle << "，角度差：" << angleDiff << "，新旋转角度：" << newRotation; // 打印角度信息
        }
    }
}

void GraphicsToolView::handleGroupMove(QMouseEvent *event)
{
    QPointF currentPos = mapToScene(event->pos()); // 映射到场景坐标
    QPointF offset = currentPos - lastDragPos; // 计算偏移量
    for (QGraphicsItem* item : selectedItems) {
        QPointF newItemPos = item->pos() + offset; // 计算新位置
        item->setPos(newItemPos); // 设置新位置
    }
    lastDragPos = currentPos; // 更新上次拖动位置
    qDebug() << "拖动选中组，偏移量：" << offset; // 打印拖动信息
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
    }
    if (!event->isAccepted()) {
        QGraphicsView::mouseReleaseEvent(event); // 调用基类处理
    }
}

void GraphicsToolView::handleLineModeRelease(QMouseEvent *event)
{
    if (isShiftPressed) {
        qreal dx = qAbs(endPoint.x() - startPoint.x()); // 计算x差值
        qreal dy = qAbs(endPoint.y() - startPoint.y()); // 计算y差值
        if (dx > dy) {
            endPoint.setY(startPoint.y()); // 限制y坐标
        } else {
            endPoint.setX(startPoint.x()); // 限制x坐标
        }
    }
}

void GraphicsToolView::handleHandleRelease()
{
    qDebug() << "端点拖动释放"; // 打印释放信息
    draggedHandle = nullptr; // 重置拖动端点
    draggedItem = nullptr; // 重置拖动项
}

void GraphicsToolView::handleGroupRelease()
{
    qDebug() << "选中组拖动释放"; // 打印释放信息
    qDebug() << "复制时Ctrl键状态：" << isCtrlPressedForCopy; // 打印Ctrl状态
    if (isCtrlPressedForCopy && !selectedItems.isEmpty()) {
        qDebug() << "拖动时按下Ctrl，复制选中图形"; // 打印复制信息
        QList<QGraphicsItem*> newItems; // 新复制项列表
        for (QGraphicsItem* item : selectedItems) {
            if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                QGraphicsItem* newItem = editableLine->clone(); // 克隆项
                if (newItem) {
                    newItem->setPos(item->pos() + QPointF(20, 20)); // 设置偏移位置
                    scene()->addItem(newItem); // 添加到场景
                    newItems.append(newItem); // 添加到新项列表
                    qDebug() << "复制图形，位置：" << newItem->pos(); // 打印复制信息
                }
            }
        }
        cleanupSelection(); // 清理选中状态
        for (QGraphicsItem* newItem : newItems) {
            if (EditableLineItem* newEditableLine = dynamic_cast<EditableLineItem*>(newItem)) {
                newEditableLine->setSelectedState(true); // 设置选中状态
                selectedItems.append(newItem); // 添加到选中列表
            }
        }
        qDebug() << "拖动时复制完成，新选中图形数：" << selectedItems.size(); // 打印完成信息
    } else {
        qDebug() << "未执行复制，Ctrl状态：" << isCtrlPressedForCopy << "，选中图形数：" << selectedItems.size(); // 打印未复制信息
    }
    isDraggingSelectionGroup = false; // 重置拖动选中组状态
    isCtrlPressedForCopy = false; // 重置Ctrl复制状态
}

void GraphicsToolView::cleanupDrawing()
{
    qDebug() << "清理绘制状态，当前模式：" << static_cast<int>(currentMode); // 打印清理信息
    if (previewLine) {
        scene()->removeItem(previewLine); // 移除预览线
        delete previewLine; // 删除预览线
        previewLine = nullptr; // 重置指针
    }
    if (previewClosingSegment) {
        scene()->removeItem(previewClosingSegment); // 移除闭合预览线
        delete previewClosingSegment; // 删除闭合预览线
        previewClosingSegment = nullptr; // 重置指针
    }
    startPoint = QPointF(); // 重置起点
    endPoint = QPointF(); // 重置终点

    if (isDrawingPolyline || !polylinePoints.isEmpty() || currentPolyline) {
        qDebug() << "清理折线绘制残留"; // 打印折线清理信息
        polylinePoints.clear(); // 清空折线点
        if (currentPolyline) {
            scene()->removeItem(currentPolyline); // 移除当前折线
            delete currentPolyline; // 删除当前折线
            currentPolyline = nullptr; // 重置指针
        }
        isDrawingPolyline = false; // 重置折线绘制状态
    }
    closePolylineOnFinish = false; // 重置闭合状态
    isDragging = false; // 重置拖动状态
}

void GraphicsToolView::cleanupSelection()
{
    qDebug() << "清理选中状态"; // 打印清理信息
    for (QGraphicsItem* item : selectedItems) {
        if (item) {
            if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
                editableLine->setSelectedState(false); // 取消选中状态
            }
        }
    }
    selectedItems.clear(); // 清空选中列表
    copiedItems.clear(); // 清空复制列表
    draggedHandle = nullptr; // 重置拖动端点
    draggedItem = nullptr; // 重置拖动项
    isDraggingSelectionGroup = false; // 重置拖动选中组状态
}

void GraphicsToolView::copySelectedItems()
{
    qDebug() << "复制选中图形"; // 打印复制信息
    copiedItems.clear(); // 清空复制列表
    for (QGraphicsItem* item : selectedItems) {
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            if (item->parentItem() == nullptr || dynamic_cast<EditableLineItem*>(item->parentItem()) == nullptr) {
                copiedItems.append(item); // 添加到复制列表
            }
        }
    }
    qDebug() << "复制图形总数：" << copiedItems.size(); // 打印复制总数
}

void GraphicsToolView::pasteCopiedItems()
{
    qDebug() << "粘贴复制的图形"; // 打印粘贴信息
    if (copiedItems.isEmpty()) {
        qDebug() << "无图形可粘贴"; // 打印无图形信息
        return;
    }
    QPointF pasteOffset(20, 20); // 粘贴偏移量
    QGraphicsScene* currentScene = scene(); // 获取当前场景
    if (!currentScene) {
        qDebug() << "错误：视图无关联场景"; // 打印错误信息
        return;
    }
    QList<QGraphicsItem*> newlyPastedItems; // 新粘贴项列表
    for (QGraphicsItem* item : copiedItems) {
        QGraphicsItem* newItem = nullptr; // 新项指针
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(item)) {
            newItem = editableLine->clone(); // 克隆直线
        } else {
            if (QGraphicsRectItem* rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
                newItem = new QGraphicsRectItem(rectItem->rect()); // 创建矩形
                static_cast<QGraphicsRectItem*>(newItem)->setPen(rectItem->pen()); // 设置画笔
                static_cast<QGraphicsRectItem*>(newItem)->setBrush(rectItem->brush()); // 设置画刷
            } else {
                qDebug() << "警告：无法粘贴不支持的图形类型"; // 打印不支持类型警告
            }
        }
        if (newItem) {
            newItem->setPos(item->pos() + pasteOffset); // 设置偏移位置
            currentScene->addItem(newItem); // 添加到场景
            newlyPastedItems.append(newItem); // 添加到新粘贴列表
        }
    }
    cleanupSelection(); // 清理选中状态
    for (QGraphicsItem* pastedItem : newlyPastedItems) {
        if (EditableLineItem* editableLine = dynamic_cast<EditableLineItem*>(pastedItem)) {
            editableLine->setSelectedState(true); // 设置选中状态
            selectedItems.append(editableLine); // 添加到选中列表
            copiedItems.append(editableLine); // 添加到复制列表
        }
    }
    qDebug() << "粘贴图形总数：" << selectedItems.size(); // 打印粘贴总数
}
