#pragma once
#include <QColor>
#include <QRect>
#include <QString>
#include <QWidget>

namespace jietuba::clipboard {

// The strip left against a screen edge while the clipboard window is docked and tucked away.
//
// It is the whole of the docked window as far as the desktop is concerned: the window itself is
// hidden, so no key reaches the clipboard while it is tucked away and the window the user is
// typing in keeps the keyboard. The strip therefore never activates and never takes focus, even
// when clicked; it only says where the window is and gives the pointer something to arrive on.
class EdgeHandle : public QWidget {
    Q_OBJECT
public:
    explicit EdgeHandle(QWidget *parent = nullptr);

    // Lays the strip against `edge` of the work area, across the span of `docked`, the rectangle
    // the window occupies while it is out.
    void place(const QString &edge, const QRect &docked);
    QString edge() const { return edge_; }
    // Where the pointer must be for the strip to count as reached: the strip itself, widened along
    // the edge so a quick move onto it is not missed.
    QRect hit_rect() const;
    void set_accent(const QColor &accent);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;

signals:
    // The pointer reached the strip: bring the window back.
    void hovered();

private:
    QString edge_ = "left";
    QRect docked_;
    QColor accent_ = QColor(111, 143, 171);
};

// The shortest distance from `frame` to each side of `available`, and which side is nearest when
// that distance is within `reach`; empty when the frame rests away from every side. A frame that
// already overlaps a side counts as resting against it.
QString edge_near(const QRect &frame, const QRect &available, int reach);

}
