#include "newimagedialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include "graphmanager.h"

NewImageDialog::NewImageDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("新建图片窗口"); // 设置窗口标题
    resize(600, 400); // 设置窗口大小

    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    GraphManager *graphManager = new GraphManager(this); // 创建图形管理器
    // 加载保存的数据
    treeWidget = graphManager->getTreeWidget(); // 获取树控件
    // 左侧树状结构
    treeWidget->setHeader(new QHeaderView(Qt::Horizontal, treeWidget)); // 设置头部
    treeWidget->header()->setFixedHeight(0); // 隐藏标题栏高度
    treeWidget->header()->setVisible(false); // 确保标题栏不可见
    treeWidget->expandAll(); // 展开所有节点
    splitter->addWidget(treeWidget); // 添加树控件到分割器

    // 右边：图片名列表 + 输入框 + 按钮
    QWidget *rightWidget = new QWidget(); // 创建右侧部件
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget); // 创建右侧垂直布局

    // 图片名列表（两列布局）
    imageList = new QListWidget(); // 创建图片列表
    imageList->setViewMode(QListWidget::ListMode); // 设置视图模式
    imageList->setFlow(QListWidget::LeftToRight); // 水平填充，从左到右
    imageList->setWrapping(true); // 自动换行
    imageList->setResizeMode(QListWidget::Fixed); // 固定模式，避免自动调整
    imageList->setGridSize(QSize(100, 50)); // 设置网格大小，控制列宽
    imageList->setMinimumWidth(200); // 确保宽度足够显示两列
    rightLayout->addWidget(imageList, 1); // 添加列表，占满空间

    // 输入框
    QLineEdit *imageNameInput = new QLineEdit(); // 创建输入框
    imageNameInput->setPlaceholderText("输入图名称"); // 设置占位符文本
    rightLayout->addWidget(imageNameInput); // 添加输入框到布局

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout(); // 创建按钮水平布局
    QPushButton *newImageButton = new QPushButton("新建"); // 新建按钮
    QPushButton *cancelButton = new QPushButton("取消"); // 取消按钮
    buttonLayout->addWidget(newImageButton); // 添加新建按钮
    buttonLayout->addWidget(cancelButton); // 添加取消按钮
    rightLayout->addLayout(buttonLayout); // 添加按钮布局到右侧布局

    splitter->addWidget(rightWidget); // 添加右侧部件到分割器
    mainLayout->addWidget(splitter); // 添加分割器到主布局

    // 连接信号
    connect(newImageButton, &QPushButton::clicked, this, [=]() {
        QString imageName = imageNameInput->text().trimmed(); // 获取输入的图片名称
        if (imageName.isEmpty()) {
            imageName = QString("image%1.png").arg(imageCount++); // 如果为空，生成默认名称
        }
        imageList->addItem(imageName); // 添加到列表
        imageNameInput->clear(); // 清空输入框
        emit imageCreated(imageName); // 发出图片创建信号
    });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject); // 连接取消按钮信号
}
