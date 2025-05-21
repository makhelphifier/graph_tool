#ifndef COLORFRAME_H
#define COLORFRAME_H

#include <QFrame>
#include <QColor>

class QMouseEvent; // 前向声明

class ColorFrame : public QFrame {
    Q_OBJECT // Q_OBJECT 宏，用于信号槽机制
public:
    ColorFrame(const QColor &color, QWidget *parent = nullptr); // 构造函数，设置颜色
    QColor color() const; // 获取当前颜色

signals:
    void clicked(const QColor& color); // 点击时发出信号，传递颜色

protected:
    void mousePressEvent(QMouseEvent *event) override; // 鼠标按下事件处理
private:
    QColor m_color; // 存储当前颜色
};

#endif // COLORFRAME_H
