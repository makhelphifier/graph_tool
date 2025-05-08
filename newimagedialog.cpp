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

NewImageDialog::NewImageDialog(QWidget *parent ) :QDialog(parent){
    setWindowTitle("新建图片窗口");
    resize(600, 400);

    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    GraphManager * graphManager = new GraphManager(this);
    // 加载保存的数据
    treeWidget = graphManager->getTreeWidget();
    // 左侧树状结构
    treeWidget->setHeader(new QHeaderView(Qt::Horizontal, treeWidget));
    treeWidget->header()->setFixedHeight(0); // 将标题栏高度设为0
    treeWidget->header()->setVisible(false); // 确保标题栏不可见
    treeWidget->expandAll();
    splitter->addWidget(treeWidget);
    // 右边：图片名列表 + 输入框 + 按钮
    QWidget *rightWidget = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);

    // 图片名列表（两列布局）
    imageList = new QListWidget();
    imageList->setViewMode(QListWidget::ListMode);
    imageList->setFlow(QListWidget::LeftToRight); // 水平填充，从左到右
    imageList->setWrapping(true);
    imageList->setResizeMode(QListWidget::Fixed); // 固定模式，避免自动调整
    imageList->setGridSize(QSize(100, 50)); // 设置每个项目的网格大小，控制列宽
    imageList->setMinimumWidth(200); // 确保宽度足够显示两列
    rightLayout->addWidget(imageList, 1); // 拉伸因子 1，占满空间

    // 输入框
    QLineEdit *imageNameInput = new QLineEdit();
    imageNameInput->setPlaceholderText("输入图名称");
    rightLayout->addWidget(imageNameInput);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *newImageButton = new QPushButton("新建");
    QPushButton *cancelButton = new QPushButton("取消");
    buttonLayout->addWidget(newImageButton);
    buttonLayout->addWidget(cancelButton);
    rightLayout->addLayout(buttonLayout);

    splitter->addWidget(rightWidget);
    mainLayout->addWidget(splitter);

    // 连接信号
    connect(newImageButton, &QPushButton::clicked, this, [=]() {
        QString imageName = imageNameInput->text().trimmed();
        if (imageName.isEmpty()) {
            imageName = QString("image%1.png").arg(imageCount++);
        }
        imageList->addItem(imageName);
        imageNameInput->clear(); // 清空输入框
        emit imageCreated(imageName); // 发出信号
    });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}





