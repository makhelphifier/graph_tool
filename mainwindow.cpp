#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "graphmanager.h"
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QWidgetAction>
#include <QPixmap>
#include <QPainter>
#include "colorselectorpopup.h"
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include "graphicstoolview.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene(this)) // 初始化场景
    , graphicsView(new GraphicsToolView(scene, this)) // 初始化图形视图
{
    scene->setSceneRect(-1000, -1000, 2000, 2000); // 设置场景范围

    initMenu(); // 初始化菜单

    resize(800, 600); // 设置窗口大小

    graphicsView->setRenderHint(QPainter::Antialiasing); // 设置抗锯齿渲染

    QWidget *centralWidget = new QWidget(this); // 创建中心部件
    QVBoxLayout *layout = new QVBoxLayout(centralWidget); // 创建垂直布局
    layout->addWidget(graphicsView, 1); // 添加图形视图
    centralWidget->setLayout(layout); // 设置布局
    setCentralWidget(centralWidget); // 设置中心部件

    QToolBar *toolBar = addToolBar("Tools"); // 添加工具栏

    QPushButton *drawTextButton = new QPushButton("输入文字"); // 文字按钮
    toolBar->addWidget(drawTextButton);
    connect(drawTextButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Text); // 设置文字模式
    });

    QPushButton *drawLineButton = new QPushButton("画线段"); // 线段按钮
    toolBar->addWidget(drawLineButton);
    connect(drawLineButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Line); // 设置直线模式
    });

    QPushButton *drawPolylineButton = new QPushButton("画折线"); // 折线按钮
    toolBar->addWidget(drawPolylineButton);
    connect(drawPolylineButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Polyline); // 设置折线模式
    });

    // 闭合折线按钮
    QAction *closePolylineAction = new QAction(tr("闭合折线 (C)"), this);
    connect(closePolylineAction, &QAction::triggered, [=]() {
        QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_C, Qt::NoModifier); // 模拟C键按下
        graphicsView->keyPressEvent(&keyEvent); // 处理按键事件
    });
    toolBar->addAction(closePolylineAction); // 添加动作到工具栏

    QPushButton *selectButton = new QPushButton("取消绘制"); // 取消绘制按钮
    toolBar->addWidget(selectButton);
    connect(selectButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::None); // 设置无模式
    });

    QPushButton *drawRectangleButton = new QPushButton("画矩形"); // 矩形按钮
    toolBar->addWidget(drawRectangleButton);
    connect(drawRectangleButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Rectangle); // 设置矩形模式
    });

    QPushButton *drawEllipseButton = new QPushButton("画椭圆"); // 椭圆按钮
    toolBar->addWidget(drawEllipseButton);
    connect(drawEllipseButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Ellipse); // 设置椭圆模式
    });

    QPushButton *drawArcButton = new QPushButton("画圆弧"); // 圆弧按钮
    toolBar->addWidget(drawArcButton);
    connect(drawArcButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Arc); // 设置圆弧模式
    });

    QPushButton *drawPolygonButton = new QPushButton("画多边形"); // 多边形按钮
    toolBar->addWidget(drawPolygonButton);
    connect(drawPolygonButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Polygon); // 设置多边形模式
    });

    lineColorButton = new QToolButton(this); // 线条颜色按钮
    lineColorButton->setText(QStringLiteral("线条颜色")); // 设置文本
    lineColorButton->setToolTip(QStringLiteral("选择线条颜色")); // 设置提示
    lineColorButton->setPopupMode(QToolButton::InstantPopup); // 设置弹出模式
    lineColorButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 设置样式
    updateLineColorButtonIcon(); // 更新图标
    toolBar->addWidget(lineColorButton); // 添加到工具栏

    colorPopup = new ColorSelectorPopup(this); // 颜色选择弹窗
    connect(colorPopup, &ColorSelectorPopup::colorSelected, this, &MainWindow::onColorSelected); // 连接颜色选择信号
    connect(colorPopup, &ColorSelectorPopup::closePopup, this, &MainWindow::closeColorPopup); // 连接关闭弹窗信号

    QMenu *colorMenu = new QMenu(lineColorButton); // 颜色菜单
    QWidgetAction *colorWidgetAction = new QWidgetAction(colorMenu); // 部件动作
    colorWidgetAction->setDefaultWidget(colorPopup); // 设置默认部件
    colorMenu->addAction(colorWidgetAction); // 添加动作到菜单

    lineColorButton->setMenu(colorMenu); // 设置菜单到按钮
}

