#include "imagecanvas.h"
#include <QPainter>
#include <QDebug>
#include <QVector>
#include <QRectF> // 用于 QRectF 和 QRect
#include <QLineF> // 用于角度和长度计算
#include <cmath>  // 用于 std::round, std::fmod 等数学函数
#include <QPolygon> // 用于 drawPolygon (可选, QVector<QPoint> 也能用)
#include <QPainter>
#include <QColor> // 确保包含
#include <QKeyEvent> // *** 新增：包含键盘事件头文件 ***

ImageCanvas::ImageCanvas(QWidget *parent)
    : QWidget(parent),
    currentMode(DrawingMode::None), // 初始化模式为 None
    arcStep(0) ,// 初始化圆弧步骤为 0
    textEditor(nullptr)
// hasStartPoint 已移除, 对线/矩形/椭圆使用 startPoint.isNull() 判断
{
    setMinimumSize(400, 300); // 设置最小尺寸
    image = QPixmap(800, 600); // 初始化画布 Pixmap 大小
    image.fill(Qt::white); // 默认背景色为白色
    setMouseTracking(false); // 初始时，鼠标跟踪是关闭的 (只有在需要时才开启)
    setFocusPolicy(Qt::ClickFocus);

}

// 设置绘图模式的统一函数
void ImageCanvas::setDrawingMode(DrawingMode mode) {
    if (currentMode == mode) return; // 如果模式未改变，则不执行任何操作

    // --- 清理旧模式的状态 ---
    qDebug() << QStringLiteral("开始清理模式:") << static_cast<int>(currentMode);
    // 清理折线/多边形状态
    if ((currentMode == DrawingMode::Polyline || currentMode == DrawingMode::Polygon) && !currentShapePoints.isEmpty()) {
        qDebug() << QStringLiteral("模式切换，取消正在绘制的折线/多边形");
    }
    // 清理圆弧状态
    if (currentMode == DrawingMode::Arc && arcStep != 0) {
        qDebug() << QStringLiteral("模式切换，取消未完成的圆弧(扇形)");
    }
    // 清理两点点击模式的状态 (线/矩形/椭圆)
    if ((currentMode == DrawingMode::Line || currentMode == DrawingMode::Rectangle || currentMode == DrawingMode::Ellipse || currentMode == DrawingMode::Text) && !startPoint.isNull()) { // *** 修改：加入 Text 模式判断 ***
        qDebug() << QStringLiteral("模式切换，取消未完成的图形/文本框");
    }

    if (currentMode == DrawingMode::Text) {
        if (textEditor) { // 如果正在编辑文字时切换模式
            qDebug() << QStringLiteral("模式切换，取消正在编辑的文本");
            finalizeTextEdit(false); // 取消编辑，不保存
        } else if (!startPoint.isNull()) { // 如果刚点了第一下，还没编辑
            qDebug() << QStringLiteral("模式切换，取消未完成的文本框定义");
            startPoint = QPoint();
            setMouseTracking(false);
        }
    }

    // 重置所有可能处于活动状态的绘图状态变量
    arcStep = 0; // 重置圆弧步骤
    currentShapePoints.clear(); // 清空点集 (用于折线/多边形)
    startPoint = QPoint(); // 重置起点 (用于线/矩形/椭圆的起点, 或圆弧的圆心)
    radiusPoint = QPoint(); // 重置半径点 (用于圆弧)
    setMouseTracking(false); // 切换模式时总是关闭鼠标跟踪
    // --- 清理完成 ---

    currentMode = mode; // 设置新模式
    qDebug() << QStringLiteral("切换到模式:") << static_cast<int>(currentMode);

    // 根据新模式设置光标
    switch (currentMode) {
    case DrawingMode::None:
        setCursor(Qt::ArrowCursor); // 无模式时使用箭头光标
        break;
    default: // 所有绘图模式都使用十字光标
        setCursor(Qt::CrossCursor);
        break;
    }
    update(); // 更新显示以清除任何旧模式的预览
}

// 设置图像内容的函数（例如，加载文件或新建时）
void ImageCanvas::setImage(const QPixmap &pixmap) {
    image = pixmap; // 更新内部的 Pixmap
    qDebug() << QStringLiteral("图像已设置");
    update(); // 触发重绘以显示新图像
}



