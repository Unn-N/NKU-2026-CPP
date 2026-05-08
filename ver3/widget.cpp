#include "widget.h"
#include "demowidget.h"
#include "racewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

// ── 配色 ─────────────────────────────────────────────
static const QColor BG_COLOR   (237, 230, 219);
static const QColor BTN_NORMAL (220, 210, 198);
static const QColor BTN_ACTIVE (204, 102,  68);
static const QColor BTN_HOVER  (230, 185, 160);
static const QColor TEXT_COLOR (160, 140, 120);

static const int NAV_H = 36;
static const int NAV_TOP = 8;

// ── 构造 ─────────────────────────────────────────────
Widget::Widget(QWidget *parent) : QWidget(parent)
{
    demoWidget = new DemoWidget(this);
    raceWidget = new RaceWidget(this);

    // 窗口宽度取两者最大值
    int w = qMax(demoWidget->width(), raceWidget->width());
    int h = qMax(demoWidget->height(), raceWidget->height()) + NAV_H + NAV_TOP * 2;
    resize(w, h);
    setMouseTracking(true);
    setMinimumSize(w, h);

    // 定位子模块
    int demoW = demoWidget->width();
    int raceW = raceWidget->width();
    demoWidget->move((w - demoW) / 2, NAV_H + NAV_TOP * 2);
    raceWidget->move((w - raceW) / 2, NAV_H + NAV_TOP * 2);

    // 按钮区域
    int btnW = 100, btnGap = 12;
    int btnY = NAV_TOP;
    int totalBtnW = btnW * 2 + btnGap;
    int startX = (w - totalBtnW) / 2;
    demoBtnRect = QRect(startX, btnY, btnW, NAV_H);
    raceBtnRect = QRect(startX + btnW + btnGap, btnY, btnW, NAV_H);

    switchTo(0);
}

// ── 切换 ─────────────────────────────────────────────
void Widget::switchTo(int module)
{
    activeModule = module;
    demoWidget->setVisible(module == 0);
    raceWidget->setVisible(module == 1);
    update();
}

// ── 绘制 ─────────────────────────────────────────────
void Widget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), BG_COLOR);

    auto drawTab = [&](const QRect& r, const QString& label, bool active) {
        QColor fill = active ? BTN_ACTIVE : BTN_NORMAL;
        QPainterPath bp;
        bp.addRoundedRect(r, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(bp);

        QFont f = p.font();
        f.setPointSize(10);
        f.setBold(active);
        p.setFont(f);
        p.setPen(active ? Qt::white : QColor(100, 80, 60));
        p.drawText(r, Qt::AlignCenter, label);
    };

    drawTab(demoBtnRect, "展示模式", activeModule == 0);
    drawTab(raceBtnRect, "比赛模式", activeModule == 1);
}

// ── 鼠标 ─────────────────────────────────────────────
void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    QPoint pos = event->position().toPoint();
    if (demoBtnRect.contains(pos)) { switchTo(0); return; }
    if (raceBtnRect.contains(pos)) { switchTo(1); return; }
}

void Widget::mouseMoveEvent(QMouseEvent *) { update(); }
