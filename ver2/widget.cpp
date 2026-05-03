#include "widget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QFontDatabase>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// ── 配色 ───────────────────────────────────────────────
static const QColor BG_COLOR      (237, 230, 219);   // 米白
static const QColor BAR_WHITE     (255, 255, 255);   // 静止色
static const QColor BAR_ORANGE    (204, 102,  68);   // 橙色（选中/悬停）
static const QColor BAR_GREEN     ( 88, 177, 120);   // 胜利绿
static const QColor TEXT_COLOR    (160, 140, 120);   // 数字颜色
static const int    RADIUS        = 8;               // 圆角半径
static const int    BAR_W         = 50;
static const int    BAR_GAP       = 18;
static const int    BOTTOM_MARGIN = 40;
static const int    LEFT_OFFSET   = 30;
static const int    BAR_COUNT     = 13;
// 窗口宽度根据柱子数量自动计算，两侧各留 LEFT_OFFSET
static const int    WIN_W         = LEFT_OFFSET * 2 + BAR_COUNT * (BAR_W + BAR_GAP) - BAR_GAP;
static const int    WIN_H         = 620;

// ── 构造 ───────────────────────────────────────────────
Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    this->resize(WIN_W, WIN_H);
    this->setMouseTracking(true); // 必须开启，才能收到无按键的 mouseMoveEvent

    srand(static_cast<unsigned>(time(nullptr)));

    // 初始化槽位
    initSlots();

    // value 范围与像素高度的映射
    const int VAL_MIN = 10;
    const int VAL_MAX = 99;
    const int H_MIN   = 40;
    const int H_MAX   = WIN_H - BOTTOM_MARGIN - 20;

    // 把 [VAL_MIN, VAL_MAX] 均分成 BAR_COUNT 段，每段内随机取一个值
    // 保证柱子之间高度差异足够明显
    QList<int> values;
    int segLen = (VAL_MAX - VAL_MIN) / BAR_COUNT;
    for (int i = 0; i < BAR_COUNT; i++)
        values.append(VAL_MIN + i * segLen + rand() % qMax(1, segLen));
    // Fisher-Yates 打乱，让排列随机
    for (int i = BAR_COUNT - 1; i > 0; i--)
        std::swap(values[i], values[rand() % (i + 1)]);

    for (int i = 0; i < BAR_COUNT; i++)
    {
        Rectitem* item = new Rectitem(this);
        item->width  = BAR_W;
        item->value  = values[i];
        // 高度与 value 严格线性对应，数字大 = 柱子高
        item->height = H_MIN + (qreal)(item->value - VAL_MIN)
                                   / (VAL_MAX - VAL_MIN) * (H_MAX - H_MIN);
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
}

Widget::~Widget()
{
    qDeleteAll(itemList);
}

// ── 槽位计算 ─────────────────────────────────────────
void Widget::initSlots()
{
    slotPositions.clear();
    for (int i = 0; i < BAR_COUNT; i++)
        slotPositions.append(LEFT_OFFSET + i * (BAR_W + BAR_GAP));
}

// ── 颜色混合 ─────────────────────────────────────────
// t: 0~1 混合比例, target: 目标色
QColor Widget::blendColor(qreal t) const
{
    // 胜利状态下目标色为绿，否则为橙
    const QColor& target = isSorted ? BAR_GREEN : BAR_ORANGE;
    int r = BAR_WHITE.red()   + t * (target.red()   - BAR_WHITE.red());
    int g = BAR_WHITE.green() + t * (target.green() - BAR_WHITE.green());
    int b = BAR_WHITE.blue()  + t * (target.blue()  - BAR_WHITE.blue());
    return QColor(r, g, b);
}

// ── 动画工具 ─────────────────────────────────────────
void Widget::animateSlideTo(Rectitem* item, qreal targetX, int duration)
{
    auto* anim = slideAnimations[item];
    if (!anim) return;
    if (qAbs(item->x - targetX) < 1.0) return; // 已到位，不重复播

    anim->stop();
    anim->setDuration(duration);
    anim->setStartValue(item->x);
    anim->setEndValue(targetX);
    anim->start();
}

