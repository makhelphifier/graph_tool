#include "graphmanager.h"

GraphManager::GraphManager(QWidget *parent)
    : QMainWindow{parent}
{
    setWindowTitle(tr("图管理")); // 设置窗口标题
    resize(400, 300); // 设置窗口大小

    // 主布局：使用 QSplitter 分割左右两部分
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter); // 设置中心部件

    // 左侧树状结构
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeader(new QHeaderView(Qt::Horizontal, treeWidget)); // 设置头部
    treeWidget->header()->setFixedHeight(0); // 隐藏标题栏高度
    treeWidget->header()->setVisible(false); // 确保标题栏不可见
    splitter->addWidget(treeWidget); // 添加树控件到分割器

    // 加载保存的数据
    loadTreeFromFile(); // 从文件加载树数据
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu); // 设置右键菜单策略
    connect(treeWidget, &QTreeWidget::customContextMenuRequested, this, &GraphManager::showContextMenu); // 连接右键菜单信号

    // 设置分隔比例
    splitter->setSizes(QList<int>() << 200 << 600); // 设置左右比例
}

GraphManager::~GraphManager()
{
    saveTreeToFile(); // 析构时保存树数据
}

void GraphManager::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = treeWidget->itemAt(pos); // 获取点击项
    if (!item) return; // 未点击项则返回

    QMenu contextMenu(tr("右键菜单"), this); // 创建右键菜单
    QAction *addChildAction = new QAction(tr("新建图形类别"), this); // 新建类别动作
    QAction *editChildAction = new QAction(tr("编辑图形类别"), this); // 编辑类别动作
    QAction *deleteChildAction = new QAction(tr("删除图形类别"), this); // 删除类别动作
    connect(addChildAction, &QAction::triggered, this, [=]() { addChildItem(item); }); // 连接新建动作
    connect(editChildAction, &QAction::triggered, this, [=]() { editItem(item); }); // 连接编辑动作
    connect(deleteChildAction, &QAction::triggered, this, [=]() { deleteItem(item); }); // 连接删除动作
    contextMenu.addAction(addChildAction); // 添加动作到菜单
    contextMenu.addAction(editChildAction); // 添加动作到菜单
    contextMenu.addAction(deleteChildAction); // 添加动作到菜单
    contextMenu.exec(treeWidget->mapToGlobal(pos)); // 显示菜单
}

// 新建图形类别
void GraphManager::addChildItem(QTreeWidgetItem *parentItem)
{
    QTreeWidgetItem *newItem = new QTreeWidgetItem(parentItem); // 创建新项
    newItem->setText(0, tr("新图形类别")); // 设置文本
    newItem->setFlags(newItem->flags() | Qt::ItemIsEditable); // 允许编辑

    treeWidget->expandItem(parentItem); // 展开父节点
    treeWidget->setCurrentItem(newItem); // 选中新项
    treeWidget->editItem(newItem); // 进入编辑状态
    saveTreeToFile(); // 保存更改
}

void GraphManager::editItem(QTreeWidgetItem *item)
{
    if (!item) {
        qDebug() << "编辑失败：未提供项"; // 打印编辑失败信息
        return;
    }
    if (!(item->flags() & Qt::ItemIsEditable)) {
        item->setFlags(item->flags() | Qt::ItemIsEditable); // 设置可编辑
        qDebug() << "设置项可编辑：" << item->text(0); // 打印设置信息
    }
    treeWidget->setFocus(); // 设置焦点
    treeWidget->setCurrentItem(item); // 选中项
    treeWidget->editItem(item); // 进入编辑状态

    // 连接编辑完成信号以保存
    connect(treeWidget, &QTreeWidget::itemChanged, this, [=]() {
        saveTreeToFile(); // 保存更改
    });
}

// 删除图形类别
void GraphManager::deleteItem(QTreeWidgetItem *item)
{
    if (!item) return; // 未提供项则返回

    // 弹出确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("删除确认"),
        tr("确定要删除图形类别 '%1' 及其所有子类别吗？").arg(item->text(0)),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        // 从父节点或树中删除
        if (QTreeWidgetItem *parent = item->parent()) {
            parent->removeChild(item); // 从父节点移除
        } else {
            int index = treeWidget->indexOfTopLevelItem(item); // 获取顶级项索引
            treeWidget->takeTopLevelItem(index); // 移除顶级项
        }
        delete item; // 释放内存
        saveTreeToFile(); // 保存更改
    }
}

QString GraphManager::getSaveFilePath() const
{
    // 使用标准路径存储文件
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath("."); // 创建目录
    }
    return path + "/graph_categories.json"; // 返回保存路径
}

void GraphManager::saveTreeToFile()
{
    QJsonArray treeArray; // 树数据数组

    // 遍历顶级项
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *topItem = treeWidget->topLevelItem(i);
        treeArray.append(itemToJson(topItem)); // 转换并添加顶级项
    }

    // 写入 JSON 文件
    QJsonDocument doc(treeArray);
    QFile file(getSaveFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson()); // 写入数据
        file.close();
        qDebug() << "树数据已保存到：" << getSaveFilePath(); // 打印保存信息
    } else {
        qDebug() << "无法保存树数据：" << file.errorString(); // 打印错误信息
    }
}

