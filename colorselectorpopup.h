#ifndef COLORSELECTORPOPUP_H
#define COLORSELECTORPOPUP_H

#include <QWidget>
#include <QColor>
#include <QVector>

class QPushButton;
class QGridLayout;
class QFrame; // 用于显示颜色块

class ColorSelectorPopup : public QWidget
{
    Q_OBJECT

public:
    explicit ColorSelectorPopup(QWidget *parent = nullptr); // 构造函数
    void addCustomColor(const QColor& color); // 添加自定义颜色

signals:
    void colorSelected(const QColor &color); // 颜色选择信号
    void closePopup(); // 关闭弹窗信号

private slots:
    void onStandardColorClicked(const QColor& color); // 标准颜色点击处理
    void onOtherColorClicked(); // 其他颜色点击处理
    void onColorPickerClicked(); // 取色功能（暂未实现）
    void onNoColorClicked(); // 无颜色或默认颜色处理
    void onCustomColorClicked(const QColor& color); // 自定义颜色点击处理

private:
    void setupUi(); // 初始化界面
    void createColorGrid(QGridLayout* layout, const QVector<QColor>& colors); // 创建颜色网格
    void updateCustomColorSlots(); // 更新自定义颜色显示

    QPushButton *noColorButton; // 无颜色按钮
    QGridLayout *standardColorLayout; // 标准颜色布局
    QPushButton *otherColorButton; // 其他颜色按钮
    QPushButton *colorPickerButton; // 取色按钮
    QGridLayout *customColorLayout; // 自定义颜色布局

    QVector<QColor> standardColors; // 标准颜色列表
    QVector<QColor> customColors; // 自定义颜色列表
    const int maxCustomColors = 8; // 最大自定义颜色数量

    QFrame* createColorButton(const QColor& color, bool isCustom = false); // 创建颜色按钮
};

#endif // COLORSELECTORPOPUP_H
