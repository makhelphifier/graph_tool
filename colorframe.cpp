#include "colorframe.h"
#include <QMouseEvent>
#include <QString> // 需要包含 QString 以使用 QString::arg

ColorFrame::ColorFrame(const QColor &color, QWidget *parent) : QFrame(parent), m_color(color) {
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);
    setFixedSize(20, 20); // 颜色块大小
    setAutoFillBackground(true); // 必须设置才能让 setStyleSheet 生效背景色
    setStyleSheet(QString("background-color: %1;").arg(m_color.isValid() ? m_color.name() : "transparent"));
    if (!m_color.isValid()) {
        // 可以画一个叉或者特殊图案表示无效/无色
        // 或者让背景透明
        setStyleSheet("background-color: white; border: 1px solid gray;"); // 暂时用白色表示空
    }
}

QColor ColorFrame::color() const {
    return m_color;
}

void ColorFrame::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_color.isValid()) {
        emit clicked(m_color); // 发出信号
    }
    QFrame::mousePressEvent(event); // 调用基类事件处理
}
