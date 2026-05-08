#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class DemoWidget;
class RaceWidget;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    DemoWidget* demoWidget = nullptr;
    RaceWidget* raceWidget = nullptr;

    int activeModule = 0; // 0 = 展示, 1 = 比赛
    QRect demoBtnRect;
    QRect raceBtnRect;

    void switchTo(int module);
};

#endif // WIDGET_H
