#ifndef NEWIMAGEDIALOG_H
#define NEWIMAGEDIALOG_H

#include <QDialog>
#include <QMainWindow>
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>

class NewImageDialog : public QDialog
{
    Q_OBJECT
public:
    NewImageDialog(QWidget *parent = nullptr); // 构造函数
    void loadTreeFromFile(); // 从文件加载树
    void initializeDefaultTree(); // 初始化默认树
    QString getSaveFilePath() const; // 获取保存文件路径
    void addNewImage() { // 添加新图片
        QString imageName = QString("image%1.png").arg(imageCount++); // 生成图片名
        imageList->addItem(imageName); // 添加到列表
    }

signals:
    void imageCreated(const QString &imageName); // 新建图片信号

private:
    QTreeWidget *treeWidget; // 树控件
    QListWidget *imageList; // 图片列表
    int imageCount; // 图片计数
};

#endif // NEWIMAGEDIALOG_H
