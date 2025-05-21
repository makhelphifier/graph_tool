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
public slots:
    void newFileWindow();
    void graphManagementWindow();
    void displayNewImage(const QString &imageName);

private slots: // *** 新增：私有槽 ***
    void onColorSelected(const QColor &color);
    void closeColorPopup(); // 用于关闭弹窗的槽


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
};
#endif // MAINWINDOW_H
