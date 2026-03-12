#include "TimeRuler.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>

namespace freedaw {

TimeRuler::TimeRuler(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Time Ruler");
    setFixedHeight(28);
    setCursor(Qt::PointingHandCursor);
}

double TimeRuler::xToBeat(int x) const
{
    double beat = (x + scrollX_) / pixelsPerBeat_;
    return std::max(0.0, beat);
}

void TimeRuler::paintEvent(QPaintEvent*)
{
    auto& theme = ThemeManager::instance().current();
    QPainter p(this);
    p.fillRect(rect(), theme.surface);

    QFont f = font();
    f.setPixelSize(10);
    p.setFont(f);

    double beatsPerBar = timeSigNum_;

    double startBeat = scrollX_ / pixelsPerBeat_;
    int firstBar = int(std::floor(startBeat / beatsPerBar));
    if (firstBar < 0) firstBar = 0;

    double endBeat = startBeat + width() / pixelsPerBeat_;
    int lastBar = int(std::ceil(endBeat / beatsPerBar)) + 1;

    for (int bar = firstBar; bar <= lastBar; ++bar) {
        double beat = bar * beatsPerBar;
        double x = beat * pixelsPerBeat_ - scrollX_;

        p.setPen(QPen(theme.textDim, 1));
        p.drawLine(QPointF(x, height() - 8), QPointF(x, height()));

        p.setPen(theme.text);
        p.drawText(QRectF(x + 3, 0, 60, height() - 8),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(bar + 1));

        if (pixelsPerBeat_ > 15) {
            p.setPen(QPen(theme.border, 0.5));
            for (int b = 1; b < timeSigNum_; ++b) {
                double bx = (beat + b) * pixelsPerBeat_ - scrollX_;
                p.drawLine(QPointF(bx, height() - 5), QPointF(bx, height()));
            }
        }
    }

    double phX = playheadBeat_ * pixelsPerBeat_ - scrollX_;
    if (phX >= 0 && phX <= width()) {
        p.setPen(QPen(theme.playhead, 2));
        p.drawLine(QPointF(phX, 0), QPointF(phX, height()));

        QPainterPath tri;
        tri.moveTo(phX - 5, 0);
        tri.lineTo(phX + 5, 0);
        tri.lineTo(phX, 7);
        tri.closeSubpath();
        p.fillPath(tri, theme.playhead);
    }

    p.setPen(theme.border);
    p.drawLine(0, height() - 1, width(), height() - 1);
}

void TimeRuler::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        double beat = xToBeat(event->pos().x());
        emit positionClicked(beat);
        event->accept();
    }
}

void TimeRuler::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        dragging_ = true;
        double beat = xToBeat(event->pos().x());
        emit positionDragged(beat);
        event->accept();
    }
}

void TimeRuler::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        event->accept();
    }
}

} // namespace freedaw
