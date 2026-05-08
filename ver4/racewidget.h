#ifndef RACEWIDGET_H
#define RACEWIDGET_H

#include <QWidget>
#include <QList>
#include <QMap>
#include <QPropertyAnimation>
#include <QTimer>
#include <functional>
#include "rectitem.h"

class RaceWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal victoryAlpha READ getVictoryAlpha WRITE setVictoryAlpha)

public:
    enum class AlgoMode { Bubble, Selection, Insertion };

    struct Button {
        QRectF  rect;
        QString label;
        int     action; // 0=start race, 1=bubble, 2=selection, 3=insertion
        bool    hovered = false;
        bool    active  = false;
    };

    explicit RaceWidget(QWidget *parent = nullptr);
    ~RaceWidget() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QList<Rectitem*> manualItems;
    QList<Rectitem*> algoItems;
    QList<int>       manualSlots;
    QList<int>       algoSlots;

    Rectitem* selectedItem = nullptr;
    int       dragOffsetX  = 0;

    QMap<Rectitem*, QPropertyAnimation*> slideAnimations;
    QMap<Rectitem*, QPropertyAnimation*> colorAnimations;

    bool  raceStarted  = false;
    bool  manualDone   = false;
    bool  algoDone     = false;
    int   finishOrder  = 0; // 0=pending, 1=manual first, 2=algo first
    qreal victoryAlpha = 0.0;
    QPropertyAnimation* victoryAnim = nullptr;
    QString winnerText;

    AlgoMode      algoMode = AlgoMode::Bubble;
    QList<Button> buttons;
    bool          algoRunning = false;
    QList<std::function<void()>> algoSteps;
    QTimer* stepTimer = nullptr;

    void initSlots();
    void initButtons();
    void resetRace();
    void startRace();

    void animateSlideTo(Rectitem* item, qreal targetX, int duration = 200);
    void animateColorTo(Rectitem* item, qreal targetBlend, int duration = 180);
    QColor blendColor(const QColor& accent, qreal t) const;

    void rearrangeOthers();
    int  targetIndexForDrag();
    void checkManualDone();
    void checkAlgoDone();
    void checkBothDone();
    void showWinner(const QString& text);

    void buildAlgoSteps();
    void buildBubbleSteps();
    void buildSelectionSteps();
    void buildInsertionSteps();
    void runAlgoSteps();

    qreal getVictoryAlpha() const { return victoryAlpha; }
    void  setVictoryAlpha(qreal v) { victoryAlpha = v; update(); }
};

#endif // RACEWIDGET_H