void Widget::animateColorTo(Rectitem* item, qreal targetBlend, int duration)
{
    auto* anim = colorAnimations[item];
    if (!anim) return;
    if (qAbs(item->colorBlend - targetBlend) < 0.01) return;

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

    // 背景
    p.fillRect(rect(), BG_COLOR);

    // 底部分隔线（淡）
    p.setPen(QPen(QColor(200, 190, 178), 1));
    p.drawLine(LEFT_OFFSET - 10, WIN_H - BOTTOM_MARGIN,
               WIN_W - 20, WIN_H - BOTTOM_MARGIN);

    // 字体
    QFont font = p.font();
    font.setPointSize(9);
    font.setWeight(QFont::Medium);
    p.setFont(font);

    // 先画非拖动矩形，再画拖动矩形（保证拖动的在最上层）
    auto drawBar = [&](Rectitem* bar) {
        QColor color = blendColor(bar->colorBlend);

        // 圆角矩形路径（只有顶部两角圆角）
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

        // 拖动中加投影
        if (bar->dragging) {
            p.save();
            // 阴影
            QColor shadow(0, 0, 0, 40);
            for (int s = 8; s >= 1; s--) {
                QPainterPath sp = path;
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0, 0, 0, 5 * s));
                // 用 translate 偏移绘制阴影
                p.save();
                p.translate(s * 0.5, s * 0.8);
                p.drawPath(sp);
                p.restore();
            }
            p.restore();
        }

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(path);

        // 底部数字
        p.setPen(TEXT_COLOR);
        QRectF textRect(bar->x, WIN_H - BOTTOM_MARGIN + 8, bar->width, 20);
        p.drawText(textRect, Qt::AlignHCenter, QString::number(bar->value));
    };

    for (auto* bar : itemList)
        if (!bar->dragging) drawBar(bar);

    // 最后画被拖动的（置顶）
    for (auto* bar : itemList)
        if (bar->dragging) drawBar(bar);

    // ── 胜利播报 ──────────────────────────────────────
    if (victoryAlpha > 0.0) {
        // 半透明遮罩（很淡，不遮住柱子）
        QColor overlay(237, 230, 219, (int)(60 * victoryAlpha));
        p.fillRect(rect(), overlay);

        // 主标题
        QFont bigFont = p.font();
        bigFont.setPointSize(36);
        bigFont.setBold(true);
        bigFont.setLetterSpacing(QFont::AbsoluteSpacing, 4);
        p.setFont(bigFont);

        QColor titleColor(88, 177, 120, (int)(255 * victoryAlpha));
        p.setPen(titleColor);
        p.drawText(QRectF(0, WIN_H / 2 - 70, WIN_W, 60),
                   Qt::AlignHCenter, "排序完成！");

        // 副标题
        QFont smallFont = p.font();
        smallFont.setPointSize(13);
        smallFont.setBold(false);
        smallFont.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        p.setFont(smallFont);

        QColor subColor(120, 160, 130, (int)(220 * victoryAlpha));
        p.setPen(subColor);
        p.drawText(QRectF(0, WIN_H / 2 - 10, WIN_W, 40),
                   Qt::AlignHCenter, "从小到大，全部就位 ✓");
    }
}

// ── 鼠标按下 ─────────────────────────────────────────
void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    if (isSorted) return; // 胜利后禁止拖动
    selectedItem = nullptr;

    // 从后往前找（最顶层优先）
    for (int i = itemList.size() - 1; i >= 0; i--) {
        Rectitem* bar = itemList[i];
        QRectF r(bar->x, bar->y, bar->width, bar->height);
        if (r.contains(event->position())) {
            selectedItem = bar;
            dragOffsetX  = event->position().x() - bar->x;
            bar->dragging = true;
            // 停止位移动画（手动控制位置）
            slideAnimations[bar]->stop();
            // 立刻变橙
            animateColorTo(bar, 1.0, 120);
            break;
        }
    }
}

// ── 鼠标移动 ─────────────────────────────────────────
int Widget::targetIndexForDrag()
{
    // 拖动柱子的中点
    qreal dragCenter = selectedItem->x + selectedItem->width / 2.0;

    int target = 0;
    qreal minDist = qAbs(dragCenter - (slotPositions[0] + BAR_W / 2.0));

    for (int i = 1; i < slotPositions.size(); i++) {
        qreal slotCenter = slotPositions[i] + BAR_W / 2.0;
        qreal dist = qAbs(dragCenter - slotCenter);
        if (dist < minDist) {
            minDist = dist;
            target  = i;
        }
    }
    return target;
}

