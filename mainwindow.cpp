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
    , scene(new QGraphicsScene(this))
    , graphicsView(new GraphicsToolView(scene,this))
{
    // ui->setupUi(this);

    scene->setSceneRect(-1000, -1000, 2000, 2000);

    initMenu();

    resize(800, 600);

    graphicsView->setRenderHint(QPainter::Antialiasing);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(graphicsView, 1);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    QToolBar *toolBar = addToolBar("Tools");

    QPushButton *drawTextButton = new QPushButton("输入文字");
    toolBar->addWidget(drawTextButton);
    connect(drawTextButton, &QPushButton::clicked, this, [=]() {
        imageCanvas->setDrawingMode(ImageCanvas::DrawingMode::Text);
    });

    QPushButton *drawLineButton = new QPushButton("画线段");
    toolBar->addWidget(drawLineButton);
    connect(drawLineButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Line);
    });

    QPushButton *drawPolylineButton = new QPushButton("画折线");
    toolBar->addWidget(drawPolylineButton);
    connect(drawPolylineButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::Polyline);
    });
    QPushButton *selectButton = new QPushButton("取消绘制");
    toolBar->addWidget(selectButton);
    connect(selectButton, &QPushButton::clicked, this, [=]() {
        graphicsView->setDrawingMode(GraphicsToolView::DrawingMode::None);
    });

    QPushButton *drawRectangleButton = new QPushButton("画矩形");
    toolBar->addWidget(drawRectangleButton);
    connect(drawRectangleButton, &QPushButton::clicked, this, [=]() {
        imageCanvas->setDrawingMode(ImageCanvas::DrawingMode::Rectangle);
    });

    QPushButton *drawEllipseButton = new QPushButton("画椭圆");
    toolBar->addWidget(drawEllipseButton);
    connect(drawEllipseButton, &QPushButton::clicked, this, [=]() {
        imageCanvas->setDrawingMode(ImageCanvas::DrawingMode::Ellipse);
    });

    QPushButton *drawArcButton = new QPushButton("画圆弧");
    toolBar->addWidget(drawArcButton);
    connect(drawArcButton, &QPushButton::clicked, this, [=]() {
        imageCanvas->setDrawingMode(ImageCanvas::DrawingMode::Arc);
    });

    QPushButton *drawPolygonButton = new QPushButton("画多边形");
    toolBar->addWidget(drawPolygonButton);
    connect(drawPolygonButton, &QPushButton::clicked, this, [=]() {
        imageCanvas->setDrawingMode(ImageCanvas::DrawingMode::Polygon);
    });

    lineColorButton = new QToolButton(this);
    lineColorButton->setText(QStringLiteral("线条颜色"));
    lineColorButton->setToolTip(QStringLiteral("选择线条颜色"));
    lineColorButton->setPopupMode(QToolButton::InstantPopup);
    lineColorButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    updateLineColorButtonIcon();
    toolBar->addWidget(lineColorButton);

    colorPopup = new ColorSelectorPopup(this);
    connect(colorPopup, &ColorSelectorPopup::colorSelected, this, &MainWindow::onColorSelected);
    connect(colorPopup, &ColorSelectorPopup::closePopup, this, &MainWindow::closeColorPopup);
    // colorPopup = new ColorSelectorPopup(this);
    // connect(colorPopup, &ColorSelectorPopup::colorSelected, this, &GraphicsToolView::onColorSelected);
    // connect(colorPopup, &ColorSelectorPopup::closePopup, colorPopup, &ColorSelectorPopup::close);

    QMenu *colorMenu = new QMenu(lineColorButton);
    QWidgetAction *colorWidgetAction = new QWidgetAction(colorMenu);
    colorWidgetAction->setDefaultWidget(colorPopup);
    colorMenu->addAction(colorWidgetAction);

    lineColorButton->setMenu(colorMenu);
}

void MainWindow::onColorSelected(const QColor &color)
{
    if (color.isValid()) {
        currentLineColor = color;
        qDebug() << "MainWindow: Color selected -" << color.name();
        if(graphicsView) {
            graphicsView->setDrawingColor(currentLineColor);
        }
        updateLineColorButtonIcon();
    }
}

void MainWindow::closeColorPopup()
{
    if (lineColorButton && lineColorButton->menu()) {
        lineColorButton->menu()->hide();
    }
}

