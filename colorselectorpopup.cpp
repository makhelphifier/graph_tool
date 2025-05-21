#include "colorselectorpopup.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QFrame>
#include <QColorDialog>
#include <QDebug>
#include <QMouseEvent>
#include "colorframe.h"

ColorSelectorPopup::ColorSelectorPopup(QWidget *parent) : QWidget(parent)
{
    // 预设标准颜色
    standardColors << Qt::black << Qt::white << Qt::red << Qt::green << Qt::blue
                   << Qt::cyan << Qt::magenta << Qt::yellow << Qt::gray << Qt::darkRed
                   << Qt::darkGreen << Qt::darkBlue << Qt::darkCyan << Qt::darkMagenta
                   << Qt::darkYellow << Qt::darkGray;
    // 可从 QSettings 加载保存的自定义颜色

    setupUi(); // 初始化界面
}

void ColorSelectorPopup::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5); // 设置边距
    mainLayout->setSpacing(5); // 设置间距

    // 1. 默认颜色按钮（黑色）
    noColorButton = new QPushButton(QStringLiteral("默认颜色 (黑色)"));
    connect(noColorButton, &QPushButton::clicked, this, &ColorSelectorPopup::onNoColorClicked);
    mainLayout->addWidget(noColorButton);

    mainLayout->addSpacing(5); // 添加间距

    // 2. 标准颜色面板
    standardColorLayout = new QGridLayout();
    standardColorLayout->setSpacing(2); // 设置网格间距
    createColorGrid(standardColorLayout, standardColors); // 创建标准颜色网格
    mainLayout->addLayout(standardColorLayout);

    mainLayout->addSpacing(5); // 添加间距

    // 3. 其他颜色按钮
    otherColorButton = new QPushButton(QStringLiteral("其他线条颜色..."));
    connect(otherColorButton, &QPushButton::clicked, this, &ColorSelectorPopup::onOtherColorClicked);
    mainLayout->addWidget(otherColorButton);

    // 4. 取色按钮（功能暂缓）
    colorPickerButton = new QPushButton(QStringLiteral("取色"));
    connect(colorPickerButton, &QPushButton::clicked, this, &ColorSelectorPopup::onColorPickerClicked);
    // colorPickerButton->setEnabled(false); // 暂时禁用
    mainLayout->addWidget(colorPickerButton);

    mainLayout->addSpacing(5); // 添加间距

    // 5. 自定义颜色面板
    customColorLayout = new QGridLayout();
    customColorLayout->setSpacing(2); // 设置网格间距
    updateCustomColorSlots(); // 初始化自定义颜色格子
    mainLayout->addLayout(customColorLayout);

    setLayout(mainLayout); // 设置主布局
    setWindowFlags(Qt::Popup); // 设置为弹出窗口样式
}

// 创建颜色网格布局
void ColorSelectorPopup::createColorGrid(QGridLayout* layout, const QVector<QColor>& colors)
{
    int row = 0, col = 0;
    const int columns = 8; // 每行显示的颜色数量
    for (const QColor& color : colors) {
        ColorFrame *colorButton = new ColorFrame(color, this);
        connect(colorButton, &ColorFrame::clicked, this, &ColorSelectorPopup::onStandardColorClicked);
        layout->addWidget(colorButton, row, col);
        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }
}

// 更新自定义颜色格子
void ColorSelectorPopup::updateCustomColorSlots()
{
    // 清理旧的格子
    QLayoutItem* item;
    while ((item = customColorLayout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    // 添加当前自定义颜色和空位
    int row = 0, col = 0;
    const int columns = maxCustomColors;
    for (int i = 0; i < maxCustomColors; ++i) {
        QColor color = (i < customColors.size()) ? customColors[i] : QColor(); // 无效颜色表示空位
        ColorFrame *customColorButton = new ColorFrame(color, this);
        if (color.isValid()) {
            connect(customColorButton, &ColorFrame::clicked, this, &ColorSelectorPopup::onCustomColorClicked);
        }
        customColorLayout->addWidget(customColorButton, row, col);
        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }
}

// 添加新的自定义颜色（未满且不重复）
void ColorSelectorPopup::addCustomColor(const QColor& color)
{
    if (!color.isValid() || customColors.contains(color)) {
        return; // 无效或已存在，不添加
    }
    if (customColors.size() >= maxCustomColors) {
        customColors.removeFirst(); // 满了则移除最早的
    }
    customColors.append(color); // 添加新颜色
    updateCustomColorSlots(); // 更新显示
    // 可将 customColors 保存到 QSettings
}

// --- 槽函数 ---

void ColorSelectorPopup::onStandardColorClicked(const QColor& color)
{
    qDebug() << "标准颜色已选择:" << color.name(); // 打印选择的标准颜色
    emit colorSelected(color); // 发出颜色选择信号
    emit closePopup(); // 请求关闭弹窗
}

void ColorSelectorPopup::onCustomColorClicked(const QColor& color)
{
    qDebug() << "自定义颜色已选择:" << color.name(); // 打印选择的自定义颜色
    emit colorSelected(color); // 发出颜色选择信号
    emit closePopup(); // 请求关闭弹窗
}

void ColorSelectorPopup::onOtherColorClicked()
{
    QColor color = QColorDialog::getColor(Qt::black, this, QStringLiteral("选择颜色")); // 打开颜色对话框
    if (color.isValid()) {
        qDebug() << "其他颜色已选择:" << color.name(); // 打印选择的其他颜色
        addCustomColor(color); // 添加到自定义颜色
        emit colorSelected(color); // 发出颜色选择信号
        emit closePopup(); // 请求关闭弹窗
    }
}

void ColorSelectorPopup::onColorPickerClicked()
{
    // TODO: 实现取色功能
    qDebug() << "取色功能点击 - 待实现"; // 打印待实现信息
    // 暂时弹出颜色对话框
    onOtherColorClicked();
}

void ColorSelectorPopup::onNoColorClicked()
{
    qDebug() << "默认颜色 (黑色) 已选择"; // 打印默认颜色选择信息
    emit colorSelected(Qt::black); // 选择黑色作为默认颜色
    emit closePopup(); // 请求关闭弹窗
}
