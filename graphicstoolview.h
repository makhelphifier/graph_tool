#ifndef GRAPHICSTOOLVIEW_H
#define GRAPHICSTOOLVIEW_H

#include <QGraphicsView>
#include <QWidget>
#include <QWheelEvent>
#include <QGraphicsLineItem>
#include <QPointF>
#include <QMouseEvent>
#include <QKeyEvent>
#include "colorselectorpopup.h"
#include "editablepolylineitem.h"
#include <QTime>

class HandleItem; // 前向声明
class EditableLineItem;
class GraphicsToolView : public QGraphicsView
{
    Q_OBJECT

public:
    enum class DrawingMode {
        None,     // 无模式
        Line,     // 直线
        Polyline, // 折线
        Rectangle,// 矩形
        Ellipse,  // 椭圆
        Arc,      // 弧线
        Polygon,  // 多边形
        Text      // 文本
    };
    GraphicsToolView(QGraphicsScene *scene, QWidget *parent = nullptr);
    void setDrawingMode(DrawingMode mode); // 设置绘图模式
    void setDrawingColor(const QColor &color); // 设置绘图颜色
    QColor currentDrawingColor() const; // 获取当前绘图颜色

    void applyColorToSelectedItems(const QColor &color); // 应用颜色到选中项
public slots:
    void copySelectedItems(); // 复制选中项
    void pasteCopiedItems(); // 粘贴复制项
    void showColorSelector(); // 显示颜色选择器
    void onColorSelected(const QColor &color); // 处理颜色选择

public:
    void wheelEvent(QWheelEvent *event) override; // 滚轮事件
    void mousePressEvent(QMouseEvent *event) override; // 鼠标按下
    void mouseMoveEvent(QMouseEvent *event) override; // 鼠标移动
    void keyPressEvent(QKeyEvent *event) override; // 按键按下
    void mouseReleaseEvent(QMouseEvent *event) override; // 鼠标释放
    void keyReleaseEvent(QKeyEvent *event) override; // 按键释放
    void handlePolylineModePress(QMouseEvent *event); // 折线模式按下处理
    void handlePolylineModeMove(QMouseEvent *event); // 折线模式移动处理
    void finishPolyline(); // 结束折线绘制
    bool checkHandleHit(const QPointF &scenePos); // 检查端点是否被点击

    void handleNoneModePress(QMouseEvent *event); // 无模式按下处理
    bool checkSelectedGroupHit(const QPointF &scenePos, qreal tolerance); // 检查选中组是否被点击
    void handleItemSelection(const QPointF &scenePos, qreal tolerance, bool isMultiSelect); // 处理项选择
    void handleLineModePress(QMouseEvent *event); // 直线模式按下处理
    void handleLineModeMove(QMouseEvent *event); // 直线模式移动处理
    void handleHandleMove(QMouseEvent *event); // 端点移动处理
    void handleGroupMove(QMouseEvent *event); // 组移动处理
    void handleLineModeRelease(QMouseEvent *event); // 直线模式释放处理
    void handleHandleRelease(); // 端点释放处理
    void handleGroupRelease(); // 组释放处理

    void updateCursorBasedOnPosition(const QPointF &scenePos); // 根据位置更新光标
    void mouseDoubleClickEvent(QMouseEvent *event); // 双击事件
    void updatePolylinePreview(const QPointF &currentMousePos); // 更新折线预览
private:
    QColor drawingColor; // 当前绘图颜色
    ColorSelectorPopup *colorSelector; // 颜色选择器弹窗

    bool isCtrlPressedForCopy; // 是否按住Ctrl键进行复制
    bool isDrawingLine = false; // 是否正在画直线
    QPointF startPoint; // 起始点
    QPointF endPoint; // 结束点
    QGraphicsLineItem *previewLine = nullptr; // 预览直线
    DrawingMode currentMode; // 当前模式
    QList<QGraphicsItem*> selectedItems; // 选中项列表
    QList<QGraphicsItem*> copiedItems; // 复制的项列表

    HandleItem* draggedHandle = nullptr; // 当前拖动的端点
    QGraphicsItem* draggedItem = nullptr; // 当前拖动的项

    bool isDraggingSelectionGroup = false; // 是否拖动选中组
    QPointF lastDragPos; // 上次拖动位置
    bool isDragging; // 是否正在拖动
    QPointF dragStartPosition; // 拖动起始位置
    QPointF fixedRotationCenter; // 旋转中心位置

    void cleanupDrawing(); // 清理绘图状态
    void cleanupSelection(); // 清理选择状态
    bool isShiftPressed; // 是否按住Shift键

    // 折线绘制相关
    bool isDrawingPolyline = false; // 是否正在绘制折线
    QVector<QPointF> polylinePoints; // 折线顶点
    EditablePolylineItem *currentPolyline = nullptr; // 当前折线对象
    int draggedHandleIndex; // 拖动的端点索引
    bool closePolylineOnFinish = false; // 结束时是否闭合折线

    EditableLineItem *previewClosingSegment = nullptr; // 闭合预览线段
};

#endif // GRAPHICSTOOLVIEW_H
