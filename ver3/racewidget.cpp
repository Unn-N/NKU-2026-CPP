#include "racewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// ── 配色 ─────────────────────────────────────────────
static const QColor BG_COLOR   (237, 230, 219);
static const QColor BAR_WHITE  (255, 255, 255);
static const QColor BAR_BLUE   ( 68, 122, 204);
static const QColor BAR_ORANGE (204, 102,  68);
static const QColor BAR_GREEN  ( 88, 177, 120);
static const QColor TEXT_COLOR (160, 140, 120);
static const QColor BTN_NORMAL (220, 210, 198);
static const QColor BTN_HOVER  (230, 185, 160);
static const QColor BTN_GREEN  ( 88, 177, 120);
static const QColor BTN_ORANGE (204, 102,  68);

static const int RADIUS      = 8;
static const int BAR_W       = 50;
static const int BAR_GAP     = 18;
static const int LEFT_OFFSET = 30;
static const int SLOT_STEP   = BAR_W + BAR_GAP;     // 68
static const int PANEL_COUNT = 6;
static const int ALGO_LEFT   = 470;
static const int WIN_W       = 900;
static const int NUM_AREA    = 30;
static const int BTN_AREA    = 60;
static const int BOTTOM_MARGIN = NUM_AREA + BTN_AREA; // 90
static const int WIN_H       = 660;
static const int STEP_MS     = 350;

// ── 构造 ─────────────────────────────────────────────
RaceWidget::RaceWidget(QWidget *parent) : QWidget(parent)
{
    resize(WIN_W, WIN_H);
    setMouseTracking(true);
    srand(static_cast<unsigned>(time(nullptr)));

    initSlots();
    initButtons();

    victoryAnim = new QPropertyAnimation(this, "victoryAlpha", this);
    victoryAnim->setEasingCurve(QEasingCurve::OutCubic);

    stepTimer = new QTimer(this);
    stepTimer->setSingleShot(false);

    resetRace();
}

RaceWidget::~RaceWidget()
{
    qDeleteAll(manualItems);
    qDeleteAll(algoItems);
}

// ── 槽位 ─────────────────────────────────────────────
void RaceWidget::initSlots()
{
    manualSlots.clear();
    algoSlots.clear();
    for (int i = 0; i < PANEL_COUNT; i++) {
        manualSlots.append(LEFT_OFFSET + i * SLOT_STEP);
        algoSlots.append(ALGO_LEFT + i * SLOT_STEP);
    }
}

// ── 按钮 ─────────────────────────────────────────────
void RaceWidget::initButtons()
{
    buttons.clear();
    QStringList labels = { "开始比赛", "冒泡", "选择", "插入" };
    QList<int> actions = { 0, 1, 2, 3 };

    int btnCount = 4;
    int btnH     = 38;
    int totalW   = WIN_W - LEFT_OFFSET * 2;
    int btnW     = (totalW - (btnCount - 1) * 10) / btnCount;
    int btnY     = WIN_H - BTN_AREA + (BTN_AREA - btnH) / 2;

    for (int i = 0; i < btnCount; i++) {
        Button b;
        b.rect    = QRectF(LEFT_OFFSET + i * (btnW + 10), btnY, btnW, btnH);
        b.label   = labels[i];
        b.action  = actions[i];
        b.hovered = false;
        b.active  = (i >= 1 && static_cast<int>(algoMode) == i - 1);
        buttons.append(b);
    }
}

