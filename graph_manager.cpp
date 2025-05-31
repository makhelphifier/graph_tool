#include "graph_manager.h"

GraphManager::GraphManager(QWidget *parent)
    : QMainWindow{parent}
{


    setWindowTitle(tr("图管理"));
    resize(400,300);

    // 主布局：使用 QSplitter 分割左右两部分
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    // 左侧树状结构
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeader(new QHeaderView(Qt::Horizontal, treeWidget));
    treeWidget->header()->setFixedHeight(0); // 将标题栏高度设为0
    treeWidget->header()->setVisible(false); // 确保标题栏不可见
    splitter->addWidget(treeWidget);


    // 加载保存的数据
    loadTreeFromFile();
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeWidget,&QTreeWidget::customContextMenuRequested,this,&GraphManager::showContextMenu);

    // 设置分隔比例
    splitter->setSizes(QList<int>() << 200 << 600);



}

GraphManager::~GraphManager()
{
    saveTreeToFile();
}


void GraphManager::showContextMenu(const QPoint &pos) {

    QTreeWidgetItem *item = treeWidget->itemAt(pos);
    if(!item) return;

    QMenu contextMenu(tr("右键菜单"),this);
    QAction *addChildAction = new QAction(tr("新建图形类别"), this);
    QAction *editChildAction = new QAction(tr("编辑图形类别"), this);
    QAction *deleteChildAction = new QAction(tr("删除图形类别"), this);
    connect(addChildAction, &QAction::triggered, this, [=]() { addChildItem(item); });
    connect(editChildAction, &QAction::triggered, this, [=]() { editItem(item); });
    connect(deleteChildAction, &QAction::triggered, this, [=]() { deleteItem(item); });
    contextMenu.addAction(addChildAction);
    contextMenu.addAction(editChildAction);
    contextMenu.addAction(deleteChildAction);
    contextMenu.exec(treeWidget->mapToGlobal(pos));

}


// 新建图形类别
void GraphManager::addChildItem(QTreeWidgetItem *parentItem) {
    // 创建新项
    QTreeWidgetItem *newItem = new QTreeWidgetItem(parentItem);
    newItem->setText(0, tr("新图形类别"));
    newItem->setFlags(newItem->flags() | Qt::ItemIsEditable); // 允许编辑

    // 展开父节点并选中新项
    treeWidget->expandItem(parentItem);
    treeWidget->setCurrentItem(newItem);
    treeWidget->editItem(newItem); // 进入编辑状态
    saveTreeToFile(); // 保存更改
}

void GraphManager::editItem(QTreeWidgetItem *item) {
    if (!item) {
        qDebug() << "Edit failed: No item provided";
        return;
    }
    // Ensure the item is editable
    if (!(item->flags() & Qt::ItemIsEditable)) {
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        qDebug() << "Set item editable for:" << item->text(0);
    }
    treeWidget->setFocus();
    treeWidget->setCurrentItem(item);
    treeWidget->editItem(item);

    // 连接 itemChanged 信号以在编辑完成时保存
    connect(treeWidget, &QTreeWidget::itemChanged, this, [=]() {
        saveTreeToFile();
    });
}
// 删除图形类别
void GraphManager::deleteItem(QTreeWidgetItem *item) {
    if (!item) return;

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
            parent->removeChild(item);
        } else {
            int index = treeWidget->indexOfTopLevelItem(item);
            treeWidget->takeTopLevelItem(index);
        }
        delete item; // 释放内存
        saveTreeToFile(); // 保存更改
    }
}

QString GraphManager::getSaveFilePath() const {
    // 使用标准路径存储文件，保存在用户数据目录
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return path + "/graph_categories.json";
}


void GraphManager::saveTreeToFile() {
    QJsonArray treeArray;

    // 遍历顶级项
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *topItem = treeWidget->topLevelItem(i);
        treeArray.append(itemToJson(topItem));
    }

    // 写入 JSON 文件
    QJsonDocument doc(treeArray);
    QFile file(getSaveFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "树数据已保存到：" << getSaveFilePath();
    } else {
        qDebug() << "无法保存树数据：" << file.errorString();
    }
}

