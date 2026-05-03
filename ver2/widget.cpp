#include "widget.h"
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
static const QColor BAR_ORANGE (204, 102,  68);
static const QColor BAR_GREEN  ( 88, 177, 120);
static const QColor TEXT_COLOR (160, 140, 120);
static const QColor BTN_NORMAL (220, 210, 198);
static const QColor BTN_ACTIVE (204, 102,  68);
static const QColor BTN_HOVER  (230, 185, 160);

static const int RADIUS      = 8;
static const int BAR_W       = 50;
static const int BAR_GAP     = 18;
static const int LEFT_OFFSET = 30;
static const int BAR_COUNT   = 13;
static const int WIN_W       = LEFT_OFFSET * 2 + BAR_COUNT * (BAR_W + BAR_GAP) - BAR_GAP;
static const int NUM_AREA    = 30;
static const int BTN_AREA    = 60;
static const int BOTTOM_MARGIN = NUM_AREA + BTN_AREA;
static const int WIN_H       = 660;
static const int STEP_MS     = 350;

// ── 构造 ─────────────────────────────────────────────
Widget::Widget(QWidget *parent) : QWidget(parent)
{
    resize(WIN_W, WIN_H);
    setMouseTracking(true);
    srand(static_cast<unsigned>(time(nullptr)));

    initSlots();
    initButtons();

    const int VAL_MIN = 10, VAL_MAX = 99;
    const int H_MIN = 40, H_MAX = WIN_H - BOTTOM_MARGIN - 20;
    QList<int> values;
    int segLen = (VAL_MAX - VAL_MIN) / BAR_COUNT;
    for (int i = 0; i < BAR_COUNT; i++)
        values.append(VAL_MIN + i * segLen + rand() % qMax(1, segLen));
    for (int i = BAR_COUNT - 1; i > 0; i--)
        std::swap(values[i], values[rand() % (i + 1)]);

    for (int i = 0; i < BAR_COUNT; i++) {
        Rectitem* item = new Rectitem(this);
        item->width  = BAR_W;
        item->value  = values[i];
        item->height = H_MIN + (qreal)(item->value - VAL_MIN) / (VAL_MAX - VAL_MIN) * (H_MAX - H_MIN);
        item->x      = slotPositions[i];
        item->y      = WIN_H - BOTTOM_MARGIN - item->height;
        itemList.append(item);

        auto* sa = new QPropertyAnimation(item, "x", this);
        sa->setEasingCurve(QEasingCurve::OutCubic);
        slideAnimations[item] = sa;
        connect(sa, &QPropertyAnimation::valueChanged, this, [this](const QVariant&){ update(); });

        auto* ca = new QPropertyAnimation(item, "colorBlend", this);
        ca->setEasingCurve(QEasingCurve::InOutQuad);
        colorAnimations[item] = ca;
        connect(ca, &QPropertyAnimation::valueChanged, this, [this](const QVariant&){ update(); });
    }

    victoryAnim = new QPropertyAnimation(this, "victoryAlpha", this);
    victoryAnim->setEasingCurve(QEasingCurve::OutCubic);

    stepTimer = new QTimer(this);
    stepTimer->setSingleShot(false);
}

Widget::~Widget() { qDeleteAll(itemList); }

// ── 槽位 ─────────────────────────────────────────────
void Widget::initSlots()
{
    slotPositions.clear();
    for (int i = 0; i < BAR_COUNT; i++)
        slotPositions.append(LEFT_OFFSET + i * (BAR_W + BAR_GAP));
}

// ── 按钮 ─────────────────────────────────────────────
void Widget::initButtons()
{
    buttons.clear();
    QStringList labels = {
        "冒泡排序",
        "选择排序",
        "插入排序",
        "基于视觉感知的\n人机协同排序算法"
    };
    QList<Mode> modes = { Mode::Bubble, Mode::Selection, Mode::Insertion, Mode::Manual };

    int btnCount = 4;
    int btnH     = 38;
    int totalW   = WIN_W - LEFT_OFFSET * 2;
    int btnW     = (totalW - (btnCount - 1) * 10) / btnCount;
    int btnY     = WIN_H - BTN_AREA + (BTN_AREA - btnH) / 2;

    for (int i = 0; i < btnCount; i++) {
        Button b;
        b.rect    = QRectF(LEFT_OFFSET + i * (btnW + 10), btnY, btnW, btnH);
        b.label   = labels[i];
        b.mode    = modes[i];
        b.hovered = false;
        buttons.append(b);
    }
}

