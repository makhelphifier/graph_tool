#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "newimagedialog.h"
#include <QColor> // *** 新增：包含 QColor ***
#include <QGraphicsView> // Include QGraphicsView
#include <QGraphicsScene> // Include QGraphicsScene
#include <QActionGroup>   // For managing tool actions
#include "graphicstoolview.h"
#include <QToolButton>
class ColorSelectorPopup; // *** 新增 ***

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
    void updateFillColorButtonIcon(); // *** 新增：更新填充按钮外观 ***

public slots:
    void newFileWindow();
    void graphManagementWindow();
    void displayNewImage(const QString &imageName);

private slots: // *** 新增：私有槽 ***
    void onColorSelected(const QColor &color);
    void closeColorPopup(); // 用于关闭弹窗的槽
    void onFillColorSelected(const QColor &color); // *** 新增：填充颜色槽 ***
    void closeFillColorPopup(); // *** 新增：关闭填充弹窗槽 ***
    void onBackgroundImageSelected(const QString &imagePath); // 处理背景图片选择


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
    QToolButton *lineColorButton = nullptr; // *** 新增 ***
    QColor currentLineColor = Qt::black;  // *** 新增：存储当前颜色 ***
    ColorSelectorPopup *colorPopup = nullptr; // *** 新增 ***

    void updateLineColorButtonIcon(); // *** 新增：更新按钮外观的辅助函数 ***

    QToolButton *fillColorButton = nullptr; // *** 新增 ***
    QColor currentFillColor = Qt::transparent; // *** 新增：存储当前填充颜色, 默认透明 ***
    ColorSelectorPopupFill *fillColorPopup = nullptr; // *** 新增 ***

    QString currentFillImagePath; // 新增：存储当前选择的填充图片路径

};
#endif // MAINWINDOW_H