QJsonObject GraphManager::itemToJson(QTreeWidgetItem *item) const
{
    QJsonObject obj; // JSON对象
    obj["name"] = item->text(0); // 设置名称
    QJsonArray children; // 子项数组
    for (int i = 0; i < item->childCount(); ++i) {
        children.append(itemToJson(item->child(i))); // 递归转换子项
    }
    obj["children"] = children; // 设置子项
    return obj; // 返回对象
}

void GraphManager::loadTreeFromFile()
{
    QFile file(getSaveFilePath()); // 获取保存文件
    if (!file.exists() || file.size() == 0) {
        qDebug() << "无保存数据，初始化默认树：" << getSaveFilePath(); // 打印无数据信息
        initializeDefaultTree(); // 初始化默认树
        return;
    }

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll()); // 读取JSON数据
        file.close();

        treeWidget->clear(); // 清空当前树
        QJsonArray treeArray = doc.array(); // 获取树数组
        for (const QJsonValue &value : treeArray) {
            QTreeWidgetItem *item = jsonToItem(value.toObject()); // 转换JSON为项
            treeWidget->addTopLevelItem(item); // 添加顶级项
        }
        qDebug() << "树数据已加载从：" << getSaveFilePath(); // 打印加载信息
    } else {
        qDebug() << "无法加载树数据：" << file.errorString(); // 打印错误信息
        initializeDefaultTree(); // 加载失败时初始化默认树
    }
}

void GraphManager::initializeDefaultTree()
{
    // 图形
    QTreeWidgetItem *graphItem = new QTreeWidgetItem(treeWidget, QStringList(tr("图形")));
    graphItem->setFlags(graphItem->flags() | Qt::ItemIsEditable); // 允许编辑
    QTreeWidgetItem *publicGraph = new QTreeWidgetItem(graphItem, QStringList(tr("公用")));
    publicGraph->setFlags(publicGraph->flags() | Qt::ItemIsEditable); // 允许编辑
    QTreeWidgetItem *jiangsuGraph = new QTreeWidgetItem(graphItem, QStringList(tr("江苏")));
    jiangsuGraph->setFlags(jiangsuGraph->flags() | Qt::ItemIsEditable); // 允许编辑

    // 图元
    QTreeWidgetItem *customItem = new QTreeWidgetItem(treeWidget, QStringList(tr("图元")));
    customItem->setFlags(customItem->flags() | Qt::ItemIsEditable); // 允许编辑
    QTreeWidgetItem *animationElement = new QTreeWidgetItem(customItem, QStringList(tr("动画元素")));
    animationElement->setFlags(animationElement->flags() | Qt::ItemIsEditable); // 允许编辑
    QTreeWidgetItem *publicElement = new QTreeWidgetItem(customItem, QStringList(tr("公用")));
    publicElement->setFlags(publicElement->flags() | Qt::ItemIsEditable); // 允许编辑

    // 模板
    QTreeWidgetItem *templateItem = new QTreeWidgetItem(treeWidget, QStringList(tr("模板")));
    templateItem->setFlags(templateItem->flags() | Qt::ItemIsEditable); // 允许编辑
    QTreeWidgetItem *publicTemplate = new QTreeWidgetItem(templateItem, QStringList(tr("公用模板")));
    publicTemplate->setFlags(publicTemplate->flags() | Qt::ItemIsEditable); // 允许编辑

    // 图块
    QTreeWidgetItem *graphPieceItem = new QTreeWidgetItem(treeWidget, QStringList(tr("图块")));
    graphPieceItem->setFlags(graphPieceItem->flags() | Qt::ItemIsEditable); // 允许编辑
    QTreeWidgetItem *publicGraphPiece = new QTreeWidgetItem(graphPieceItem, QStringList(tr("公用图块")));
    publicGraphPiece->setFlags(publicGraphPiece->flags() | Qt::ItemIsEditable); // 允许编辑

    saveTreeToFile(); // 保存初始化树
}

QTreeWidget *GraphManager::getTreeWidget()
{
    return treeWidget; // 返回树控件
}

QTreeWidgetItem *GraphManager::jsonToItem(const QJsonObject &obj) const
{
    QTreeWidgetItem *item = new QTreeWidgetItem; // 创建新项
    item->setText(0, obj["name"].toString()); // 设置文本
    item->setFlags(item->flags() | Qt::ItemIsEditable); // 允许编辑

    QJsonArray children = obj["children"].toArray(); // 获取子项数组
    for (const QJsonValue &childValue : children) {
        QTreeWidgetItem *childItem = jsonToItem(childValue.toObject()); // 递归转换子项
        item->addChild(childItem); // 添加子项
    }
    return item; // 返回项
}