void MainWindow::updateLineColorButtonIcon()
{
    if (!lineColorButton) return;

    int iconSize = 16;
    QPixmap pixmap(iconSize, iconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(currentLineColor);
    painter.setPen(Qt::gray);
    painter.drawRect(0, 0, iconSize - 1, iconSize - 1);

    lineColorButton->setIcon(QIcon(pixmap));
}

void MainWindow::graphManagementWindow() {
    GraphManager *newWindow = new GraphManager(this);
    newWindow->show();
}
void MainWindow::newFileWindow() {
    NewImageDialog *newWindow = new NewImageDialog(this);
    newWindow->show();
    connect(newWindow, &NewImageDialog::imageCreated, this, &MainWindow::displayNewImage);
}

void MainWindow::displayNewImage(const QString &imageName) {
    QPixmap blankImage(800, 600);
    blankImage.fill(Qt::white);
    imageCanvas->setImage(blankImage);
}

void MainWindow::initMenu(){
    QMenuBar *menuBar = this->menuBar();
    QMenu *fileMenu = menuBar->addMenu(tr("文件(&F)"));
    QAction *newAction = fileMenu->addAction(tr("新建(&N)"));newAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    QAction *openAction = fileMenu->addAction(tr("打开(&O)"));openAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    QAction *saveAction = fileMenu->addAction(tr("保存"));saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    QAction *exportAction = fileMenu->addAction(tr("导出..."));exportAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    QAction *importAction = fileMenu->addAction(tr("导入..."));importAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    fileMenu->addSeparator();
    QAction *graphManagementAction = fileMenu->addAction(tr("图管理(&T)..."));graphManagementAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    QAction *symboManagementlAction = fileMenu->addAction(tr("符号管理(&M)..."));symboManagementlAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    fileMenu->addSeparator();
    QAction *pritAction = fileMenu->addAction(tr("打印(&P)..."));pritAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    QAction *printViewAction = fileMenu->addAction(tr("打印预览(&V)"));
    QAction *pageSettingAction = fileMenu->addAction(tr("页面设置...(&U)"));
    fileMenu->addSeparator();
    QAction *closeAction = fileMenu->addAction(tr("关闭(&X)"));

    QMenu *editMenu = menuBar->addMenu(tr("编辑(&E)"));
    QAction *undoAction = editMenu->addAction(tr("撤销(&U)"));
    QAction *redoAction = editMenu->addAction(tr("重做(&R)"));
    fileMenu->addSeparator();
    QAction *cutAction = editMenu->addAction(tr("剪切(&T)"));cutAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X));
    QAction *copyAction = editMenu->addAction(tr("复制(&C)"));copyAction->setShortcut(QKeySequence(Qt::CTRL  | Qt::Key_C));
    QAction *pasteAction = editMenu->addAction(tr("粘贴(&P)"));pasteAction->setShortcut(QKeySequence(Qt::CTRL  | Qt::Key_V));
    QAction *deleteAction = editMenu->addAction(tr("删除(&D)"));deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));
    QAction *selectAllAction = editMenu->addAction(tr("全选(&A)"));selectAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));
    QMenu *viewMenu = menuBar->addMenu(tr("视窗(&V)"));
    QAction *markToolRefAction = viewMenu->addAction(tr("标准工具条"));
    QAction *iconMarkToolRefAction = viewMenu->addAction(tr("图形编辑工具条"));
    QAction *forToolRefAction = viewMenu->addAction(tr("对齐工具条"));
    QAction *fontToolRefAction = viewMenu->addAction(tr("字体工具条"));
    QAction *lineToolRefAction = viewMenu->addAction(tr("线形工具条"));
    QAction *iconManageToolRefAction = viewMenu->addAction(tr("图层管理工具条"));
    QAction *runToolRefAction = viewMenu->addAction(tr("运行工具条"));
    QAction *statusBarAction = viewMenu->addAction(tr("状态条"));
    QAction *gridAction = viewMenu->addAction(tr("网格"));
    QAction *drawResourceExplorerAction = viewMenu->addAction(tr("绘图资源管理器"));
    QMenu *toolsMenu = menuBar->addMenu(tr("工具(&T)"));
    QAction *configPreviewModeAction = toolsMenu->addAction(tr("配置预览方式"));
    QMenu *windowMenu = menuBar->addMenu(tr("窗口(&W)"));
    QAction *tileHorizontallyAction = windowMenu->addAction(tr("横向平铺"));
    QAction *tileVerticallyAction = windowMenu->addAction(tr("纵向平铺"));
    QAction *cascadeAction = windowMenu->addAction(tr("层叠"));
    QAction *arrangeIconsAction = windowMenu->addAction(tr("排列图标"));
    QAction *otherWindowsAction = windowMenu->addAction(tr("其他窗口"));
    QMenu *testMenu = menuBar->addMenu(tr("测点"));
    QAction *showDefaultValueAction = testMenu->addAction(tr("测点显示默认值"));
    QMenu *helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    QAction *aboutAction = helpMenu->addAction(tr("关于(&A)"));
    QAction *changelogAction = helpMenu->addAction(tr("更新日志"));

    connect(closeAction,&QAction::triggered,this,&QMainWindow::close);

    connect(graphManagementAction, &QAction::triggered, this, &MainWindow::graphManagementWindow);
    connect(newAction, &QAction::triggered, this, &MainWindow::newFileWindow);

    // 添加连接：将复制动作的 triggered 信号连接到 graphicsView 的 copySelectedItems 槽
    connect(copyAction, &QAction::triggered, graphicsView, &GraphicsToolView::copySelectedItems);
    // 添加连接：将粘贴动作的 triggered 信号连接到 graphicsView 的 pasteCopiedItems 槽
    connect(pasteAction, &QAction::triggered, graphicsView, &GraphicsToolView::pasteCopiedItems);

}

MainWindow::~MainWindow()
{
    delete ui;
}
