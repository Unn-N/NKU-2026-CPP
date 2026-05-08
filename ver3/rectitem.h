#ifndef RECTITEM_H
#define RECTITEM_H

#include <QObject>
#include <QColor>

class Rectitem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ getX WRITE setX)
    Q_PROPERTY(qreal colorBlend READ getColorBlend WRITE setColorBlend)

public:
    explicit Rectitem(QObject* parent = nullptr);

    // 几何
    qreal getX() const { return x; }
    void  setX(qreal v) { x = v; }

    qreal getColorBlend() const { return colorBlend; }
    void  setColorBlend(qreal v) { colorBlend = v; }

    qreal x = 0;
    qreal y = 0;
    qreal width = 50;
    qreal height = 100;
    int   value = 0;       // 底部显示的数字

    // 0.0 = 白色, 1.0 = 橙色
    qreal colorBlend = 0.0;

    // 是否正在被拖动
    bool dragging = false;
};

#endif // RECTITEM_H