void Widget::rearrangeOthers()
{
    int targetIdx = targetIndexForDrag();

    // 构建不含 selectedItem 的排序列表
    QList<Rectitem*> others;
    for (auto* b : itemList)
        if (b != selectedItem) others.append(b);

    // 在 targetIdx 位置插入一个"占位"，其他矩形绕开
    // others 当前按 x 排序（近似）
    std::sort(others.begin(), others.end(), [](Rectitem* a, Rectitem* b){
        return a->x < b->x;
    });

    // 分配槽位：把 targetIdx 留给 selectedItem，其余按顺序填
    QList<int> freeSlots = slotPositions;
    // freeSlots 全部槽位，按顺序给 others（跳过 targetIdx 给 selected 用）
    int si = 0; // others 指针
    for (int slotI = 0; slotI < freeSlots.size() && si < others.size(); slotI++) {
        if (slotI == targetIdx) continue; // 留给 selected
        animateSlideTo(others[si], freeSlots[slotI], 180);
        si++;
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    // ── 拖动逻辑 ──
    if (selectedItem) {
        selectedItem->x = event->position().x() - dragOffsetX;
        // 限制不超出窗口
        selectedItem->x = qMax((qreal)0, qMin(selectedItem->x, (qreal)(WIN_W - BAR_W)));
        rearrangeOthers();
        update();
        return;
    }

    // ── 悬停高亮（无拖动时） ──
    if (isSorted) return; // 胜利后不再响应悬停
    for (int i = 0; i < itemList.size(); i++) {
        Rectitem* bar = itemList[i];
        QRectF r(bar->x, bar->y, bar->width, bar->height);
        bool hovered = r.contains(event->position());
        animateColorTo(bar, hovered ? 1.0 : 0.0, 80);
    }
}

// ── 鼠标释放 ─────────────────────────────────────────
void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !selectedItem) {
        selectedItem = nullptr;
        return;
    }

    int targetIdx = targetIndexForDrag();

    // 重建 itemList 顺序
    QList<Rectitem*> others;
    for (auto* b : itemList)
        if (b != selectedItem) others.append(b);

    std::sort(others.begin(), others.end(), [](Rectitem* a, Rectitem* b){
        return a->x < b->x;
    });

    others.insert(targetIdx, selectedItem);
    itemList = others;

    // 所有矩形平滑归位
    for (int i = 0; i < itemList.size(); i++)
        animateSlideTo(itemList[i], slotPositions[i], 220);

    selectedItem->dragging = false;
    // 释放后渐变回白色（如果鼠标不在上面且未胜利）
    if (!isSorted) {
        QRectF r(slotPositions[targetIdx], selectedItem->y, BAR_W, selectedItem->height);
        if (!r.contains(event->position()))
            animateColorTo(selectedItem, 0.0, 250);
    }

    selectedItem = nullptr;
    update();

    // 检测是否完成排序
    checkSorted();
}

// ── 排序检测 ─────────────────────────────────────────
void Widget::checkSorted()
{
    if (isSorted) return; // 已经胜利，不重复触发

    for (int i = 1; i < itemList.size(); i++) {
        if (itemList[i]->value < itemList[i-1]->value)
            return; // 未排好，直接返回
    }

    // 排好了！
    triggerVictory();
}

void Widget::triggerVictory()
{
    isSorted = true;

    // 等归位动画（220ms）跑完后再从最左端依次点绿
    for (int i = 0; i < itemList.size(); i++) {
        QTimer::singleShot(220 + i * 60, this, [this, i]() {
            if (i < itemList.size())
                animateColorTo(itemList[i], 1.0, 300);
        });
    }

    // 胜利文字在所有柱子点完后淡入
    int textDelay = 220 + itemList.size() * 60;
    QTimer::singleShot(textDelay, this, [this]() {
        victoryAlpha = 0.0;
        victoryAnim->stop();
        victoryAnim->setDuration(800);
        victoryAnim->setStartValue(0.0);
        victoryAnim->setEndValue(1.0);
        victoryAnim->start();
    });
}
