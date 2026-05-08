#include "widget.h"
#include "demowidget.h"
#include "racewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>

// ── 配色 ─────────────────────────────────────────────
static const QColor BG_COLOR   (237, 230, 219);
static const QColor BTN_NORMAL (220, 210, 198);
static const QColor BTN_ACTIVE (204, 102,  68);
static const QColor BTN_HOVER  (230, 185, 160);

static const int NAV_H = 36;
static const int NAV_TOP = 8;

// ── 构造 ─────────────────────────────────────────────
Widget::Widget(QWidget *parent) : QWidget(parent)
{
    demoWidget = new DemoWidget(this);
    raceWidget = new RaceWidget(this);

    setMouseTracking(true);
    switchTo(0);
}

// ── 切换 ─────────────────────────────────────────────
void Widget::switchTo(int module)
{
    activeModule = module;

    QWidget* child;
    int childW, childH;
    if (module == 0) {
        child = demoWidget;
        childW = demoWidget->width();
        childH = demoWidget->height();
        raceWidget->hide();
    } else {
        child = raceWidget;
        childW = raceWidget->width();
        childH = raceWidget->height();
        demoWidget->hide();
    }

    // 窗口大小
    int navArea = NAV_H + NAV_TOP * 2;
    int w = childW;
    int h = childH + navArea;
    resize(w, h);

    // 子模块居中定位
    child->move(0, navArea);
    child->setVisible(true);

    // 导航按钮居中
    int btnW = 100, btnGap = 12;
    int btnY = NAV_TOP;
    int totalBtnW = btnW * 2 + btnGap;
    int startX = (w - totalBtnW) / 2;
    demoBtnRect = QRect(startX, btnY, btnW, NAV_H);
    raceBtnRect = QRect(startX + btnW + btnGap, btnY, btnW, NAV_H);

    // 窗口居中显示
    if (auto* screen = QApplication::primaryScreen()) {
        QRect screenRect = screen->availableGeometry();
        move((screenRect.width() - w) / 2, (screenRect.height() - h) / 2);
    }

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