// --- 鼠标事件处理函数 ---

void ImageCanvas::mousePressEvent(QMouseEvent *event)
{
    // 只处理用于绘图的鼠标左键点击
    if (event->button() == Qt::LeftButton) {
        bool needsUpdate = true; // 除非另有说明，否则假定需要更新界面
        currentMousePos = event->pos(); // 按下时更新当前鼠标位置

        switch (currentMode) {
        case DrawingMode::Line:
        case DrawingMode::Rectangle:
        case DrawingMode::Ellipse:
            // --- 线段、矩形、椭圆的两点点击绘图逻辑 ---
            if (startPoint.isNull()) { // 第一次点击：设置起点
                startPoint = currentMousePos;
                setMouseTracking(true); // 启用鼠标跟踪以显示预览
                qDebug() << QStringLiteral("模式") << static_cast<int>(currentMode) << QStringLiteral(": 第一次点击，设置起点于") << startPoint;
            } else { // 第二次点击：完成形状绘制
                setMouseTracking(false); // 禁用鼠标跟踪
                QRect rect = QRect(startPoint, currentMousePos).normalized(); // 计算外接矩形 (用于矩形/椭圆)

                // 调用相应的绘制函数将图形画在 Pixmap 上
                if (currentMode == DrawingMode::Line) {
                    drawLineOnPixmap(startPoint, currentMousePos);
                } else if (currentMode == DrawingMode::Rectangle) {
                    if(rect.width() > 0 && rect.height() > 0) drawRectangleOnPixmap(rect);
                    else needsUpdate = false; // 如果矩形无效则不更新
                } else { // Ellipse
                    if(rect.width() > 0 && rect.height() > 0) drawEllipseOnPixmap(rect);
                    else needsUpdate = false; // 如果矩形无效则不更新
                }
                qDebug() << QStringLiteral("模式") << static_cast<int>(currentMode) << QStringLiteral(": 第二次点击，终点于") << currentMousePos << QStringLiteral("，绘制完成");
                startPoint = QPoint(); // 为下一个形状重置起点
            }
            break;

        case DrawingMode::Polyline:
        case DrawingMode::Polygon:
            // --- 折线和多边形的点击添加顶点逻辑 ---
            currentShapePoints.append(currentMousePos); // 添加当前点到列表
            if (currentShapePoints.size() == 1) {
                setMouseTracking(true); // 第一个点之后启用鼠标跟踪
            }
            qDebug() << QStringLiteral("模式") << static_cast<int>(currentMode) << QStringLiteral(": 添加顶点") << currentShapePoints.size() << "于" << currentMousePos;
            break;

        case DrawingMode::Arc:
            // --- 圆弧（扇形）的三点点击逻辑 ---
            if (arcStep == 0) { // 第一次点击：设置圆心
                startPoint = currentMousePos; // 使用 startPoint 作为圆心
                arcStep = 1; // 进入下一步
                setMouseTracking(true); // 开始跟踪鼠标用于画半径预览
                qDebug() << QStringLiteral("画圆弧(扇形)：步骤 1/3 - 设置圆心于") << startPoint;
            } else if (arcStep == 1) { // 第二次点击：设置半径/起点
                radiusPoint = currentMousePos; // 记录定义半径和起角的点
                if (radiusPoint == startPoint) { // 半径不能为零
                    qDebug() << QStringLiteral("画圆弧(扇形)：半径为0，请重新点击半径点");
                    needsUpdate = false; // 保持在步骤 1，让用户重选
                } else {
                    arcStep = 2; // 进入下一步，准备接收终点
                    qDebug() << QStringLiteral("画圆弧(扇形)：步骤 2/3 - 设置半径/起点于") << radiusPoint;
                }
            } else { // arcStep == 2, 第三次点击：设置终点并完成绘制
                setMouseTracking(false); // 结束绘制，禁用跟踪
                QLineF radiusLine(startPoint, radiusPoint); // 计算半径线
                qreal radius = radiusLine.length(); // 获取半径

                if(radius > 0) { // 必须有半径才能画圆弧
                    QLineF startLine(startPoint, radiusPoint); // 用于计算起始角的线
                    QLineF endLine(startPoint, currentMousePos);   // 用于计算终止角的线
                    qreal startAngleDeg = startLine.angle(); // 获取起始角 (度)
                    qreal endAngleDeg = endLine.angle();   // 获取终止角 (度)
                    qreal spanAngleDeg = endAngleDeg - startAngleDeg; // 计算扫过的角度

                    // QPainter 需要 1/16 度单位的角度
                    int startAngle16 = static_cast<int>(round(startAngleDeg * 16.0));
                    int spanAngle16 = static_cast<int>(round(spanAngleDeg * 16.0));
                    // 计算圆弧的外接矩形
                    QRectF boundingRect(startPoint.x() - radius, startPoint.y() - radius, radius * 2, radius * 2);

                    drawPieOnPixmap(boundingRect, startAngle16, spanAngle16); // 绘制最终的扇形到 Pixmap
                    qDebug() << QStringLiteral("画圆弧(扇形)：步骤 3/3 - 设置终点于") << currentMousePos << QStringLiteral("，绘制完成");
                } else {
                    qDebug() << QStringLiteral("画圆弧(扇形)：半径为0，无法绘制");
                    needsUpdate = false; // 无效操作，不更新
                }
                // 为下一次绘制重置圆弧状态
                arcStep = 0;
                startPoint = QPoint();
                radiusPoint = QPoint();
            }
            break;
        case DrawingMode::Text:
            if (!textEditor) { // 只有在没有编辑器时，点击才有效
                if (startPoint.isNull()) { // 第一次点击：设置起点
                    startPoint = currentMousePos;
                    setMouseTracking(true);
                    qDebug() << QStringLiteral("输入文字：第一次点击，设置边界框起点于") << startPoint;
                } else { // 第二次点击：定义区域并开始编辑
                    setMouseTracking(false);
                    currentTextRect = QRect(startPoint, currentMousePos).normalized(); // 记录区域
                    startPoint = QPoint(); // 重置起点

                    if (currentTextRect.width() > 1 && currentTextRect.height() > 1) { // 区域至少要大于1x1
                        startInlineTextEdit(); // *** 调用函数创建并显示 QLineEdit ***
                        needsUpdate = false; // 编辑器会自己显示，不需要这次 update
                    } else {
                        qDebug() << QStringLiteral("文本边界框太小或无效");
                        currentTextRect = QRect(); // 重置区域
                        needsUpdate = false;
                    }
                }
            }
        case DrawingMode::None: // 无绘图模式
        default:
            needsUpdate = false; // 无绘图操作，此处无需更新
            QWidget::mousePressEvent(event); // 将事件传递给基类处理
            break;
        }
        if(needsUpdate) update(); // 如果发生了有效的绘图操作，触发界面重绘
    } else {
        QWidget::mousePressEvent(event); // 处理非左键点击（例如右键菜单等）
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent *event)
{
    currentMousePos = event->pos(); // 总是更新当前鼠标位置，用于预览

    bool needsUpdate = false; // 检查是否需要重绘以显示预览
    switch (currentMode) {
    // 使用两点点击且在第一次点击后有预览的模式
    case DrawingMode::Line:
    case DrawingMode::Rectangle:
    case DrawingMode::Ellipse:
        if (!startPoint.isNull()) needsUpdate = true; // 如果起点已设置，需要更新预览
        break;
    // 使用顶点列表且在第一个点后有预览的模式
    case DrawingMode::Polyline:
    case DrawingMode::Polygon:
        if (!currentShapePoints.isEmpty()) needsUpdate = true; // 如果已开始绘制，需要更新预览
        break;
    // 圆弧模式预览在第一次点击后激活 (step > 0)
    case DrawingMode::Arc:
        if (arcStep > 0) needsUpdate = true; // 如果已开始定义圆弧，需要更新预览
        break;
    case DrawingMode::Text: // *** 新增：文本模式也需要更新预览矩形 ***
        if (!startPoint.isNull()) needsUpdate = true;
        break;
    case DrawingMode::None: // 无模式下无需预览
    default:
        break;
    }

    if(needsUpdate) update(); // 触发重绘以显示预览
    // else { QWidget::mouseMoveEvent(event); } // 通常不需要将移动事件传递给基类，除非需要在非绘图状态下也响应移动
}


void ImageCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 处理鼠标左键双击
    if (event->button() == Qt::LeftButton) {
        bool needsUpdate = false; // 标记是否需要更新界面
        if (currentMode == DrawingMode::Polyline) {
            // --- 完成折线绘制 ---
            // 如果还没有点，双击视为单击添加第一个点
            if (currentShapePoints.isEmpty()) { mousePressEvent(event); return; }
            currentShapePoints.append(event->pos()); // 添加最后一个点（双击位置）
            qDebug() << QStringLiteral("画折线：双击结束于") << event->pos();
            if (currentShapePoints.size() >= 2) { // 折线至少需要2个点
                drawPolylineOnPixmap(currentShapePoints); // 绘制最终折线
            } else { qDebug() << QStringLiteral("画折线：点数不足 (<2)，无法绘制"); }
            currentShapePoints.clear(); // 清空点集，准备下次绘制
            setMouseTracking(false); // 关闭鼠标跟踪
            needsUpdate = true;

        } else if (currentMode == DrawingMode::Polygon) {
            // --- 完成多边形绘制 ---
            // 如果还没有点，双击视为单击添加第一个点
            if (currentShapePoints.isEmpty()) { mousePressEvent(event); return; }
            currentShapePoints.append(event->pos()); // 添加最后一个点（双击位置）
            qDebug() << QStringLiteral("画多边形：双击结束于") << event->pos();
            if (currentShapePoints.size() >= 3) { // 多边形至少需要3个顶点
                drawPolygonOnPixmap(currentShapePoints); // 绘制最终多边形
            } else { qDebug() << QStringLiteral("画多边形：顶点数不足 (<3)，无法绘制"); }
            currentShapePoints.clear(); // 清空点集，准备下次绘制
            setMouseTracking(false); // 关闭鼠标跟踪
            needsUpdate = true;

        } else {
            QWidget::mouseDoubleClickEvent(event); // 其他模式将双击事件传递给基类
        }
        if(needsUpdate) update(); // 如果完成了绘制，更新界面
    } else {
        QWidget::mouseDoubleClickEvent(event); // 处理非左键双击
    }
}


