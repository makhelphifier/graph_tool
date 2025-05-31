#include "background_image_selector_dialog.h"
#include <QFileDialog>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QSettings>

BackgroundImageSelectorDialog::BackgroundImageSelectorDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("选择背景图片"));
    setMinimumSize(400, 300);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 图片列表
    imageListWidget = new QListWidget(this);
    imageListWidget->setViewMode(QListWidget::IconMode); // 图标模式显示缩略图
    imageListWidget->setIconSize(QSize(100, 100)); // 设置缩略图大小
    imageListWidget->setSpacing(10); // 设置项间距
    connect(imageListWidget, &QListWidget::itemClicked, this, &BackgroundImageSelectorDialog::onItemClicked);
    mainLayout->addWidget(imageListWidget);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    // 上传图片按钮
    uploadButton = new QPushButton(QStringLiteral("上传图片"));
    connect(uploadButton, &QPushButton::clicked, this, &BackgroundImageSelectorDialog::onUploadButtonClicked);
    buttonLayout->addWidget(uploadButton);

    buttonLayout->addStretch(); // 添加伸缩项，将右侧按钮推到右边

    // 确定和取消按钮
    okButton = new QPushButton(QStringLiteral("确定"));
    connect(okButton, &QPushButton::clicked, this, &BackgroundImageSelectorDialog::onOkButtonClicked);
    buttonLayout->addWidget(okButton);

    cancelButton = new QPushButton(QStringLiteral("取消"));
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    // 加载保存的图片列表
    loadImageList();

}

void BackgroundImageSelectorDialog::loadImageList()
{
    // 使用 QSettings 加载保存的图片路径
    QSettings settings("MyApp", "BackgroundImages");
    imagePaths = settings.value("imagePaths", QStringList()).toStringList();

    // 清空当前列表
    imageListWidget->clear();

    // 将保存的图片路径添加到列表中
    for (const QString &filePath : imagePaths) {
        QPixmap pixmap(filePath);
        if (!pixmap.isNull()) {
            // 缩放图片以适应缩略图大小
            QPixmap scaledPixmap = pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QListWidgetItem *item = new QListWidgetItem(QIcon(scaledPixmap), filePath.split('/').last());
            item->setData(Qt::UserRole, filePath); // 存储完整路径
            imageListWidget->addItem(item);
        } else {
            qDebug() << "无法加载图片（可能文件已删除或移动）:" << filePath;
            // 可以选择从列表中移除无效路径，但这里暂时保留以便用户了解
        }
    }
    qDebug() << "加载图片列表，数量:" << imagePaths.size();
}

void BackgroundImageSelectorDialog::saveImageList()
{
    // 使用 QSettings 保存图片路径列表
    QSettings settings("MyApp", "BackgroundImages");
    settings.setValue("imagePaths", imagePaths);
    qDebug() << "保存图片列表，数量:" << imagePaths.size();
}




void BackgroundImageSelectorDialog::onUploadButtonClicked()
{
    // 打开文件选择对话框，允许选择图片文件
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    QStringLiteral("上传背景图片"),
                                                    QString(),
                                                    QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp)"));
    if (!filePath.isEmpty()) {
        // 加载图片并显示缩略图
        QPixmap pixmap(filePath);
        if (!pixmap.isNull()) {
            // 缩放图片以适应缩略图大小
            QPixmap scaledPixmap = pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QListWidgetItem *item = new QListWidgetItem(QIcon(scaledPixmap), filePath.split('/').last());
            item->setData(Qt::UserRole, filePath); // 存储完整路径
            imageListWidget->addItem(item);
        } else {
            qDebug() << "无法加载图片:" << filePath;
        }
    }
}

void BackgroundImageSelectorDialog::onItemClicked(QListWidgetItem *item)
{
    // 记录当前选中的图片路径
    selectedImagePath = item->data(Qt::UserRole).toString();
}

void BackgroundImageSelectorDialog::onOkButtonClicked()
{
    if (!selectedImagePath.isEmpty()) {
        emit imageSelected(selectedImagePath); // 发出选中的图片路径
    }
    accept(); // 关闭对话框
}
