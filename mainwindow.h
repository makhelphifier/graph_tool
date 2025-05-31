#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "new_image_dialog.h"
#include <QColor> // *** 包含 QColor ***
#include <QGraphicsView> // Include QGraphicsView
#include <QGraphicsScene> // Include QGraphicsScene
#include <QActionGroup>   // For managing tool actions
#include "graphics_tool_view.h"
#include <QToolButton>
#include <QSpinBox> //  包含 QSpinBox
#include <QComboBox>

class ColorSelectorPopup; // *** 前向声明 ***

class ColorSelectorPopupFill;

enum class ToolType {
    Select,
    Line
    // Add other tools later
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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initMenu();
    void updateFillColorButtonIcon(); // *** 更新填充按钮外观 ***

public slots:
    void newFileWindow();
    void graphManagementWindow();
    void displayNewImage(const QString &imageName);

private slots: // *** 私有槽 ***
    void onColorSelected(const QColor &color);
    void closeColorPopup(); // 用于关闭弹窗的槽
    void onFillColorSelected(const QColor &color); // *** 填充颜色槽 ***
    void closeFillColorPopup(); // *** 关闭填充弹窗槽 ***
    void onBackgroundImageSelected(const QString &imagePath); // 处理背景图片选择
    void onLineThicknessChanged(int thickness); // 线条粗细变化槽
    void onLineStyleChanged(int  index);//线条样式变化槽
    void onAlignLeftTriggered(); //左对齐槽
    void onAlignRightTriggered(); //  右对齐槽
    void onAlignTopTriggered(); // 顶对齐槽
    void onAlignBottomTriggered(); // 底对齐槽
    void onAlignCenterVerticalTriggered(); // 垂直居中对齐槽
    void onAlignCenterHorizontalTriggered(); // 水平居中对齐槽

private:
    Ui::MainWindow *ui;
    QLabel *imageLabel;

    // --- Graphics View Related ---
    QGraphicsScene *scene;       // The scene to hold items
    GraphicsToolView *graphicsView; // The view to display the scene
        // OR: Replace with your custom GraphicsView subclass if you create one

    // --- Tool Management ---
    QActionGroup *toolActionGroup; // Group for tool actions (radio button behavior)
    ToolType currentTool = ToolType::Select; // Track the currently selected tool

    // Actions for tools
    QAction *selectAction;
    QAction *lineAction;


    // --- 颜色选择相关 ---
    QToolButton *lineColorButton = nullptr; // *** 线条颜色按钮 ***
    QColor currentLineColor = Qt::black;  // *** 存储当前颜色 ***
    ColorSelectorPopup *colorPopup = nullptr; // *** 颜色选择弹窗 ***

    void updateLineColorButtonIcon(); // *** 更新按钮外观的辅助函数 ***

    QToolButton *fillColorButton = nullptr; // *** 填充颜色按钮 ***
    QColor currentFillColor = Qt::transparent; // *** 存储当前填充颜色, 默认透明 ***
    ColorSelectorPopupFill *fillColorPopup = nullptr; // *** 填充颜色选择弹窗 ***

    QString currentFillImagePath; // 存储当前选择的填充图片路径
    QSpinBox *lineThicknessSpinBox; // 线条粗细选择框
    int currentLineThickness = 2; // 当前线条粗细，默认2像素
    QComboBox *lineStyleComboBox; //  线条样式选择框

};
#endif // MAINWINDOW_H