QJsonObject GraphManager::itemToJson(QTreeWidgetItem *item) const {
    QJsonObject obj;
    obj["name"] = item->text(0);
    QJsonArray children;
    for (int i = 0; i < item->childCount(); ++i) {
        children.append(itemToJson(item->child(i)));
    }
    obj["children"] = children;
    return obj;
}

void GraphManager::loadTreeFromFile() {
    QFile file(getSaveFilePath());
    if (!file.exists() || file.size() == 0) {
        qDebug() << "无保存数据，初始化默认树：" << getSaveFilePath();
        initializeDefaultTree();
        return;
    }


    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        treeWidget->clear(); // 清空当前树
        QJsonArray treeArray = doc.array();
        for (const QJsonValue &value : treeArray) {
            QTreeWidgetItem *item = jsonToItem(value.toObject());
            treeWidget->addTopLevelItem(item);
        }
        qDebug() << "树数据已加载从：" << getSaveFilePath();
    } else {
        qDebug() << "无法加载树数据：" << file.errorString();
        initializeDefaultTree(); // 加载失败时也初始化默认树
    }
}

void GraphManager::initializeDefaultTree() {
    // 图形
    QTreeWidgetItem *graphItem = new QTreeWidgetItem(treeWidget, QStringList(tr("图形")));
    graphItem->setFlags(graphItem->flags() | Qt::ItemIsEditable);
    QTreeWidgetItem *publicGraph = new QTreeWidgetItem(graphItem, QStringList(tr("公用")));
    publicGraph->setFlags(publicGraph->flags() | Qt::ItemIsEditable);
    QTreeWidgetItem *jiangsuGraph = new QTreeWidgetItem(graphItem, QStringList(tr("江苏")));
    jiangsuGraph->setFlags(jiangsuGraph->flags() | Qt::ItemIsEditable);

    // 图元
    QTreeWidgetItem *customItem = new QTreeWidgetItem(treeWidget, QStringList(tr("图元")));
    customItem->setFlags(customItem->flags() | Qt::ItemIsEditable);
    QTreeWidgetItem *animationElement = new QTreeWidgetItem(customItem, QStringList(tr("动画元素")));
    animationElement->setFlags(animationElement->flags() | Qt::ItemIsEditable);
    QTreeWidgetItem *publicElement = new QTreeWidgetItem(customItem, QStringList(tr("公用")));
    publicElement->setFlags(publicElement->flags() | Qt::ItemIsEditable);

    // 模板
    QTreeWidgetItem *templateItem = new QTreeWidgetItem(treeWidget, QStringList(tr("模板")));
    templateItem->setFlags(templateItem->flags() | Qt::ItemIsEditable);
    QTreeWidgetItem *publicTemplate = new QTreeWidgetItem(templateItem, QStringList(tr("公用模板")));
    publicTemplate->setFlags(publicTemplate->flags() | Qt::ItemIsEditable);

    // 图块
    QTreeWidgetItem *graphPieceItem = new QTreeWidgetItem(treeWidget, QStringList(tr("图块")));
    graphPieceItem->setFlags(graphPieceItem->flags() | Qt::ItemIsEditable);
    QTreeWidgetItem *publicGraphPiece = new QTreeWidgetItem(graphPieceItem, QStringList(tr("公用图块")));
    publicGraphPiece->setFlags(publicGraphPiece->flags() | Qt::ItemIsEditable);

    // 保存初始化的树
    saveTreeToFile();
}

QTreeWidget *GraphManager::getTreeWidget()
{
    return treeWidget;
}


QTreeWidgetItem *GraphManager::jsonToItem(const QJsonObject &obj) const {
    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, obj["name"].toString());
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    QJsonArray children = obj["children"].toArray();
    for (const QJsonValue &childValue : children) {
        QTreeWidgetItem *childItem = jsonToItem(childValue.toObject());
        item->addChild(childItem);
    }
    return item;
}