void MainWindow::onColorSelected(const QColor &color)
{
    if (color.isValid()) {
        currentLineColor = color; // 设置当前颜色
        qDebug() << "主窗口：颜色已选择 -" << color.name(); // 打印选择信息
        if (graphicsView) {
            graphicsView->setDrawingColor(currentLineColor); // 设置绘图颜色
        }
        updateLineColorButtonIcon(); // 更新按钮图标
    }
}

void MainWindow::closeColorPopup()
{
    if (lineColorButton && lineColorButton->menu()) {
        lineColorButton->menu()->hide(); // 隐藏颜色菜单
    }
}

void MainWindow::updateLineColorButtonIcon()
{
    if (!lineColorButton) return; // 按钮不存在则返回

    int iconSize = 16; // 图标大小
    QPixmap pixmap(iconSize, iconSize); // 创建像素图
    pixmap.fill(Qt::transparent); // 填充透明背景

    QPainter painter(&pixmap); // 创建画家
    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿
    painter.setBrush(currentLineColor); // 设置画刷颜色
    painter.setPen(Qt::gray); // 设置画笔颜色
    painter.drawRect(0, 0, iconSize - 1, iconSize - 1); // 绘制矩形

    lineColorButton->setIcon(QIcon(pixmap)); // 设置按钮图标
}

void MainWindow::graphManagementWindow()
{
    GraphManager *newWindow = new GraphManager(this); // 创建图形管理窗口
    newWindow->show(); // 显示窗口
}

void MainWindow::newFileWindow()
{
    NewImageDialog *newWindow = new NewImageDialog(this); // 创建新建图像对话框
    newWindow->show(); // 显示对话框
    connect(newWindow, &NewImageDialog::imageCreated, this, &MainWindow::displayNewImage); // 连接图像创建信号
}

void MainWindow::displayNewImage(const QString &imageName)
{
    QPixmap blankImage(800, 600); // 创建空白图像
    blankImage.fill(Qt::white); // 填充白色
    if (scene) {
        scene->clear(); // 清空场景
    }
}

