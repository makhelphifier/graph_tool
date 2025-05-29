#ifndef COLOR_SELECTOR_POPUP_FILL_H
#define COLOR_SELECTOR_POPUP_FILL_H

#include <QWidget>
#include <QColor>
#include <QVector>

// 前向声明，减少头文件依赖
class QPushButton;
class QGridLayout;
class QFrame; // 用于显示颜色块

class ColorSelectorPopupFill : public QWidget
{
    Q_OBJECT

public:
    explicit ColorSelectorPopupFill(QWidget *parent = nullptr);
    void addCustomColor(const QColor& color); // 用于添加自定义颜色

public slots:
signals:
    void colorSelected(const QColor &color); // 当颜色被选择时发出此信号
    void closePopup(); // 请求关闭弹窗的信号
    void backgroundImageSelected(const QString &imagePath); // 背景图片选择信号

private slots:
    void onStandardColorClicked(const QColor& color);
    void onOtherColorClicked();
    void onColorPickerClicked(); // 取色功能槽（暂缓实现）
    void onNoColorClicked(); // “无颜色”或默认颜色槽
    void onCustomColorClicked(const QColor& color);
    void onfillBackGroundColorClicked();

private:
    void setupUi();
    void createColorGrid(QGridLayout* layout, const QVector<QColor>& colors);
    void updateCustomColorSlots(); // 更新自定义颜色显示

    QPushButton *noColorButton;
    QGridLayout *standardColorLayout;
    QPushButton *otherColorButton;
    QPushButton *fillBackGroundColorButton;
    QPushButton *colorPickerButton;
    QGridLayout *customColorLayout; // 用于显示自定义颜色

    QVector<QColor> standardColors;
    QVector<QColor> customColors; // 存储用户选择的自定义颜色 (可持久化)
    const int maxCustomColors = 8; // 最多存储几个自定义颜色

    // 用于颜色块的小部件内部类或辅助函数会更好，这里简化处理
    QFrame* createColorButton(const QColor& color, bool isCustom = false);

};

#endif // COLOR_SELECTOR_POPUP_FILL_H
