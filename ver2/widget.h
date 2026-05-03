#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QList>
#include <QPropertyAnimation>
#include <QTimer>
#include "rectitem.h"

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QList<Rectitem*> itemList;
    QList<int>       slotPositions;

    Rectitem* selectedItem  = nullptr;
    int       dragOffsetX   = 0;

    QMap<Rectitem*, QPropertyAnimation*> slideAnimations;
    QMap<Rectitem*, QPropertyAnimation*> colorAnimations;

    // 胜利状态
    bool  isSorted       = false;   // 是否已完成排序
    qreal victoryAlpha   = 0.0;     // 胜利文字淡入进度 0~1
    QPropertyAnimation* victoryAnim = nullptr;
    Q_PROPERTY(qreal victoryAlpha READ getVictoryAlpha WRITE setVictoryAlpha)
    qreal getVictoryAlpha() const { return victoryAlpha; }
    void  setVictoryAlpha(qreal v) { victoryAlpha = v; update(); }

    void initSlots();
    void animateSlideTo(Rectitem* item, qreal targetX, int duration = 200);
    void animateColorTo(Rectitem* item, qreal targetBlend, int duration = 180);
    void rearrangeOthers();
    int  targetIndexForDrag();
    QColor blendColor(qreal t) const;
    void checkSorted();          // 松手后检测是否排好序
    void triggerVictory();       // 触发胜利动画
};

#endif // WIDGET_H
