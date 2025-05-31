#ifndef NEW_IMAGE_DIALOG_H
#define NEW_IMAGE_DIALOG_H

#include <QDialog>
#include <QMainWindow>
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QListWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
class NewImageDialog : public QDialog
{
    Q_OBJECT
public:
    NewImageDialog(QWidget *parent = nullptr);
    void loadTreeFromFile(); // 从文件加载树

    void initializeDefaultTree();
    QString getSaveFilePath() const;
    void addNewImage() {
        // 添加新图片名
        QString imageName = QString("image%1.png").arg(imageCount++);
        imageList->addItem(imageName);
    }
signals:
    void imageCreated(const QString &imageName); // 新建图片信号
private:
    QTreeWidget *treeWidget ;
    QListWidget *imageList;
    int imageCount;
};

#endif // NEW_IMAGE_DIALOG_H
