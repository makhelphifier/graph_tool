#ifndef IMAGECANVAS_H
#define IMAGECANVAS_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPixmap>
#include <QRect>
#include <QKeyEvent> // *** 新增：包含键盘事件头文件 ***
#include <QInputDialog> // *** 新增：包含输入对话框头文件 ***
#include <QString>    // *** 新增：包含 QString 头文件 ***
#include <QKeyEvent>  // 确保包含
#include <QRectF>
#include <QLineF>
#include <QLineEdit> // *** 新增：包含 QLineEdit ***
#include <QColor> // *** 新增：包含 QColor ***

class ImageCanvas : public QWidget
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

    ImageCanvas(QWidget *parent = nullptr);

    void setDrawingLineMode(bool enabled);
    void setImage(const QPixmap &pixmap);
    void setDrawingMode(DrawingMode mode);

    void setCurrentPenColor(const QColor &color); // *** 新增：设置当前画笔颜色 ***


protected:
    // 不再需要 mouseReleaseEvent
    // void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override ;
    void mousePressEvent(QMouseEvent *event) override ;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override; // *** 新增：键盘按下事件声明 ***

private:
    QPixmap image;
    DrawingMode currentMode;
    QPoint currentMousePos;
    QPoint startPoint;      // 用于画线段模式的起点 (也可以考虑整合)
    QPoint currentPoint;    // 鼠标当前位置或临时终点
    QVector<QPoint> currentPolylinePoints; // *** 新增：存储当前折线顶点 ***
    bool hasStartPoint;     // *** 新增/恢复：标记是否已设置起点 (用于 Line, Rectangle) ***

    QVector<QPoint> currentShapePoints; // 用于 Polyline 和 Polygon: // *** 可以考虑重命名 currentPolylinePoints ***

    // Arc 专用:
    int arcStep;            // *** 新增：标记圆弧绘制步骤 (0, 1, 2) ***
    // startPoint 将用作圆心 (centerPoint)
    QPoint radiusPoint;     // *** 新增：标记半径和圆弧起点的点 ***

    // --- 用于内联文本编辑 ---
    QLineEdit *textEditor = nullptr;   // *** 新增：指向当前文本编辑器的指针 ***
    QRect currentTextRect;          // *** 新增：存储当前编辑的文本框区域 ***

    QColor currentPenColor = Qt::black; // *** 新增：存储当前画笔颜色，默认为黑色 ***



    // 辅助函数，用于绘制最终图形到Pixmap
    void drawLineOnPixmap(const QPoint& p1, const QPoint& p2);
    void drawPolylineOnPixmap(const QVector<QPoint>& points);
    void drawRectangleOnPixmap(const QRect& rect);
    void drawEllipseOnPixmap(const QRect& rect);
    void drawPieOnPixmap(const QRectF& rect, int startAngle, int spanAngle); // *** 修改/新增：绘制扇形到 Pixmap ***
    void drawPolygonOnPixmap(const QVector<QPoint>& points); // *** 新增 ***
    void drawTextOnPixmap(const QRect& rect, const QString& text);
    // *** 新增：文本编辑辅助函数 ***
    void startInlineTextEdit();
    void finalizeTextEdit(bool saveText);
    // 如果不用信号槽，可以直接用这个函数作为 editingFinished 的目标
    void onTextEditingFinished(); // 这个函数内部会调用 finalizeTextEdit


};

#endif // IMAGECANVAS_H
