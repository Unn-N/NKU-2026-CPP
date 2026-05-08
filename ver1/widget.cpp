#include "widget.h"
#include "ui_widget.h"
#include<QPainter>
#include<QMouseEvent>
#include<cmath>
#include <algorithm>

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
    if(event->button() != Qt::LeftButton) return;
    selectedItem = nullptr;

    for(auto it = itemList.rbegin(); it != itemList.rend(); ++it)
    {
        Rectitem* bar = *it;
        if(event->x() >= bar->x && event->x() <= bar->x + bar->width &&
            event->y() >= bar->y && event->y() <= bar->y + bar->height)
        {
            selectedItem = bar;
            dragOffsetX = event->x() - bar->x;
            break;
        }
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (!selectedItem) return;

    // 1. 跟随鼠标（保持不变）
    selectedItem->x = event->x() - dragOffsetX;

    QList<int> positions = {30, 100, 170, 240, 310, 380};

    // ========================
    // 核心修复：计算【拖动矩形的中心点】
    // ========================
    int currentCenter = selectedItem->x + selectedItem->width / 2;

    // 2. 根据【中心点】判断应该在第几个索引位置
    int targetIndex = 0;
    for (int i = 0; i < positions.size(); i++) {
        // 目标位置的中心点
        int slotCenter = positions[i] + selectedItem->width / 2;
        if (currentCenter >= slotCenter) {
            targetIndex = i;
        } else {
            break;
        }
    }

    // 3. 重新排序队列
    QList<Rectitem*> sortedList = itemList;
    sortedList.removeOne(selectedItem);
    sortedList.insert(targetIndex, selectedItem);

    // 4. 其他矩形实时让位（只动别人）
    for (int i = 0; i < sortedList.size(); i++) {
        if (sortedList[i] != selectedItem) {
            sortedList[i]->x = positions[i];
        }
    }

    update();
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton || !selectedItem)
    {
        selectedItem = nullptr;
        return;
    }

    QList<int> slot = {30,100,170,240,310,380};
    QList<Rectitem*> sorted = itemList;
    std::sort(sorted.begin(), sorted.end(),
              [](Rectitem*a, Rectitem*b){ return a->x < b->x; });

    // 全部整齐归位
    for(int i = 0; i < sorted.size(); ++i)
    {
        sorted[i]->x = slot[i];
    }

    selectedItem = nullptr;
    update();
}

// 统一排列所有矩形：吸附、避让、排序全在这里
void Widget::rearrangeAllItems()
{
    QList<int> positions = {30, 100, 170, 240, 310, 380};

    // 按当前X坐标排序
    QList<Rectitem*> sorted = itemList;
    std::sort(sorted.begin(), sorted.end(), [](Rectitem* a, Rectitem* b) {
        return a->x < b->x;
    });

    // 一次性给所有矩形分配固定位置
    for (int i = 0; i < sorted.size(); i++) {
        sorted[i]->x = positions[i];
    }
}
