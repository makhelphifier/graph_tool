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
#include <QGraphicsEllipseItem> // 新增：包含椭圆项
#include <QGraphicsPathItem> // 新增：用于绘制路径，包括圆弧
#include <QPainterPath>      // 新增：用于定义路径
#include <QGraphicsPolygonItem> // 新增：用于绘制多边形
#include <QGraphicsTextItem> // 新增：用于显示文本

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

    // 新增文本模式处理函数
    void handleTextModePress(QMouseEvent *event);
    // 文本模式下，移动和释放可能不需要特殊处理，主要在按下时创建


    // 新增多边形模式处理函数
    void handlePolygonModePress(QMouseEvent *event);
    void handlePolygonModeMove(QMouseEvent *event);
    void finishPolygon(); // 结束多边形绘制 (可能与折线共用部分逻辑)


    // 新增圆弧模式处理函数
    void handleArcModePress(QMouseEvent *event);
    void handleArcModeMove(QMouseEvent *event);
    void handleArcModeRelease(QMouseEvent *event); // 第三次点击可以视为释放来完成


    // 新增矩形模式处理函数
    void handleRectangleModePress(QMouseEvent *event);
    void handleRectangleModeMove(QMouseEvent *event);
    void handleRectangleModeRelease(QMouseEvent *event);
    // 新增椭圆模式处理函数
    void handleEllipseModePress(QMouseEvent *event);
    void handleEllipseModeMove(QMouseEvent *event);
    void handleEllipseModeRelease(QMouseEvent *event);



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
    void updatePolylinePreview(const QPointF &currentMousePos);
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

    EditableLineItem *previewClosingSegment = nullptr; // 闭合预览线（从鼠标到第一个点） // 新增



    // 矩形绘制相关
    QGraphicsRectItem *previewRect = nullptr; // 新增：预览矩形
    QPointF rectStartPoint; // 新增：矩形绘制的起始点


    // 椭圆绘制相关 (新增)
    QGraphicsEllipseItem *previewEllipse = nullptr; // 新增：预览椭圆
    QPointF ellipseStartPoint; // 新增：椭圆绘制的起始点 (与rectStartPoint功能类似，但可以分开管理)



    // 圆弧绘制相关 (新增)
    enum class ArcDrawingState {
        DefineCenter,     // 定义圆心
        DefineRadiusStart, // 定义半径和起始点
        DefineEnd         // 定义结束点/角度
    };
    ArcDrawingState currentArcState;
    QPointF arcCenterPoint;
    QPointF arcRadiusStartPoint;
    QGraphicsPathItem *previewArc = nullptr; // 预览圆弧

    // 多边形绘制相关 (新增)
    // 注意：多边形绘制逻辑与折线非常相似，可以复用许多变量
    // bool isDrawingPolygon = false; // 可以复用 isDrawingPolyline
    // QVector<QPointF> polygonPoints; // 可以复用 polylinePoints
    QGraphicsPolygonItem *previewPolygon = nullptr; // 预览多边形
    // EditablePolylineItem *currentPolyline = nullptr; // 这个可能需要调整为 QGraphicsPolygonItem


    // 文本绘制相关 (新增)
    // QPointF textInsertionPoint; // 如果需要预览或更复杂逻辑
};

#endif // GRAPHICSTOOLVIEW_H