// ── 重置 / 开始 ──────────────────────────────────────
void RaceWidget::resetRace()
{
    stepTimer->stop();
    disconnect(stepTimer, &QTimer::timeout, this, nullptr);
    algoSteps.clear();
    algoRunning   = false;
    raceStarted   = false;
    manualDone    = false;
    algoDone      = false;
    finishOrder   = 0;
    victoryAlpha  = 0.0;
    if (victoryAnim) victoryAnim->stop();
    selectedItem = nullptr;
    winnerText.clear();

    const int VAL_MIN = 10, VAL_MAX = 99;
    const int H_MIN = 40, H_MAX = WIN_H - BOTTOM_MARGIN - 20;
    QList<int> values;
    int segLen = (VAL_MAX - VAL_MIN) / PANEL_COUNT;
    for (int i = 0; i < PANEL_COUNT; i++)
        values.append(VAL_MIN + i * segLen + rand() % qMax(1, segLen));
    for (int i = PANEL_COUNT - 1; i > 0; i--)
        std::swap(values[i], values[rand() % (i + 1)]);

    if (manualItems.isEmpty()) {
        for (int i = 0; i < PANEL_COUNT; i++) {
            auto* item = new Rectitem(this);
            item->width = BAR_W;
            manualItems.append(item);
            auto* sa = new QPropertyAnimation(item, "x", this);
            sa->setEasingCurve(QEasingCurve::OutCubic);
            slideAnimations[item] = sa;
            connect(sa, &QPropertyAnimation::valueChanged, this, [this](const QVariant&){ update(); });
            auto* ca = new QPropertyAnimation(item, "colorBlend", this);
            ca->setEasingCurve(QEasingCurve::InOutQuad);
            colorAnimations[item] = ca;
            connect(ca, &QPropertyAnimation::valueChanged, this, [this](const QVariant&){ update(); });
        }
        for (int i = 0; i < PANEL_COUNT; i++) {
            auto* item = new Rectitem(this);
            item->width = BAR_W;
            algoItems.append(item);
            auto* sa = new QPropertyAnimation(item, "x", this);
            sa->setEasingCurve(QEasingCurve::OutCubic);
            slideAnimations[item] = sa;
            connect(sa, &QPropertyAnimation::valueChanged, this, [this](const QVariant&){ update(); });
            auto* ca = new QPropertyAnimation(item, "colorBlend", this);
            ca->setEasingCurve(QEasingCurve::InOutQuad);
            colorAnimations[item] = ca;
            connect(ca, &QPropertyAnimation::valueChanged, this, [this](const QVariant&){ update(); });
        }
    }

    for (int i = 0; i < PANEL_COUNT; i++) {
        Rectitem* item = manualItems[i];
        slideAnimations[item]->stop();
        colorAnimations[item]->stop();
        item->value      = values[i];
        item->height     = H_MIN + (qreal)(item->value - VAL_MIN) / (VAL_MAX - VAL_MIN) * (H_MAX - H_MIN);
        item->x          = manualSlots[i];
        item->y          = WIN_H - BOTTOM_MARGIN - item->height;
        item->colorBlend = 0.0;
        item->dragging   = false;
    }
    for (int i = 0; i < PANEL_COUNT; i++) {
        Rectitem* item = algoItems[i];
        slideAnimations[item]->stop();
        colorAnimations[item]->stop();
        item->value      = values[i];
        item->height     = H_MIN + (qreal)(item->value - VAL_MIN) / (VAL_MAX - VAL_MIN) * (H_MAX - H_MIN);
        item->x          = algoSlots[i];
        item->y          = WIN_H - BOTTOM_MARGIN - item->height;
        item->colorBlend = 0.0;
        item->dragging   = false;
    }
    update();
}

void RaceWidget::startRace()
{
    raceStarted = true;
    buildAlgoSteps();
    runAlgoSteps();
    for (auto& b : buttons) {
        b.active = (b.action == 0);
    }
    update();
}

// ── 动画 ─────────────────────────────────────────────
void RaceWidget::animateSlideTo(Rectitem* item, qreal targetX, int duration)
{
    auto* anim = slideAnimations[item];
    if (!anim || qAbs(item->x - targetX) < 1.0) return;
    anim->stop();
    anim->setDuration(duration);
    anim->setStartValue(item->x);
    anim->setEndValue(targetX);
    anim->start();
}

