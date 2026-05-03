#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include<QList>
#include"rectitem.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private:
    Ui::Widget *ui;

    QList<Rectitem*> itemList;
    Rectitem* selectedItem = nullptr; // 正在拖动的矩形
    int dragOffsetX; // 鼠标在矩形内部的偏移
    void rearrangeAllItems();


protected:
    void paintEvent(QPaintEvent *event)override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
#endif // WIDGET_H
