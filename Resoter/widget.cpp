#include "widget.h"
#include "ui_widget.h"
#include<QPainter>
#include<QMouseEvent>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->resize(800, 600);
    int windowHeight = 600;
    int barWidth = 50;
    int xPos = 30;


    for(int i=0; i<6; i++)
    {
        Rectitem* item = new Rectitem();
        item->x = xPos;
        item->width = barWidth;
        item->height = 100 + rand() % 150;
        item->y = windowHeight - item->height;
        itemList.append(item);
        xPos += barWidth + 20;
    }


}

Widget::~Widget()
{
    delete ui;
}


void Widget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter p(this);
    p.setBrush(Qt::white);

    for(auto bar : itemList)
    {
        p.drawRect(bar->x, bar->y, bar->width, bar->height);
    }
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    // 只处理左键点击
    if(event->button() == Qt::LeftButton)
    {
        // 从后往前遍历，这样上面的矩形会优先被选中
        for(auto it = itemList.rbegin(); it != itemList.rend(); ++it)
        {
            Rectitem* bar = *it;
            // 判断鼠标是否点中了矩形
            if(event->x() >= bar->x && event->x() <= bar->x + bar->width &&
                event->y() >= bar->y && event->y() <= bar->y + bar->height)
            {
                selectedItem = bar;
                // 计算偏移量：鼠标X坐标 - 矩形X坐标
                dragOffsetX = event->x() - bar->x;
                break;
            }
        }
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if(selectedItem != nullptr)
    {
        // 只改变X坐标，保持Y坐标不变，这样只会左右拖动
        selectedItem->x = event->x() - dragOffsetX;
        // 强制重绘窗口，让矩形位置立刻更新
        update();
    }
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        if (selectedItem != nullptr) {
            //6个固定位置
            QList<int> positions = {30, 100, 170, 240, 310, 380};

            // 找到离当前矩形最近的固定位置
            int bestPos = positions[0];
            int minDist = abs(selectedItem->x - bestPos);

            for (int pos : positions) {
                int dist = abs(selectedItem->x - pos);
                if (dist < minDist) {
                    minDist = dist;
                    bestPos = pos;
                }
            }

            // 自动吸附过去
            selectedItem->x = bestPos;
            update(); // 刷新画面
        }

        selectedItem = nullptr;
    }
}