void MainWindow::initMenu()
{
    QMenuBar *menuBar = this->menuBar(); // 获取菜单栏
    QMenu *fileMenu = menuBar->addMenu(tr("文件(&F)")); // 添加文件菜单
    QAction *newAction = fileMenu->addAction(tr("新建(&N)")); newAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F)); // 新建动作
    QAction *openAction = fileMenu->addAction(tr("打开(&O)")); openAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O)); // 打开动作
    QAction *saveAction = fileMenu->addAction(tr("保存")); saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S)); // 保存动作
    QAction *exportAction = fileMenu->addAction(tr("导出...")); exportAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E)); // 导出动作
    QAction *importAction = fileMenu->addAction(tr("导入...")); importAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I)); // 导入动作
    fileMenu->addSeparator(); // 添加分隔线
    QAction *graphManagementAction = fileMenu->addAction(tr("图管理(&T)...")); graphManagementAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G)); // 图管理动作
    QAction *symboManagementlAction = fileMenu->addAction(tr("符号管理(&M)...")); symboManagementlAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M)); // 符号管理动作
    fileMenu->addSeparator(); // 添加分隔线
    QAction *pritAction = fileMenu->addAction(tr("打印(&P)...")); pritAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P)); // 打印动作
    QAction *printViewAction = fileMenu->addAction(tr("打印预览(&V)")); // 打印预览动作
    QAction *pageSettingAction = fileMenu->addAction(tr("页面设置...(&U)")); // 页面设置动作
    fileMenu->addSeparator(); // 添加分隔线
    QAction *closeAction = fileMenu->addAction(tr("关闭(&X)")); // 关闭动作

    QMenu *editMenu = menuBar->addMenu(tr("编辑(&E)")); // 添加编辑菜单
    QAction *undoAction = editMenu->addAction(tr("撤销(&U)")); // 撤销动作
    QAction *redoAction = editMenu->addAction(tr("重做(&R)")); // 重做动作
    fileMenu->addSeparator(); // 添加分隔线
    QAction *cutAction = editMenu->addAction(tr("剪切(&T)")); cutAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X)); // 剪切动作
    QAction *copyAction = editMenu->addAction(tr("复制(&C)")); copyAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C)); // 复制动作
    QAction *pasteAction = editMenu->addAction(tr("粘贴(&P)")); pasteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V)); // 粘贴动作
    QAction *deleteAction = editMenu->addAction(tr("删除(&D)")); deleteAction->setShortcut(QKeySequence(Qt::Key_Delete)); // 删除动作
    QAction *selectAllAction = editMenu->addAction(tr("全选(&A)")); selectAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A)); // 全选动作

    QMenu *viewMenu = menuBar->addMenu(tr("视窗(&V)")); // 添加视窗菜单
    QAction *markToolRefAction = viewMenu->addAction(tr("标准工具条")); // 标准工具条动作
    QAction *iconMarkToolRefAction = viewMenu->addAction(tr("图形编辑工具条")); // 图形编辑工具条动作
    QAction *forToolRefAction = viewMenu->addAction(tr("对齐工具条")); // 对齐工具条动作
    QAction *fontToolRefAction = viewMenu->addAction(tr("字体工具条")); // 字体工具条动作
    QAction *lineToolRefAction = viewMenu->addAction(tr("线形工具条")); // 线形工具条动作
    QAction *iconManageToolRefAction = viewMenu->addAction(tr("图层管理工具条")); // 图层管理工具条动作
    QAction *runToolRefAction = viewMenu->addAction(tr("运行工具条")); // 运行工具条动作
    QAction *statusBarAction = viewMenu->addAction(tr("状态条")); // 状态条动作
    QAction *gridAction = viewMenu->addAction(tr("网格")); // 网格动作
    QAction *drawResourceExplorerAction = viewMenu->addAction(tr("绘图资源管理器")); // 绘图资源管理器动作

    QMenu *toolsMenu = menuBar->addMenu(tr("工具(&T)")); // 添加工具菜单
    QAction *configPreviewModeAction = toolsMenu->addAction(tr("配置预览方式")); // 配置预览方式动作

    QMenu *windowMenu = menuBar->addMenu(tr("窗口(&W)")); // 添加窗口菜单
    QAction *tileHorizontallyAction = windowMenu->addAction(tr("横向平铺")); // 横向平铺动作
    QAction *tileVerticallyAction = windowMenu->addAction(tr("纵向平铺")); // 纵向平铺动作
    QAction *cascadeAction = windowMenu->addAction(tr("层叠")); // 层叠动作
    QAction *arrangeIconsAction = windowMenu->addAction(tr("排列图标")); // 排列图标动作
    QAction *otherWindowsAction = windowMenu->addAction(tr("其他窗口")); // 其他窗口动作

    QMenu *testMenu = menuBar->addMenu(tr("测点")); // 添加测点菜单
    QAction *showDefaultValueAction = testMenu->addAction(tr("测点显示默认值")); // 测点默认值动作

    QMenu *helpMenu = menuBar->addMenu(tr("帮助(&H)")); // 添加帮助菜单
    QAction *aboutAction = helpMenu->addAction(tr("关于(&A)")); // 关于动作
    QAction *changelogAction = helpMenu->addAction(tr("更新日志")); // 更新日志动作

    connect(closeAction, &QAction::triggered, this, &QMainWindow::close); // 连接关闭动作
    connect(graphManagementAction, &QAction::triggered, this, &MainWindow::graphManagementWindow); // 连接图管理动作
    connect(newAction, &QAction::triggered, this, &MainWindow::newFileWindow); // 连接新建动作
    connect(copyAction, &QAction::triggered, graphicsView, &GraphicsToolView::copySelectedItems); // 连接复制动作
    connect(pasteAction, &QAction::triggered, graphicsView, &GraphicsToolView::pasteCopiedItems); // 连接粘贴动作
}

MainWindow::~MainWindow()
{
    delete ui; // 清理UI资源
}