// ── 重置 ─────────────────────────────────────────────
void Widget::resetBars()
{
    stepTimer->stop();
    disconnect(stepTimer, &QTimer::timeout, this, nullptr);
    algoSteps.clear();
    algoRunning  = false;
    isSorted     = false;
    victoryAlpha = 0.0;
    victoryAnim->stop();
    selectedItem = nullptr;

    const int VAL_MIN = 10, VAL_MAX = 99;
    const int H_MIN = 40, H_MAX = WIN_H - BOTTOM_MARGIN - 20;
    QList<int> values;
    int segLen = (VAL_MAX - VAL_MIN) / BAR_COUNT;
    for (int i = 0; i < BAR_COUNT; i++)
        values.append(VAL_MIN + i * segLen + rand() % qMax(1, segLen));
    for (int i = BAR_COUNT - 1; i > 0; i--)
        std::swap(values[i], values[rand() % (i + 1)]);

    for (int i = 0; i < BAR_COUNT; i++) {
        Rectitem* item = itemList[i];
        slideAnimations[item]->stop();
        colorAnimations[item]->stop();
        item->value      = values[i];
        item->height     = H_MIN + (qreal)(item->value - VAL_MIN) / (VAL_MAX - VAL_MIN) * (H_MAX - H_MIN);
        item->x          = slotPositions[i];
        item->y          = WIN_H - BOTTOM_MARGIN - item->height;
        item->colorBlend = 0.0;
        item->dragging   = false;
    }
    update();
}

// ── 模式切换 ─────────────────────────────────────────
void Widget::switchMode(Mode m)
{
    // 移除 algoRunning 守卫，任何时候都可以切换（resetBars会停止当前演示）
    resetBars();
    currentMode = m;
    if (m == Mode::Bubble)    { buildBubbleSteps();    runAlgoSteps(); }
    if (m == Mode::Selection) { buildSelectionSteps(); runAlgoSteps(); }
    if (m == Mode::Insertion) { buildInsertionSteps(); runAlgoSteps(); }
}

// ── 颜色 ─────────────────────────────────────────────
QColor Widget::blendColor(qreal t) const
{
    const QColor& target = isSorted ? BAR_GREEN : BAR_ORANGE;
    return QColor(
        BAR_WHITE.red()   + t * (target.red()   - BAR_WHITE.red()),
        BAR_WHITE.green() + t * (target.green() - BAR_WHITE.green()),
        BAR_WHITE.blue()  + t * (target.blue()  - BAR_WHITE.blue())
        );
}

// ── 动画 ─────────────────────────────────────────────
void Widget::animateSlideTo(Rectitem* item, qreal targetX, int duration)
{
    auto* anim = slideAnimations[item];
    if (!anim || qAbs(item->x - targetX) < 1.0) return;
    anim->stop();
    anim->setDuration(duration);
    anim->setStartValue(item->x);
    anim->setEndValue(targetX);
    anim->start();
}

void Widget::animateColorTo(Rectitem* item, qreal targetBlend, int duration)
{
    auto* anim = colorAnimations[item];
    if (!anim || qAbs(item->colorBlend - targetBlend) < 0.01) return;
    anim->stop();
    anim->setDuration(duration);
    anim->setStartValue(item->colorBlend);
    anim->setEndValue(targetBlend);
    anim->start();
}

