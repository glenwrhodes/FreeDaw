#include "PianoRollRuler.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>

namespace OpenDaw {

PianoRollRuler::PianoRollRuler(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Piano Roll Ruler");
    setFixedHeight(24);
    setCursor(Qt::PointingHandCursor);
}

double PianoRollRuler::xToBeat(int x) const
{
    double beat = (x + scrollX_) / pixelsPerBeat_;
    if (snapFn_) beat = snapFn_(beat);
    return std::max(0.0, beat);
}

void PianoRollRuler::paintEvent(QPaintEvent*)
{
    auto& theme = ThemeManager::instance().current();
    QPainter p(this);
    p.fillRect(rect(), theme.surface.darker(110));

    QFont f = font();
    f.setPixelSize(9);
    p.setFont(f);

    double beatsPerBar = timeSigNum_;
    double startBeat = scrollX_ / pixelsPerBeat_;
    int firstBar = static_cast<int>(std::floor(startBeat / beatsPerBar));
    if (firstBar < 0) firstBar = 0;
    double endBeat = startBeat + width() / pixelsPerBeat_;
    int lastBar = static_cast<int>(std::ceil(endBeat / beatsPerBar)) + 1;

    int h = height();

    for (int bar = firstBar; bar <= lastBar; ++bar) {
        double beat = bar * beatsPerBar;
        double x = beat * pixelsPerBeat_ - scrollX_;

        p.setPen(QPen(theme.textDim, 1));
        p.drawLine(QPointF(x, h - 7), QPointF(x, h));

        p.setPen(theme.text);
        p.drawText(QRectF(x + 3, 0, 50, h - 6),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(bar + 1));

        if (pixelsPerBeat_ > 15) {
            p.setPen(QPen(theme.border, 0.5));
            for (int b = 1; b < timeSigNum_; ++b) {
                double bx = (beat + b) * pixelsPerBeat_ - scrollX_;
                p.drawLine(QPointF(bx, h - 4), QPointF(bx, h));
            }
        }
    }

    double cx = cursorBeat_ * pixelsPerBeat_ - scrollX_;
    if (cx >= -6 && cx <= width() + 6) {
        QColor cursorColor(0, 220, 110);
        p.setPen(QPen(cursorColor, 2));
        p.drawLine(QPointF(cx, 0), QPointF(cx, h));

        QPainterPath tri;
        tri.moveTo(cx - 5, 0);
        tri.lineTo(cx + 5, 0);
        tri.lineTo(cx, 7);
        tri.closeSubpath();
        p.fillPath(tri, cursorColor);
    }

    p.setPen(theme.border);
    p.drawLine(0, h - 1, width(), h - 1);
}

void PianoRollRuler::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        double beat = xToBeat(event->pos().x());
        emit cursorPositionClicked(beat);
        event->accept();
    }
}

void PianoRollRuler::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_ && (event->buttons() & Qt::LeftButton)) {
        double beat = xToBeat(event->pos().x());
        emit cursorPositionDragged(beat);
        event->accept();
    }
}

void PianoRollRuler::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        event->accept();
    }
}

} // namespace OpenDaw