// --- 新增：键盘按下事件处理 ---


void ImageCanvas::keyPressEvent(QKeyEvent *event)
{

    // *** 如果文本编辑器是活动的，优先处理它的按键 ***
    if (textEditor && currentMode == DrawingMode::Text) {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            finalizeTextEdit(true); // 回车确认输入
            return; // 事件已处理
        } else if (event->key() == Qt::Key_Escape) {
            finalizeTextEdit(false); // Esc 取消输入
            return; // 事件已处理
        }
        // 对于其他按键，可以允许 QLineEdit 自己处理，但如果需要画布也响应（可能不需要），可以考虑是否调用基类
        // QWidget::keyPressEvent(event); // 通常不需要，让编辑器处理输入即可
        return; // 阻止 Esc 触发下面的取消逻辑
    }

    // 检查是否按下了 Esc 键
    if (event->key() == Qt::Key_Escape) {
        bool drawingCancelled = false; // 标记是否真的取消了操作

        // 检查当前是否处于某个绘图模式的中间状态
        switch (currentMode) {
        case DrawingMode::Line:
        case DrawingMode::Rectangle:
        case DrawingMode::Ellipse:
            if (!startPoint.isNull()) { // 如果已设置起点 (等待终点)
                startPoint = QPoint(); // 重置起点
                setMouseTracking(false); // 关闭鼠标跟踪
                drawingCancelled = true;
                qDebug() << QStringLiteral("ESC: 取消绘制 线/矩形/椭圆");
            }
            break;
        case DrawingMode::Polyline:
        case DrawingMode::Polygon:
            if (!currentShapePoints.isEmpty()) { // 如果已经开始绘制点
                currentShapePoints.clear(); // 清空点集
                setMouseTracking(false); // 关闭鼠标跟踪
                drawingCancelled = true;
                qDebug() << QStringLiteral("ESC: 取消绘制 折线/多边形");
            }
            break;
        case DrawingMode::Arc:
            if (arcStep > 0) { // 如果已经开始定义圆弧 (点了圆心或半径点)
                arcStep = 0; // 重置步骤
                startPoint = QPoint(); // 清理圆心
                radiusPoint = QPoint(); // 清理半径点
                setMouseTracking(false); // 关闭鼠标跟踪
                drawingCancelled = true;
                qDebug() << QStringLiteral("ESC: 取消绘制 圆弧(扇形)");
            }
            break;
        case DrawingMode::None: // 无模式，不做操作
        default:
            break;
        }

        if (drawingCancelled) {
            update(); // 如果取消了绘制，更新界面以清除预览
            return; // Esc 事件已被处理，直接返回
        }
    }

    // 如果不是 Esc 键或者没有取消任何操作，则调用基类的处理方法
    QWidget::keyPressEvent(event);
}