void RaceWidget::animateColorTo(Rectitem* item, qreal targetBlend, int duration)
{
    auto* anim = colorAnimations[item];
    if (!anim || qAbs(item->colorBlend - targetBlend) < 0.01) return;
    anim->stop();
    anim->setDuration(duration);
    anim->setStartValue(item->colorBlend);
    anim->setEndValue(targetBlend);
    anim->start();
}

QColor RaceWidget::blendColor(const QColor& accent, qreal t) const
{
    return QColor(
        BAR_WHITE.red()   + t * (accent.red()   - BAR_WHITE.red()),
        BAR_WHITE.green() + t * (accent.green() - BAR_WHITE.green()),
        BAR_WHITE.blue()  + t * (accent.blue()  - BAR_WHITE.blue())
    );
}

// ── 绘制 ─────────────────────────────────────────────
void RaceWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), BG_COLOR);

    QFont font = p.font();
    font.setPointSize(9);
    font.setWeight(QFont::Medium);
    p.setFont(font);

    // ── 面板标题 ──
    QFont lf = p.font();
    lf.setPointSize(12);
    lf.setBold(true);
    p.setFont(lf);
    p.setPen(TEXT_COLOR);
    p.drawText(QRectF(LEFT_OFFSET, 10, 150, 24), Qt::AlignLeft, "你（手动）");
    p.drawText(QRectF(ALGO_LEFT, 10, 150, 24), Qt::AlignLeft, "算法");
    p.setFont(font);

    // ── 中间分隔线 ──
    int divX = (manualSlots.last() + BAR_W + algoSlots.first()) / 2;
    p.setPen(QPen(QColor(200, 190, 178), 1, Qt::DashLine));
    p.drawLine(divX, 40, divX, WIN_H - BOTTOM_MARGIN);

    // ── 底线 ──
    p.setPen(QPen(QColor(200, 190, 178), 1));
    p.drawLine(0, WIN_H - BOTTOM_MARGIN, WIN_W, WIN_H - BOTTOM_MARGIN);

    // ── 状态文字 ──
    QString leftStatus, rightStatus;
    if (!raceStarted) {
        leftStatus  = "等待开始";
        rightStatus = "等待开始";
    } else {
        leftStatus  = manualDone ? "已完成 ✓" : "拖拽排序中...";
        rightStatus = algoDone   ? "已完成 ✓" : "运行中...";
    }
    p.setPen(TEXT_COLOR);
    p.drawText(QRectF(LEFT_OFFSET, WIN_H - BOTTOM_MARGIN + 2, 200, NUM_AREA - 4), Qt::AlignLeft,
               "你: " + leftStatus);
    p.drawText(QRectF(ALGO_LEFT, WIN_H - BOTTOM_MARGIN + 2, 200, NUM_AREA - 4), Qt::AlignLeft,
               "算法: " + rightStatus);

    // ── 绘制柱子 ──
    auto drawBar = [&](Rectitem* bar, const QColor& accent, bool done) {
        QColor targetColor = done ? BAR_GREEN : accent;
        QColor color = blendColor(targetColor, bar->colorBlend);

        QPainterPath path;
        QRectF r(bar->x, bar->y, bar->width, bar->height);
        path.moveTo(r.left() + RADIUS, r.top());
        path.lineTo(r.right() - RADIUS, r.top());
        path.quadTo(r.right(), r.top(), r.right(), r.top() + RADIUS);
        path.lineTo(r.right(), r.bottom() - RADIUS);
        path.quadTo(r.right(), r.bottom(), r.right() - RADIUS, r.bottom());
        path.lineTo(r.left() + RADIUS, r.bottom());
        path.quadTo(r.left(), r.bottom(), r.left(), r.bottom() - RADIUS);
        path.lineTo(r.left(), r.top() + RADIUS);
        path.quadTo(r.left(), r.top(), r.left() + RADIUS, r.top());
        path.closeSubpath();

        if (bar->dragging) {
            for (int s = 8; s >= 1; s--) {
                p.save();
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0, 0, 0, 5 * s));
                p.translate(s * 0.5, s * 0.8);
                p.drawPath(path);
                p.restore();
            }
        }

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(path);

        p.setPen(TEXT_COLOR);
        p.drawText(QRectF(bar->x, WIN_H - BOTTOM_MARGIN + 5, bar->width, 20),
                   Qt::AlignHCenter, QString::number(bar->value));
    };

    for (auto* bar : manualItems) if (!bar->dragging) drawBar(bar, BAR_BLUE, manualDone);
    for (auto* bar : manualItems) if ( bar->dragging) drawBar(bar, BAR_BLUE, manualDone);
    for (auto* bar : algoItems) drawBar(bar, BAR_ORANGE, algoDone);

    // ── 按钮 ──
    p.setPen(QPen(QColor(210, 200, 188), 1));
    p.drawLine(0, WIN_H - BTN_AREA, WIN_W, WIN_H - BTN_AREA);

    for (auto& btn : buttons) {
        QColor fill = BTN_NORMAL;
        if (btn.action == 0) {
            fill = btn.active ? BTN_GREEN : (btn.hovered ? BTN_HOVER : BTN_NORMAL);
        } else {
            fill = btn.active ? BTN_ORANGE : (btn.hovered ? BTN_HOVER : BTN_NORMAL);
        }

        QPainterPath bp;
        bp.addRoundedRect(btn.rect, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(bp);

        QFont bf = p.font();
        bf.setPointSize(9);
        bf.setBold(btn.active || btn.action == 0);
        p.setFont(bf);
        p.setPen(btn.active ? Qt::white : QColor(100, 80, 60));
        p.drawText(btn.rect, Qt::AlignCenter, btn.label);
    }
    p.setFont(font);

    // ── 胜利播报 ──
    if (victoryAlpha > 0.0) {
        p.fillRect(rect(), QColor(237, 230, 219, (int)(60 * victoryAlpha)));

        QFont big = p.font();
        big.setPointSize(36);
        big.setBold(true);
        p.setFont(big);
        p.setPen(QColor(88, 177, 120, (int)(255 * victoryAlpha)));
        p.drawText(QRectF(0, WIN_H / 2 - 120, WIN_W, 60), Qt::AlignHCenter, winnerText);

        QFont small = p.font();
        small.setPointSize(13);
        small.setBold(false);
        p.setFont(small);
        p.setPen(QColor(120, 160, 130, (int)(220 * victoryAlpha)));
        p.drawText(QRectF(0, WIN_H / 2 - 50, WIN_W, 40), Qt::AlignHCenter,
                   "点击「开始比赛」再来一局");
        p.setFont(font);
    }
}

