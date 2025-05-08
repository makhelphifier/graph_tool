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
    // 预设一些标准颜色
    standardColors << Qt::black << Qt::white << Qt::red << Qt::green << Qt::blue
                   << Qt::cyan << Qt::magenta << Qt::yellow << Qt::gray << Qt::darkRed
                   << Qt::darkGreen << Qt::darkBlue << Qt::darkCyan << Qt::darkMagenta
                   << Qt::darkYellow << Qt::darkGray;
    // 可以从 QSettings 加载保存的 customColors

    setupUi();
}

void ColorSelectorPopup::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // 1. 无填充颜色按钮 (这里理解为“默认/黑色”线条)
    noColorButton = new QPushButton(QStringLiteral("默认颜色 (黑色)"));
    connect(noColorButton, &QPushButton::clicked, this, &ColorSelectorPopup::onNoColorClicked);
    mainLayout->addWidget(noColorButton);

    mainLayout->addSpacing(5);

    // 2. 标准颜色面板
    standardColorLayout = new QGridLayout();
    standardColorLayout->setSpacing(2);
    createColorGrid(standardColorLayout, standardColors);
    mainLayout->addLayout(standardColorLayout);

    mainLayout->addSpacing(5);

    // 3. 其他颜色按钮
    otherColorButton = new QPushButton(QStringLiteral("其他线条颜色..."));
    connect(otherColorButton, &QPushButton::clicked, this, &ColorSelectorPopup::onOtherColorClicked);
    mainLayout->addWidget(otherColorButton);

    // 4. 取色按钮 (功能暂缓)
    colorPickerButton = new QPushButton(QStringLiteral("取色"));
    connect(colorPickerButton, &QPushButton::clicked, this, &ColorSelectorPopup::onColorPickerClicked);
    // colorPickerButton->setEnabled(false); // 暂时禁用
    mainLayout->addWidget(colorPickerButton);

    mainLayout->addSpacing(5);

    // 5. 自定义颜色行/面板
    customColorLayout = new QGridLayout();
    customColorLayout->setSpacing(2);
    updateCustomColorSlots(); // 初始化自定义颜色格子
    mainLayout->addLayout(customColorLayout);

    setLayout(mainLayout);
    setWindowFlags(Qt::Popup); // 设置为弹出窗口样式
}

// 创建颜色网格布局
void ColorSelectorPopup::createColorGrid(QGridLayout* layout, const QVector<QColor>& colors)
{
    int row = 0, col = 0;
    const int columns = 8; // 每行显示多少个颜色
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

    // 添加当前的自定义颜色和空位
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

// 添加新的自定义颜色（如果未满且不重复）
void ColorSelectorPopup::addCustomColor(const QColor& color)
{
    if (!color.isValid() || customColors.contains(color)) {
        return; // 无效颜色或已存在，不添加
    }
    if (customColors.size() >= maxCustomColors) {
        customColors.removeFirst(); // 如果满了，移除最早的
    }
    customColors.append(color);
    updateCustomColorSlots(); // 更新显示
    // 可以将 customColors 保存到 QSettings
}


// --- 槽函数 ---

void ColorSelectorPopup::onStandardColorClicked(const QColor& color)
{
    qDebug() << "Standard color selected:" << color.name();
    emit colorSelected(color);
    emit closePopup(); // 请求关闭
}

void ColorSelectorPopup::onCustomColorClicked(const QColor& color)
{
    qDebug() << "Custom color selected:" << color.name();
    emit colorSelected(color);
    emit closePopup(); // 请求关闭
}

void ColorSelectorPopup::onOtherColorClicked()
{
    QColor color = QColorDialog::getColor(Qt::black, this, QStringLiteral("选择颜色"));
    if (color.isValid()) {
        qDebug() << "Other color selected:" << color.name();
        addCustomColor(color); // 添加到自定义颜色中
        emit colorSelected(color);
        emit closePopup(); // 请求关闭
    }
}

void ColorSelectorPopup::onColorPickerClicked()
{
    // TODO: 实现取色功能
    qDebug() << "Color Picker clicked - 功能待实现";
    // 暂时也弹出颜色对话框
    onOtherColorClicked();
    // emit closePopup(); // 请求关闭
}

void ColorSelectorPopup::onNoColorClicked()
{
    qDebug() << "Default color (Black) selected";
    emit colorSelected(Qt::black); // 选择黑色作为默认线条颜色
    emit closePopup(); // 请求关闭
}