// --- 用于绘制预览的 Paint 事件 ---

void ImageCanvas::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event); // 首先调用基类的实现 (可能擦除背景等)
    QPainter painter(this); // 创建 QPainter 用于在当前部件 (this) 上绘制

    // 1. 绘制背景图像（包含所有已完成的永久绘图）
    if (!image.isNull()) {
        painter.drawPixmap(0, 0, image);
    }

    // 2. 根据当前模式和状态绘制临时预览形状
    painter.setPen(QPen(Qt::red, 1, Qt::DashLine)); // 为所有预览设置样式（红色虚线）

    switch (currentMode) {
    case DrawingMode::Line: // 预览线段
        if (!startPoint.isNull()) { // 如果已设置起点
            painter.drawLine(startPoint, currentMousePos); // 从起点到当前鼠标位置画线
        }
        break;
    case DrawingMode::Rectangle: // 预览矩形
        if (!startPoint.isNull()) { // 如果已设置起点
            QRect previewRect = QRect(startPoint, currentMousePos).normalized(); // 计算预览矩形
            painter.drawRect(previewRect); // 绘制预览矩形边框
        }
        break;
    case DrawingMode::Ellipse: // 预览椭圆
        if (!startPoint.isNull()) { // 如果已设置起点
            QRect previewRect = QRect(startPoint, currentMousePos).normalized(); // 计算预览椭圆的外接矩形
            painter.drawEllipse(previewRect); // 绘制预览椭圆
        }
        break;
    case DrawingMode::Polyline: // 预览折线
        if (!currentShapePoints.isEmpty()) { // 如果已开始绘制
            // 如果至少有2个点，绘制已确定的折线段
            if (currentShapePoints.size() >= 2) {
                painter.drawPolyline(currentShapePoints.constData(), currentShapePoints.size());
            } else { // 如果只有1个点，可以画一个点标记
                painter.drawPoint(currentShapePoints.first());
            }
            // 从最后一个确定的点绘制到当前鼠标位置的预览线段
            painter.drawLine(currentShapePoints.last(), currentMousePos);
        }
        break;
    case DrawingMode::Polygon: // 预览多边形
        if (!currentShapePoints.isEmpty()) { // 如果已开始绘制
            // 如果至少有2个点，绘制已确定的边
            if (currentShapePoints.size() >= 2) {
                painter.drawPolyline(currentShapePoints.constData(), currentShapePoints.size());
            } else { // 如果只有1个点，画个点标记
                painter.drawPoint(currentShapePoints.first());
            }
            // 从最后一个确定的点绘制到当前鼠标位置的预览边
            painter.drawLine(currentShapePoints.last(), currentMousePos);
            // 绘制从当前鼠标位置回到第一个点的闭合预览边
            painter.drawLine(currentMousePos, currentShapePoints.first());
        }
        break;
    case DrawingMode::Arc: // 预览圆弧（扇形）
        if (arcStep == 1) { // 步骤1：预览半径线
            painter.drawLine(startPoint, currentMousePos); // 从圆心画到鼠标
            painter.drawEllipse(startPoint, 2, 2); // 在圆心画个小圆点标记
        } else if (arcStep == 2) { // 步骤2：预览扇形
            QLineF radiusLine(startPoint, radiusPoint); // 计算半径线
            qreal radius = radiusLine.length(); // 获取半径
            if(radius > 0) { // 必须有半径
                // 计算圆的外接矩形
                QRectF circleRect(startPoint.x() - radius, startPoint.y() - radius, radius * 2, radius * 2);
                // 计算起始角和当前鼠标位置对应的角度 (度)
                qreal startAngleDeg = QLineF(startPoint, radiusPoint).angle();
                qreal currentAngleDeg = QLineF(startPoint, currentMousePos).angle();
                qreal spanAngleDeg = currentAngleDeg - startAngleDeg; // 计算扫过的角度

                // 转换为 QPainter 使用的 1/16 度单位
                int startAngle16 = static_cast<int>(round(startAngleDeg * 16.0));
                int spanAngle16 = static_cast<int>(round(spanAngleDeg * 16.0));
                // 绘制预览扇形切片
                painter.drawPie(circleRect, startAngle16, spanAngle16);
            }
        }
        break;
    case DrawingMode::Text: // 绘制文本边界框预览
        if (!startPoint.isNull()) { // 只有在定义矩形时才画预览框
            QRect previewRect = QRect(startPoint, currentMousePos).normalized();
            painter.drawRect(previewRect);
        }
        break; // 结束 Text case
    case DrawingMode::None: // 无模式
    default:
        // 无需预览
        break;
    }
}