// ── 鼠标按下 ─────────────────────────────────────────
void RaceWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    for (auto& btn : buttons) {
        if (!btn.rect.contains(event->position())) continue;

        if (btn.action == 0) {
            if (algoRunning) {
                stepTimer->stop();
                disconnect(stepTimer, &QTimer::timeout, this, nullptr);
                algoRunning = false;
                algoSteps.clear();
            }
            resetRace();
            startRace();
        } else {
            if (raceStarted && !manualDone) break;
            algoMode = static_cast<AlgoMode>(btn.action - 1);
            for (auto& b : buttons) b.active = (b.action == btn.action);
            update();
        }
        return;
    }

    if (!raceStarted || manualDone) return;

    selectedItem = nullptr;
    for (int i = manualItems.size() - 1; i >= 0; i--) {
        Rectitem* bar = manualItems[i];
        if (QRectF(bar->x, bar->y, bar->width, bar->height).contains(event->position())) {
            selectedItem = bar;
            dragOffsetX  = event->position().x() - bar->x;
            bar->dragging = true;
            slideAnimations[bar]->stop();
            animateColorTo(bar, 1.0, 120);
            break;
        }
    }
}

// ── 拖拽辅助 ─────────────────────────────────────────
int RaceWidget::targetIndexForDrag()
{
    qreal dragCenter = selectedItem->x + selectedItem->width / 2.0;
    int target = 0;
    qreal minDist = qAbs(dragCenter - (manualSlots[0] + BAR_W / 2.0));
    for (int i = 1; i < manualSlots.size(); i++) {
        qreal dist = qAbs(dragCenter - (manualSlots[i] + BAR_W / 2.0));
        if (dist < minDist) { minDist = dist; target = i; }
    }
    return target;
}