// ── 绘制 ─────────────────────────────────────────────
void Widget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), BG_COLOR);

    // 分隔线
    p.setPen(QPen(QColor(200, 190, 178), 1));
    p.drawLine(LEFT_OFFSET - 10, WIN_H - BOTTOM_MARGIN,
               WIN_W - LEFT_OFFSET + 10, WIN_H - BOTTOM_MARGIN);
    p.setPen(QPen(QColor(210, 200, 188), 1));
    p.drawLine(0, WIN_H - BTN_AREA, WIN_W, WIN_H - BTN_AREA);

    QFont font = p.font();
    font.setPointSize(9);
    font.setWeight(QFont::Medium);
    p.setFont(font);

    // ── 柱子 ──
    auto drawBar = [&](Rectitem* bar) {
        QColor color = blendColor(bar->colorBlend);
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

    for (auto* bar : itemList) if (!bar->dragging) drawBar(bar);
    for (auto* bar : itemList) if ( bar->dragging) drawBar(bar);

    // ── 按钮 ──
    for (auto& btn : buttons) {
        bool isActive = (btn.mode == currentMode);
        QColor fill = isActive ? BTN_ACTIVE : (btn.hovered ? BTN_HOVER : BTN_NORMAL);

        QPainterPath bp;
        bp.addRoundedRect(btn.rect, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(bp);

        QFont bf = p.font();
        bf.setPointSize(btn.mode == Mode::Manual ? 8 : 9);
        bf.setBold(isActive);
        p.setFont(bf);
        p.setPen(isActive ? Qt::white : QColor(100, 80, 60));
        p.drawText(btn.rect, Qt::AlignCenter, btn.label);
    }
    p.setFont(font); // 恢复字体

    // ── 算法模式提示（顶部）──
    if (currentMode != Mode::Manual && !isSorted && algoRunning) {
        QFont hf = p.font();
        hf.setPointSize(9);
        p.setFont(hf);
        p.setPen(QColor(160, 130, 100));
        p.drawText(QRectF(0, 10, WIN_W, 20), Qt::AlignHCenter, "演示中...");
        p.setFont(font);
    }

    // ── 胜利播报 ──
    if (victoryAlpha > 0.0) {
        p.fillRect(rect(), QColor(237, 230, 219, (int)(60 * victoryAlpha)));

        QFont big = p.font();
        big.setPointSize(36);
        big.setBold(true);
        big.setLetterSpacing(QFont::AbsoluteSpacing, 4);
        p.setFont(big);
        p.setPen(QColor(88, 177, 120, (int)(255 * victoryAlpha)));
        p.drawText(QRectF(0, WIN_H / 2 - 70, WIN_W, 60), Qt::AlignHCenter, "排序完成！");

        QFont small = p.font();
        small.setPointSize(13);
        small.setBold(false);
        small.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        p.setFont(small);
        p.setPen(QColor(120, 160, 130, (int)(220 * victoryAlpha)));
        p.drawText(QRectF(0, WIN_H / 2 - 10, WIN_W, 40), Qt::AlignHCenter, "从小到大，全部就位 ✓");
    }
}

// ── 鼠标按下 ─────────────────────────────────────────
void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    // 检查按钮点击
    for (auto& btn : buttons) {
        if (btn.rect.contains(event->position())) {
            switchMode(btn.mode);
            return;
        }
    }

    // 算法运行中或非手动模式或已胜利，不响应拖拽
    if (algoRunning || currentMode != Mode::Manual || isSorted) return;

    selectedItem = nullptr;
    for (int i = itemList.size() - 1; i >= 0; i--) {
        Rectitem* bar = itemList[i];
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

// ── 鼠标移动 ─────────────────────────────────────────
int Widget::targetIndexForDrag()
{
    qreal dragCenter = selectedItem->x + selectedItem->width / 2.0;
    int target = 0;
    qreal minDist = qAbs(dragCenter - (slotPositions[0] + BAR_W / 2.0));
    for (int i = 1; i < slotPositions.size(); i++) {
        qreal dist = qAbs(dragCenter - (slotPositions[i] + BAR_W / 2.0));
        if (dist < minDist) { minDist = dist; target = i; }
    }
    return target;
}

void Widget::rearrangeOthers()
{
    int targetIdx = targetIndexForDrag();
    QList<Rectitem*> others;
    for (auto* b : itemList) if (b != selectedItem) others.append(b);
    std::sort(others.begin(), others.end(), [](Rectitem* a, Rectitem* b){ return a->x < b->x; });

    int si = 0;
    for (int slotI = 0; slotI < slotPositions.size() && si < others.size(); slotI++) {
        if (slotI == targetIdx) continue;
        animateSlideTo(others[si], slotPositions[slotI], 180);
        si++;
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    // 按钮悬停
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

    if (isSorted || currentMode != Mode::Manual) { update(); return; }

    for (auto* bar : itemList) {
        bool hovered = QRectF(bar->x, bar->y, bar->width, bar->height).contains(event->position());
        animateColorTo(bar, hovered ? 1.0 : 0.0, 80);
    }
    update();
}

// ── 鼠标释放 ─────────────────────────────────────────
void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !selectedItem) {
        selectedItem = nullptr;
        return;
    }

    int targetIdx = targetIndexForDrag();
    QList<Rectitem*> others;
    for (auto* b : itemList) if (b != selectedItem) others.append(b);
    std::sort(others.begin(), others.end(), [](Rectitem* a, Rectitem* b){ return a->x < b->x; });
    others.insert(targetIdx, selectedItem);
    itemList = others;

    for (int i = 0; i < itemList.size(); i++)
        animateSlideTo(itemList[i], slotPositions[i], 220);

    selectedItem->dragging = false;
    if (!isSorted) {
        QRectF r(slotPositions[targetIdx], selectedItem->y, BAR_W, selectedItem->height);
        if (!r.contains(event->position()))
            animateColorTo(selectedItem, 0.0, 250);
    }
    selectedItem = nullptr;
    update();
    checkSorted();
}

// ── 排序检测 ─────────────────────────────────────────
void Widget::checkSorted()
{
    if (isSorted) return;
    for (int i = 1; i < itemList.size(); i++)
        if (itemList[i]->value < itemList[i-1]->value) return;
    triggerVictory();
}

void Widget::triggerVictory()
{
    isSorted    = true;
    algoRunning = false;
    stepTimer->stop();
    disconnect(stepTimer, &QTimer::timeout, this, nullptr);

    for (int i = 0; i < itemList.size(); i++) {
        QTimer::singleShot(220 + i * 60, this, [this, i]() {
            if (i < itemList.size()) animateColorTo(itemList[i], 1.0, 300);
        });
    }
    QTimer::singleShot(220 + itemList.size() * 60, this, [this]() {
        victoryAlpha = 0.0;
        victoryAnim->stop();
        victoryAnim->setDuration(800);
        victoryAnim->setStartValue(0.0);
        victoryAnim->setEndValue(1.0);
        victoryAnim->start();
    });
}

// ── 算法步骤播放 ─────────────────────────────────────
void Widget::runAlgoSteps()
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
            QTimer::singleShot(STEP_MS, this, [this](){ checkSorted(); });
            return;
        }
        algoSteps[(*idx)++]();
    });
    stepTimer->start(STEP_MS);
}

