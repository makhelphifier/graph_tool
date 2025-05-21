#include "colorframe.h"
#include <QMouseEvent>
#include <QString> // 包含 QString 以使用 QString::arg

ColorFrame::ColorFrame(const QColor &color, QWidget *parent) : QFrame(parent), m_color(color) {
    setFrameShape(QFrame::StyledPanel); // 设置框架形状
    setFrameShadow(QFrame::Sunken); // 设置框架阴影
    setFixedSize(20, 20); // 设置颜色块大小
    setAutoFillBackground(true); // 必须设置以应用背景色
    setStyleSheet(QString("background-color: %1;").arg(m_color.isValid() ? m_color.name() : "transparent")); // 设置背景色
    if (!m_color.isValid()) {
        setStyleSheet("background-color: white; border: 1px solid gray;"); // 无颜色时用白色表示
    }
}

QColor ColorFrame::color() const {
    return m_color; // 返回当前颜色
}

void ColorFrame::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_color.isValid()) {
        emit clicked(m_color); // 左键点击且颜色有效时发出信号
    }
    QFrame::mousePressEvent(event); // 调用基类事件处理
}