void RaceWidget::rearrangeOthers()
{
    int targetIdx = targetIndexForDrag();
    QList<Rectitem*> others;
    for (auto* b : manualItems) if (b != selectedItem) others.append(b);
    std::sort(others.begin(), others.end(), [](Rectitem* a, Rectitem* b){ return a->x < b->x; });

    int si = 0;
    for (int slotI = 0; slotI < manualSlots.size() && si < others.size(); slotI++) {
        if (slotI == targetIdx) continue;
        animateSlideTo(others[si], manualSlots[slotI], 180);
        si++;
    }
}

// ── 鼠标移动 ─────────────────────────────────────────
void RaceWidget::mouseMoveEvent(QMouseEvent *event)
{
    for (auto& btn : buttons) {
        btn.hovered = btn.rect.contains(event->position());
    }

    if (selectedItem) {
        selectedItem->x = qMax((qreal)0,
                               qMin(event->position().x() - dragOffsetX, (qreal)(WIN_W - BAR_W)));
        rearrangeOthers();
        update();
        return;
    }

    if (!raceStarted || manualDone) { update(); return; }

    for (auto* bar : manualItems) {
        bool hovered = QRectF(bar->x, bar->y, bar->width, bar->height).contains(event->position());
        animateColorTo(bar, hovered ? 1.0 : 0.0, 80);
    }
    update();
}

// ── 鼠标释放 ─────────────────────────────────────────
void RaceWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !selectedItem) {
        selectedItem = nullptr;
        return;
    }

    int targetIdx = targetIndexForDrag();
    QList<Rectitem*> others;
    for (auto* b : manualItems) if (b != selectedItem) others.append(b);
    std::sort(others.begin(), others.end(), [](Rectitem* a, Rectitem* b){ return a->x < b->x; });
    others.insert(targetIdx, selectedItem);
    manualItems = others;

    for (int i = 0; i < manualItems.size(); i++)
        animateSlideTo(manualItems[i], manualSlots[i], 220);

    selectedItem->dragging = false;
    selectedItem = nullptr;
    update();
    checkManualDone();
}

// ── 胜负判定 ─────────────────────────────────────────
void RaceWidget::checkManualDone()
{
    if (manualDone || !raceStarted) return;
    for (int i = 1; i < manualItems.size(); i++)
        if (manualItems[i]->value < manualItems[i-1]->value) return;

    manualDone = true;
    if (finishOrder == 0) finishOrder = 1;
    for (int i = 0; i < manualItems.size(); i++) {
        QTimer::singleShot(100 + i * 60, this, [this, i]() {
            if (i < manualItems.size()) animateColorTo(manualItems[i], 1.0, 300);
        });
    }
    checkBothDone();
}

void RaceWidget::checkAlgoDone()
{
    if (algoDone) return;

    algoDone = true;
    if (finishOrder == 0) finishOrder = 2;
    for (int i = 0; i < algoItems.size(); i++) {
        QTimer::singleShot(100 + i * 60, this, [this, i]() {
            if (i < algoItems.size()) animateColorTo(algoItems[i], 1.0, 300);
        });
    }
    checkBothDone();
}

