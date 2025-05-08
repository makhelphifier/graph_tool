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
    explicit GraphManager(QWidget *parent = nullptr);
    ~GraphManager(); // 添加析构函数以确保保存
    QTreeWidgetItem *jsonToItem(const QJsonObject &obj) const;
    QJsonObject itemToJson(QTreeWidgetItem *item) const;
    void saveTreeToFile(); // 保存树到文件
    void loadTreeFromFile(); // 从文件加载树
    QString getSaveFilePath() const; // 获取保存文件路径
    void initializeDefaultTree();
    QTreeWidget* getTreeWidget();
public slots:
    void showContextMenu(const QPoint &pos);
    void addChildItem(QTreeWidgetItem *parentItem);
    void editItem(QTreeWidgetItem *item);
    void deleteItem(QTreeWidgetItem *item);
private slots:
    // void GraphManager::showContextMenu(const QPoint &pos) ;

private:
    QTreeWidget *treeWidget ;

signals:
};

#endif // GRAPHMANAGER_H
