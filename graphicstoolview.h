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
        None,
        Line,
        Polyline,
        Rectangle,
        Ellipse,
        Arc,
        Polygon,
        Text
    };
    GraphicsToolView(QGraphicsScene *scene, QWidget *parent = nullptr);
    void setDrawingMode(DrawingMode mode);
    void setDrawingColor(const QColor &color); // 设置当前绘图颜色
    QColor currentDrawingColor() const; // 获取当前绘图颜色


    void applyColorToSelectedItems(const QColor &color);
public slots:
    void copySelectedItems();
    void pasteCopiedItems();
    void showColorSelector(); // 显示颜色选择器
    void onColorSelected(const QColor &color); // 处理颜色选择信号

public:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void handlePolylineModePress(QMouseEvent *event); // 新增折线模式处理
    void handlePolylineModeMove(QMouseEvent *event);  // 新增折线模式移动处理
    void finishPolyline(); // 结束折线绘制
    bool checkHandleHit(const QPointF &scenePos); // 更新以支持折线端点

    void handleNoneModePress(QMouseEvent *event);
    bool checkSelectedGroupHit(const QPointF &scenePos, qreal tolerance);
    void handleItemSelection(const QPointF &scenePos, qreal tolerance, bool isMultiSelect);
    void handleLineModePress(QMouseEvent *event);
    void handleLineModeMove(QMouseEvent *event);
    void handleHandleMove(QMouseEvent *event);
    void handleGroupMove(QMouseEvent *event);
    void handleLineModeRelease(QMouseEvent *event);
    void handleHandleRelease();
    void handleGroupRelease();

    void updateCursorBasedOnPosition(const QPointF &scenePos);
    void mouseDoubleClickEvent(QMouseEvent *event);
private:
    QColor drawingColor; // 当前绘图颜色
    ColorSelectorPopup *colorSelector; // 颜色选择器弹窗

    bool isCtrlPressedForCopy; // 用于记录拖动时是否按住 Ctrl 键以进行复制
    bool isDrawingLine = false;
    QPointF startPoint;
    QPointF endPoint;
    QGraphicsLineItem *previewLine = nullptr;
    DrawingMode currentMode;
    QList<QGraphicsItem*> selectedItems;
    QList<QGraphicsItem*> copiedItems; // 存储复制的图形项

    HandleItem* draggedHandle = nullptr; // 存储当前正在拖动的端点
    QGraphicsItem* draggedItem = nullptr; // 存储当前正在拖动的项

    bool isDraggingSelectionGroup = false; // 标志是否正在拖动选中组
    QPointF lastDragPos; // 记录上次拖动的位置，用于计算偏移量
    bool isDragging; // 标志用户是否正在拖动
    QPointF dragStartPosition; // 拖动开始时的鼠标位置
    QPointF fixedRotationCenter; // 用于保存旋转开始时的旋转中心位置

    void cleanupDrawing();
    void cleanupSelection();
    bool isShiftPressed;

    // 折线绘制相关变量
    bool isDrawingPolyline = false; // 是否正在绘制折线
    QVector<QPointF> polylinePoints; // 存储折线顶点
    EditablePolylineItem *currentPolyline = nullptr; // 当前正在绘制的折线对象
    int draggedHandleIndex;
    bool closePolylineOnFinish = false; // 新增：是否在结束时闭合折线

};

#endif // GRAPHICSTOOLVIEW_H