void RaceWidget::checkBothDone()
{
    if (!manualDone || !algoDone) return;

    QString text;
    if      (finishOrder == 1) text = "你赢了!";
    else if (finishOrder == 2) text = "算法赢了!";
    else                       text = "平局!";

    QTimer::singleShot(600, this, [this, text]() { showWinner(text); });
}

void RaceWidget::showWinner(const QString& text)
{
    winnerText = text;
    victoryAnim->stop();
    victoryAnim->setDuration(800);
    victoryAnim->setStartValue(0.0);
    victoryAnim->setEndValue(1.0);
    victoryAnim->start();
}

// ── 算法步骤播放 ─────────────────────────────────────
void RaceWidget::buildAlgoSteps()
{
    switch (algoMode) {
        case AlgoMode::Bubble:    buildBubbleSteps();    break;
        case AlgoMode::Selection: buildSelectionSteps(); break;
        case AlgoMode::Insertion: buildInsertionSteps(); break;
    }
}

void RaceWidget::runAlgoSteps()
{
    if (algoSteps.isEmpty()) return;
    algoRunning = true;
    int* idx = new int(0);

    connect(stepTimer, &QTimer::timeout, this, [this, idx]() {
        if (*idx >= algoSteps.size()) {
            stepTimer->stop();
            disconnect(stepTimer, &QTimer::timeout, this, nullptr);
            algoRunning = false;
            delete idx;
            QTimer::singleShot(STEP_MS, this, [this](){ checkAlgoDone(); });
            return;
        }
        algoSteps[(*idx)++]();
    });
    stepTimer->start(STEP_MS);
}

// ── 冒泡排序 ─────────────────────────────────────────
void RaceWidget::buildBubbleSteps()
{
    algoSteps.clear();
    QList<Rectitem*> arr = algoItems;
    int n = arr.size();

    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            Rectitem* a = arr[j];
            Rectitem* b = arr[j+1];
            algoSteps.append([this, a, b, i, n]() {
                for (int k = n - 1 - i + 1; k < n; k++) animateColorTo(algoItems[k], 0.5, 80);
                for (auto* bar : algoItems) if (bar->colorBlend < 0.4) animateColorTo(bar, 0.0, 80);
                animateColorTo(a, 1.0, 100);
                animateColorTo(b, 1.0, 100);
            });

            if (arr[j]->value > arr[j+1]->value) {
                std::swap(arr[j], arr[j+1]);
                Rectitem* left  = arr[j];
                Rectitem* right = arr[j+1];
                int ls = j, rs = j + 1;
                algoSteps.append([this, left, right, ls, rs]() {
                    int ia = algoItems.indexOf(left), ib = algoItems.indexOf(right);
                    if (ia >= 0 && ib >= 0) std::swap(algoItems[ia], algoItems[ib]);
                    animateSlideTo(left,  algoSlots[ls], STEP_MS - 50);
                    animateSlideTo(right, algoSlots[rs], STEP_MS - 50);
                });
            }
        }
        Rectitem* done = arr[n - 1 - i];
        algoSteps.append([this, done]() { animateColorTo(done, 0.5, 200); });
    }
    Rectitem* last = arr[0];
    algoSteps.append([this, last]() { animateColorTo(last, 0.5, 200); });
}

