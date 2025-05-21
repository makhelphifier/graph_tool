#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "newimagedialog.h"
#include <QColor> // 包含 QColor
#include <QGraphicsView> // 包含 QGraphicsView
#include <QGraphicsScene> // 包含 QGraphicsScene
#include <QActionGroup> // 用于管理工具动作
#include "graphicstoolview.h"
#include <QToolButton>

class ColorSelectorPopup; // 前向声明

enum class ToolType {
    Select, // 选择工具
    Line    // 直线工具
    // 后续添加其他工具
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr); // 构造函数
    ~MainWindow(); // 析构函数

    void initMenu(); // 初始化菜单

public slots:
    void newFileWindow(); // 新建文件窗口
    void graphManagementWindow(); // 图形管理窗口
    void displayNewImage(const QString &imageName); // 显示新图像

private slots:
    void onColorSelected(const QColor &color); // 颜色选择处理
    void closeColorPopup(); // 关闭颜色弹窗

private:
    Ui::MainWindow *ui;
    QLabel *imageLabel; // 图像标签

    // 图形视图相关
    QGraphicsScene *scene; // 场景，持有图形项
    GraphicsToolView *graphicsView; // 视图，显示场景

    // 工具管理
    QActionGroup *toolActionGroup; // 工具动作组（单选行为）
    ToolType currentTool = ToolType::Select; // 当前选中的工具

    // 工具动作
    QAction *selectAction; // 选择工具动作
    QAction *lineAction; // 直线工具动作

    // 颜色选择相关
    QToolButton *lineColorButton = nullptr; // 线条颜色按钮
    QColor currentLineColor = Qt::black; // 当前线条颜色
    ColorSelectorPopup *colorPopup = nullptr; // 颜色选择弹窗

    void updateLineColorButtonIcon(); // 更新线条颜色按钮图标
};
#endif // MAINWINDOW_H
