#ifndef BACKGROUND_IMAGE_SELECTOR_DIALOG_H
#define BACKGROUND_IMAGE_SELECTOR_DIALOG_H

#include <QDialog>
#include <QObject>
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
class BackgroundImageSelectorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BackgroundImageSelectorDialog(QWidget *parent = nullptr);

signals:
    void imageSelected(const QString &imagePath); // 发出选中图片的路径


private slots:
    void onUploadButtonClicked(); // 处理上传图片
    void onOkButtonClicked(); // 处理确定按钮
    void onItemClicked(QListWidgetItem *item); // 处理图片项点击

private:
    QListWidget *imageListWidget; // 图片列表
    QPushButton *uploadButton; // 上传按钮
    QPushButton *okButton; // 确定按钮
    QPushButton *cancelButton; // 取消按钮
    QString selectedImagePath; // 当前选中的图片路径
};

#endif // BACKGROUND_IMAGE_SELECTOR_DIALOG_H