// ── 选择排序 ─────────────────────────────────────────
void RaceWidget::buildSelectionSteps()
{
    algoSteps.clear();
    int n = algoItems.size();

    QList<int> pos;
    for (int i = 0; i < n; i++) pos.append(i);

    QList<int> slot;
    for (int i = 0; i < n; i++) slot.append(i);

    for (int i = 0; i < n - 1; i++) {
        int minSlot = i;
        for (int j = i + 1; j < n; j++)
            if (algoItems[slot[j]]->value < algoItems[slot[minSlot]]->value)
                minSlot = j;

        int curMinSlot = i;
        for (int j = i + 1; j < n; j++) {
            int scanBarIdx   = slot[j];
            int curMinBarIdx = slot[curMinSlot];
            int ii = i;
            algoSteps.append([this, scanBarIdx, curMinBarIdx, ii]() {
                for (auto* bar : algoItems) animateColorTo(bar, 0.0, 60);
                for (int k = 0; k < ii; k++) animateColorTo(algoItems[k], 0.5, 60);
                animateColorTo(algoItems[scanBarIdx],   1.0, 100);
                animateColorTo(algoItems[curMinBarIdx], 0.7, 100);
            });
            if (algoItems[slot[j]]->value < algoItems[slot[curMinSlot]]->value)
                curMinSlot = j;
        }

        if (minSlot != i) {
            int barA = slot[i];
            int barB = slot[minSlot];

            pos[barA] = minSlot;
            pos[barB] = i;
            slot[i]       = barB;
            slot[minSlot] = barA;

            int targetA = minSlot;
            int targetB = i;
            algoSteps.append([this, barA, barB, targetA, targetB]() {
                animateColorTo(algoItems[barA], 1.0, 100);
                animateColorTo(algoItems[barB], 1.0, 100);
                animateSlideTo(algoItems[barA], algoSlots[targetA], STEP_MS - 50);
                animateSlideTo(algoItems[barB], algoSlots[targetB], STEP_MS - 50);
            });
        }

        QList<int> doneIndices;
        for (int k = 0; k <= i; k++) doneIndices.append(slot[k]);
        algoSteps.append([this, doneIndices]() {
            for (auto* bar : algoItems) animateColorTo(bar, 0.0, 60);
            for (int idx : doneIndices) animateColorTo(algoItems[idx], 0.5, 200);
        });
    }

    int lastIdx = slot[n - 1];
    algoSteps.append([this, lastIdx]() {
        animateColorTo(algoItems[lastIdx], 0.5, 200);
    });

    QList<int> finalSlot = slot;
    algoSteps.append([this, finalSlot]() {
        QList<Rectitem*> sorted(algoItems.size());
        for (int j = 0; j < finalSlot.size(); j++)
            sorted[j] = algoItems[finalSlot[j]];
        algoItems = sorted;
    });
}

// ── 插入排序 ─────────────────────────────────────────
void RaceWidget::buildInsertionSteps()
{
    algoSteps.clear();
    QList<Rectitem*> arr = algoItems;
    int n = arr.size();

    for (int i = 1; i < n; i++) {
        Rectitem* key = arr[i];
        int j = i - 1;

        algoSteps.append([this, key, i]() {
            for (auto* bar : algoItems) animateColorTo(bar, 0.0, 80);
            for (int k = 0; k < i; k++) animateColorTo(algoItems[k], 0.5, 80);
            animateColorTo(key, 1.0, 100);
        });

        while (j >= 0 && arr[j]->value > key->value) {
            arr[j + 1] = arr[j];
            Rectitem* moving = arr[j];
            int toSlot = j + 1;
            algoSteps.append([this, moving, toSlot]() {
                int idx = algoItems.indexOf(moving);
                if (idx >= 0 && toSlot < algoItems.size()) {
                    algoItems.removeAt(idx);
                    algoItems.insert(toSlot, moving);
                }
                animateSlideTo(moving, algoSlots[toSlot], STEP_MS - 50);
            });
            j--;
        }

        arr[j + 1] = key;
        int insertSlot = j + 1;
        algoSteps.append([this, key, insertSlot, i]() {
            int idx = algoItems.indexOf(key);
            if (idx >= 0) {
                algoItems.removeAt(idx);
                algoItems.insert(insertSlot, key);
            }
            animateSlideTo(key, algoSlots[insertSlot], STEP_MS - 50);
            for (int k = 0; k <= i; k++) animateColorTo(algoItems[k], 0.5, 200);
        });
    }
}
