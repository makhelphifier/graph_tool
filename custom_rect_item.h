#ifndef CUSTOM_RECT_ITEM_H
#define CUSTOM_RECT_ITEM_H

#include <QGraphicsRectItem>
#include <QWidget>
#include <QPixmap>

class CustomRectItem : public QGraphicsRectItem
{

public:
    CustomRectItem(const QRectF &rect, QGraphicsItem *parent = nullptr);
    void setFillPixmap(const QPixmap &pixmap);
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
private:
    QPixmap fillPixmap;
};

#endif // CUSTOM_RECT_ITEM_H
