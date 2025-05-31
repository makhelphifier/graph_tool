#ifndef COLOR_FRAME_H
#define COLOR_FRAME_H

#include <QFrame>
#include <QColor>

class QMouseEvent; // 前向声明

class ColorFrame : public QFrame {
    Q_OBJECT // Q_OBJECT 宏现在在头文件中
public:
    ColorFrame(const QColor &color, QWidget *parent = nullptr);
    QColor color() const;

signals:
    void clicked(const QColor& color);

protected:
    void mousePressEvent(QMouseEvent *event) override;
private:
    QColor m_color;
};

#endif // COLOR_FRAME_H