// ── 冒泡排序 ─────────────────────────────────────────
void Widget::buildBubbleSteps()
{
    algoSteps.clear();
    QList<Rectitem*> arr = itemList;
    int n = arr.size();

    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            Rectitem* a = arr[j];
            Rectitem* b = arr[j+1];
            // 高亮比较的两根
            algoSteps.append([this, a, b, i, n]() {
                for (int k = n - 1 - i + 1; k < n; k++) animateColorTo(itemList[k], 0.5, 80);
                for (auto* bar : itemList) if (bar->colorBlend < 0.4) animateColorTo(bar, 0.0, 80);
                animateColorTo(a, 1.0, 100);
                animateColorTo(b, 1.0, 100);
            });

            if (arr[j]->value > arr[j+1]->value) {
                std::swap(arr[j], arr[j+1]);
                Rectitem* left  = arr[j];
                Rectitem* right = arr[j+1];
                int ls = j, rs = j + 1;
                algoSteps.append([this, left, right, ls, rs]() {
                    int ia = itemList.indexOf(left), ib = itemList.indexOf(right);
                    if (ia >= 0 && ib >= 0) std::swap(itemList[ia], itemList[ib]);
                    animateSlideTo(left,  slotPositions[ls], STEP_MS - 50);
                    animateSlideTo(right, slotPositions[rs], STEP_MS - 50);
                });
            }
        }
        // 最右端已就位，变淡绿
        Rectitem* done = arr[n - 1 - i];
        algoSteps.append([this, done]() { animateColorTo(done, 0.5, 200); });
    }
    Rectitem* last = arr[0];
    algoSteps.append([this, last]() { animateColorTo(last, 0.5, 200); });
}

