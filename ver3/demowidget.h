#ifndef DEMOWIDGET_H
#define DEMOWIDGET_H

#include <QWidget>
#include <QList>
#include <QPropertyAnimation>
#include <QTimer>
#include <functional>
#include "rectitem.h"

class DemoWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal victoryAlpha READ getVictoryAlpha WRITE setVictoryAlpha)

public:
    enum class Mode { Manual, Bubble, Selection, Insertion };

    struct Button {
        QRectF  rect;
        QString label;
        Mode    mode;
        bool    hovered = false;
    };

    explicit DemoWidget(QWidget *parent = nullptr);
    ~DemoWidget() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QList<Rectitem*> itemList;
    QList<int>       slotPositions;

    Rectitem* selectedItem = nullptr;
    int       dragOffsetX  = 0;

    QMap<Rectitem*, QPropertyAnimation*> slideAnimations;
    QMap<Rectitem*, QPropertyAnimation*> colorAnimations;

    bool  isSorted     = false;
    qreal victoryAlpha = 0.0;
    QPropertyAnimation* victoryAnim = nullptr;
    qreal getVictoryAlpha() const { return victoryAlpha; }
    void  setVictoryAlpha(qreal v) { victoryAlpha = v; update(); }

    Mode          currentMode = Mode::Manual;
    QList<Button> buttons;

    bool algoRunning = false;
    QList<std::function<void()>> algoSteps;
    QTimer* stepTimer = nullptr;

    void initSlots();
    void initButtons();
    void resetBars();
    void switchMode(Mode m);

    void animateSlideTo(Rectitem* item, qreal targetX, int duration = 200);
    void animateColorTo(Rectitem* item, qreal targetBlend, int duration = 180);
    QColor blendColor(qreal t) const;

    void rearrangeOthers();
    int  targetIndexForDrag();
    void checkSorted();
    void triggerVictory();

    void runAlgoSteps();
    void buildBubbleSteps();
    void buildSelectionSteps();
    void buildInsertionSteps();
};

#endif // DEMOWIDGET_H
