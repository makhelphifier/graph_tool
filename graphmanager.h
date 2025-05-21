#ifndef GRAPHMANAGER_H
#define GRAPHMANAGER_H

#include <QMainWindow>
#include <QSplitter>
#include <QTreeWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QPushButton>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

class GraphManager : public QMainWindow
{
    Q_OBJECT
public:
    explicit GraphManager(QWidget *parent = nullptr); // 构造函数
    ~GraphManager(); // 析构函数，确保保存
    QTreeWidgetItem *jsonToItem(const QJsonObject &obj) const; // JSON转树项
    QJsonObject itemToJson(QTreeWidgetItem *item) const; // 树项转JSON
    void saveTreeToFile(); // 保存树到文件
    void loadTreeFromFile(); // 从文件加载树
    QString getSaveFilePath() const; // 获取保存文件路径
    void initializeDefaultTree(); // 初始化默认树
    QTreeWidget* getTreeWidget(); // 获取树控件

public slots:
    void showContextMenu(const QPoint &pos); // 显示右键菜单
    void addChildItem(QTreeWidgetItem *parentItem); // 添加子项
    void editItem(QTreeWidgetItem *item); // 编辑项
    void deleteItem(QTreeWidgetItem *item); // 删除项

private slots:
               // void GraphManager::showContextMenu(const QPoint &pos); // 右键菜单槽（注释）

private:
    QTreeWidget *treeWidget; // 树控件

signals:
};

#endif // GRAPHMANAGER_H