// ── 选择排序 ─────────────────────────────────────────
void Widget::buildSelectionSteps()
{
    algoSteps.clear();
    int n = itemList.size();

    // pos[i] 表示柱子 itemList[i] 当前在哪个槽位
    QList<int> pos;
    for (int i = 0; i < n; i++) pos.append(i);

    // slot[j] 表示槽位 j 上现在是哪个柱子（索引）
    QList<int> slot;
    for (int i = 0; i < n; i++) slot.append(i);

    for (int i = 0; i < n - 1; i++) {
        // 找 [i, n) 中值最小的柱子
        int minSlot = i;
        for (int j = i + 1; j < n; j++)
            if (itemList[slot[j]]->value < itemList[slot[minSlot]]->value)
                minSlot = j;

        // 扫描高亮
        int curMinSlot = i;
        for (int j = i + 1; j < n; j++) {
            int scanBarIdx   = slot[j];
            int curMinBarIdx = slot[curMinSlot];
            int ii = i;
            algoSteps.append([this, scanBarIdx, curMinBarIdx, ii]() {
                for (auto* bar : itemList) animateColorTo(bar, 0.0, 60);
                for (int k = 0; k < ii; k++) animateColorTo(itemList[k], 0.5, 60);
                animateColorTo(itemList[scanBarIdx],   1.0, 100);
                animateColorTo(itemList[curMinBarIdx], 0.7, 100);
            });
            if (itemList[slot[j]]->value < itemList[slot[curMinSlot]]->value)
                curMinSlot = j;
        }

        // 如果最小值不在槽位 i，交换
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
                animateColorTo(itemList[barA], 1.0, 100);
                animateColorTo(itemList[barB], 1.0, 100);
                animateSlideTo(itemList[barA], slotPositions[targetA], STEP_MS - 50);
                animateSlideTo(itemList[barB], slotPositions[targetB], STEP_MS - 50);
            });
        }

        // 本轮结束，槽位 [0,i] 上的柱子变淡绿
        QList<int> doneIndices;
        for (int k = 0; k <= i; k++) doneIndices.append(slot[k]);
        algoSteps.append([this, doneIndices]() {
            for (auto* bar : itemList) animateColorTo(bar, 0.0, 60);
            for (int idx : doneIndices) animateColorTo(itemList[idx], 0.5, 200);
        });
    }

    // 最后一根就位
    int lastIdx = slot[n - 1];
    algoSteps.append([this, lastIdx]() {
        animateColorTo(itemList[lastIdx], 0.5, 200);
    });

    // 同步 itemList 顺序，让 checkSorted 能正确检测
    QList<int> finalSlot = slot;
    algoSteps.append([this, finalSlot]() {
        QList<Rectitem*> sorted(itemList.size());
        for (int j = 0; j < finalSlot.size(); j++)
            sorted[j] = itemList[finalSlot[j]];
        itemList = sorted;
    });
}

// ── 插入排序 ─────────────────────────────────────────
void Widget::buildInsertionSteps()
{
    algoSteps.clear();
    QList<Rectitem*> arr = itemList;
    int n = arr.size();

    for (int i = 1; i < n; i++) {
        Rectitem* key = arr[i];
        int j = i - 1;
        int ii = i;

        algoSteps.append([this, key, ii]() {
            for (auto* bar : itemList) animateColorTo(bar, 0.0, 80);
            for (int k = 0; k < ii; k++) animateColorTo(itemList[k], 0.5, 80);
            animateColorTo(key, 1.0, 100);
        });

        while (j >= 0 && arr[j]->value > key->value) {
            arr[j + 1] = arr[j];
            Rectitem* moving = arr[j];
            int toSlot = j + 1;
            algoSteps.append([this, moving, toSlot]() {
                int idx = itemList.indexOf(moving);
                if (idx >= 0 && toSlot < itemList.size()) {
                    itemList.removeAt(idx);
                    itemList.insert(toSlot, moving);
                }
                animateSlideTo(moving, slotPositions[toSlot], STEP_MS - 50);
            });
            j--;
        }

        arr[j + 1] = key;
        int insertSlot = j + 1;
        algoSteps.append([this, key, insertSlot, ii]() {
            int idx = itemList.indexOf(key);
            if (idx >= 0) {
                itemList.removeAt(idx);
                itemList.insert(insertSlot, key);
            }
            animateSlideTo(key, slotPositions[insertSlot], STEP_MS - 50);
            for (int k = 0; k <= ii; k++) animateColorTo(itemList[k], 0.5, 200);
        });
    }
}