// --- 辅助函数：将最终形状绘制到 QPixmap 上 ---


void ImageCanvas::drawTextOnPixmap(const QRect& rect, const QString& text) {
    if (image.isNull() || !rect.isValid() || text.isEmpty()) return;
    QPainter painter(&image);
    painter.setPen(QPen(currentPenColor)); // 设置绘制文本的颜色
    // painter.setFont(QFont("Arial", 10)); // 可以设置字体和大小
    int flags = Qt::AlignCenter | Qt::TextWordWrap; // 文本居中并自动换行
    painter.drawText(rect, flags, text); // 在指定区域绘制文本
    qDebug() << QStringLiteral("最终绘制文本:") << text << QStringLiteral("于区域:") << rect;
}



// 绘制线段到 Pixmap
void ImageCanvas::drawLineOnPixmap(const QPoint& p1, const QPoint& p2) {
    if (image.isNull()) return; // 检查 Pixmap 是否有效
    QPainter painter(&image); // 创建用于在 Pixmap 上绘图的 Painter
    painter.setPen(QPen(currentPenColor, 2)); // 设置最终绘图样式（例如：黑色，2像素宽）
    painter.drawLine(p1, p2); // 绘制线段
    qDebug() << QStringLiteral("最终绘制线段从") << p1 << QStringLiteral("到") << p2; // 调试输出
    setDrawingMode(DrawingMode::None);

}

