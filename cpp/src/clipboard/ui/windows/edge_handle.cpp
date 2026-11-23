#include "clipboard/ui/windows/edge_handle.h"
#include "core/ui_scale.h"
#include <QCursor>
#include <QEnterEvent>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <algorithm>

namespace jietuba::clipboard {
namespace {
// The strip is a sliver of the edge, not a bar: wide enough to see and to hit, narrow enough that
// it never stands in for a window of its own.
constexpr double STRIP_THICKNESS = 6.0;
constexpr double STRIP_RADIUS = 2.5;
// How far past the strip the pointer still counts as reaching it.
constexpr int HIT_SLACK = 2;
}

EdgeHandle::EdgeHandle(QWidget *parent) : QWidget(parent) {
    // No activation and no focus, ever: the strip stands for a hidden window and must not take the
    // keyboard from the one the user is typing in, nor answer Esc.
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::ArrowCursor);
    setWindowTitle(QString());
}

void EdgeHandle::set_accent(const QColor &accent) {
    accent_ = accent;
    update();
}

void EdgeHandle::place(const QString &edge, const QRect &docked) {
    edge_ = edge;
    docked_ = docked;
    const QScreen *screen = QGuiApplication::screenAt(docked.center());
    const QRect available = screen ? screen->availableGeometry() : QRect();
    if (available.isEmpty()) return;
    const int thickness = scaled(STRIP_THICKNESS);
    QRect strip;
    if (edge == "left") strip = QRect(available.left(), docked.top(), thickness, docked.height());
    else if (edge == "right") strip = QRect(available.right() + 1 - thickness, docked.top(), thickness, docked.height());
    else if (edge == "top") strip = QRect(docked.left(), available.top(), docked.width(), thickness);
    else strip = QRect(docked.left(), available.bottom() + 1 - thickness, docked.width(), thickness);
    setGeometry(strip);
    update();
}

QRect EdgeHandle::hit_rect() const {
    // Global coordinates: the strip is its own top-level window.
    const int slack = scaled(HIT_SLACK);
    return geometry().adjusted(-slack, -slack, slack, slack);
}

void EdgeHandle::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const double radius = scaled(STRIP_RADIUS);
    QColor strip = accent_;
    strip.setAlpha(70);
    painter.setPen(Qt::NoPen);
    painter.setBrush(strip);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0, 0, -0.5, -0.5), radius, radius);
    // A grip in the middle of the span: something to aim at, and the only part at full strength.
    QColor grip = accent_;
    grip.setAlpha(215);
    painter.setBrush(grip);
    if (edge_ == "left" || edge_ == "right") {
        const int length = std::max(scaled(18), height() * 2 / 5);
        painter.drawRoundedRect(QRectF(1, (height() - length) / 2.0, width() - 2.0, length), radius, radius);
    } else {
        const int length = std::max(scaled(18), width() * 2 / 5);
        painter.drawRoundedRect(QRectF((width() - length) / 2.0, 1, length, height() - 2.0), radius, radius);
    }
}

void EdgeHandle::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    // Docking leaves the pointer on the strip: it has to be left once before hovering it counts,
    // or the window would come back on the first mouse move after it was put away.
    armed_ = !hit_rect().contains(QCursor::pos());
}

void EdgeHandle::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    if (armed_) emit hovered();
}

void EdgeHandle::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    armed_ = true;
}

QString edge_near(const QRect &frame, const QRect &available, int reach) {
    if (available.isEmpty() || frame.isEmpty()) return {};
    const int left = frame.left() - available.left();
    const int right = available.right() - frame.right();
    const int top = frame.top() - available.top();
    const int bottom = available.bottom() - frame.bottom();
    const int nearest = std::min({left, right, top, bottom});
    // A frame already over a side rests against it; the taskbar's side is the work area's, not the
    // screen's, so a docked window never ends up under the taskbar.
    if (nearest > reach) return {};
    if (nearest == left) return "left";
    if (nearest == right) return "right";
    if (nearest == top) return "top";
    return "bottom";
}
}
