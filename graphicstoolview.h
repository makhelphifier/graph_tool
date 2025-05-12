#ifndef GRAPHICSTOOLVIEW_H
#define GRAPHICSTOOLVIEW_H

#include <QGraphicsView>
#include <QWidget>
#include <QWheelEvent>
#include <QGraphicsLineItem>
#include <QPointF>
#include <QMouseEvent>
#include <QKeyEvent>
class HandleItem; // 前向声明

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

public slots:
    void copySelectedItems();
    void pasteCopiedItems();
protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void handleNoneModePress(QMouseEvent *event);
    bool checkHandleHit(const QPointF &scenePos);
    bool checkSelectedGroupHit(const QPointF &scenePos, qreal tolerance);
    void handleItemSelection(const QPointF &scenePos, qreal tolerance, bool isMultiSelect);
    void handleLineModePress(QMouseEvent *event);
    void handleLineModeMove(QMouseEvent *event);
    void handleHandleMove(QMouseEvent *event);
    void handleGroupMove(QMouseEvent *event);
    void handleLineModeRelease(QMouseEvent *event);
    void handleHandleRelease();
    void handleGroupRelease();



private:
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
};

#endif // GRAPHICSTOOLVIEW_H