// 绘制折线到 Pixmap
void ImageCanvas::drawPolylineOnPixmap(const QVector<QPoint>& points) {
    if (image.isNull() || points.size() < 2) return; // 折线至少需要2个点
    QPainter painter(&image);
    painter.setPen(QPen(currentPenColor, 2));
    painter.drawPolyline(points.constData(), points.size()); // 使用 drawPolyline 绘制
    qDebug() << QStringLiteral("最终绘制折线，共") << points.size() << QStringLiteral("个点");
    setDrawingMode(DrawingMode::None);

}

// 绘制矩形到 Pixmap
void ImageCanvas::drawRectangleOnPixmap(const QRect& rect) {
    if (image.isNull() || !rect.isValid()) return; // 检查矩形是否有效
    QPainter painter(&image);
    painter.setPen(QPen(currentPenColor, 2));
    painter.drawRect(rect); // 使用 drawRect 绘制
    qDebug() << QStringLiteral("最终绘制矩形:") << rect;
    setDrawingMode(DrawingMode::None);

}

// 绘制椭圆到 Pixmap
void ImageCanvas::drawEllipseOnPixmap(const QRect& rect) {
    if (image.isNull() || !rect.isValid()) return; // 椭圆的外接矩形必须有效
    QPainter painter(&image);
    painter.setPen(QPen(currentPenColor, 2));
    painter.drawEllipse(rect); // 使用 drawEllipse 绘制
    qDebug() << QStringLiteral("最终绘制椭圆，外接矩形:") << rect;
    setDrawingMode(DrawingMode::None);

}

// 绘制扇形到 Pixmap
void ImageCanvas::drawPieOnPixmap(const QRectF& rect, int startAngle16, int spanAngle16) {
    if (image.isNull() || !rect.isValid()) return; // 扇形的外接矩形必须有效
    QPainter painter(&image);
    painter.setPen(QPen(currentPenColor, 2));
    painter.drawPie(rect, startAngle16, spanAngle16); // 使用 drawPie 绘制
    qDebug() << QStringLiteral("最终绘制扇形，外接矩形:") << rect << " 起始角(1/16):" << startAngle16 << " 跨度角(1/16):" << spanAngle16;
    setDrawingMode(DrawingMode::None);

}

// 绘制多边形到 Pixmap
void ImageCanvas::drawPolygonOnPixmap(const QVector<QPoint>& points) {
    if (image.isNull() || points.size() < 3) return; // 多边形至少需要3个点
    QPainter painter(&image);
    painter.setPen(QPen(currentPenColor, 2));
    // drawPolygon 会自动闭合第一个和最后一个点
    painter.drawPolygon(points.constData(), points.size());
    // 或者使用 QPolygon 对象: painter.drawPolygon(QPolygon(points));
    qDebug() << QStringLiteral("最终绘制多边形，共") << points.size() << QStringLiteral("个顶点");
    setDrawingMode(DrawingMode::None);
}







// 开始内联文本编辑
void ImageCanvas::startInlineTextEdit() {
    // 如果已有编辑器，先销毁它（理论上不应发生，但以防万一）
    if (textEditor) {
        textEditor->deleteLater();
        textEditor = nullptr;
    }

    // 创建 QLineEdit
    textEditor = new QLineEdit(this); // 父对象设为 ImageCanvas
    textEditor->setGeometry(currentTextRect); // 设置位置和大小
    textEditor->setFont(this->font()); // 可以设置字体，例如使用画布的字体
    // textEditor->setStyleSheet("background-color: yellow; border: 1px solid black;"); // 可以设置样式
    textEditor->setAlignment(Qt::AlignCenter); // 设置文本对齐

    // 连接 editingFinished 信号到我们的处理函数
    // connect(textEditor, &QLineEdit::editingFinished, this, &ImageCanvas::onTextEditingFinished);
    // 注意：如果 .h 文件没有 Q_OBJECT，就不能用信号槽，需要手动处理 Enter/Esc 或焦点事件

    textEditor->show(); // 显示编辑器
    textEditor->setFocus(); // 设置键盘焦点，让用户可以直接输入
    qDebug() << QStringLiteral("开始内联文本编辑，区域:") << currentTextRect;
}

// 文本编辑完成的处理（由 editingFinished 信号或按键事件调用）
void ImageCanvas::finalizeTextEdit(bool saveText) {
    if (!textEditor) return; // 如果编辑器不存在，直接返回

    qDebug() << QStringLiteral("结束内联文本编辑，保存:") << saveText;
    QString text;
    if (saveText) {
        text = textEditor->text(); // 获取编辑器中的文本
    }

    // 标记稍后删除编辑器，这样更安全
    textEditor->deleteLater();
    textEditor = nullptr; // 将指针置空，表示编辑器不再活动

    // 如果需要保存且文本不为空，则绘制
    if (saveText && !text.isEmpty() && currentTextRect.isValid()) {
        drawTextOnPixmap(currentTextRect, text); // 绘制到背景 Pixmap
        update(); // 更新画布显示最终结果
    } else {
        update(); // 即使用户取消或文本为空，也需要更新以确保编辑器残留被清除
    }

    currentTextRect = QRect(); // 清理当前文本区域
}

// 如果使用信号槽，这个函数作为槽
void ImageCanvas::onTextEditingFinished() {
    // 这个信号可能在多种情况下触发（回车、焦点丢失）
    // 我们在这里统一调用 finalize 并保存文本
    finalizeTextEdit(true);
}



// *** 新增：设置当前画笔颜色的方法 ***
void ImageCanvas::setCurrentPenColor(const QColor &color)
{
    if (color.isValid()) {
        currentPenColor = color;
        qDebug() << "ImageCanvas: Pen color set to" << currentPenColor.name();
    }
}







