#include "PianoKeyboard.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <QMouseEvent>

namespace freedaw {

PianoKeyboard::PianoKeyboard(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Piano Keyboard");
    setFixedWidth(KEY_WIDTH);
}

QSize PianoKeyboard::sizeHint() const
{
    return QSize(KEY_WIDTH, static_cast<int>(TOTAL_NOTES * noteRowHeight_));
}

bool PianoKeyboard::isBlackKey(int note)
{
    int n = note % 12;
    return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
}

QString PianoKeyboard::noteName(int note)
{
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    int octave = (note / 12) - 1;
    return QString("%1%2").arg(names[note % 12]).arg(octave);
}

int PianoKeyboard::noteFromY(double y) const
{
    double adjustedY = y + scrollOffset_;
    int row = static_cast<int>(adjustedY / noteRowHeight_);
    return (TOTAL_NOTES - 1) - row;
}

void PianoKeyboard::paintEvent(QPaintEvent*)
{
    auto& theme = ThemeManager::instance().current();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    int h = height();

    for (int note = 0; note < TOTAL_NOTES; ++note) {
        int row = (TOTAL_NOTES - 1) - note;
        double y = row * noteRowHeight_ - scrollOffset_;

        if (y + noteRowHeight_ < 0 || y > h) continue;

        QRectF r(0, y, KEY_WIDTH, noteRowHeight_);

        bool black = isBlackKey(note);

        QColor baseColor = black ? theme.pianoKeyBlack : theme.pianoKeyWhite;
        QColor highlight = black ? QColor(70, 70, 70) : QColor(255, 255, 255);
        QColor shadow    = black ? QColor(10, 10, 10)  : QColor(175, 175, 175);

        painter.fillRect(r, baseColor);

        painter.setPen(QPen(highlight, 1.0));
        painter.drawLine(QPointF(r.left(), r.top()), QPointF(r.right() - 1, r.top()));
        painter.drawLine(QPointF(r.left(), r.top()), QPointF(r.left(), r.bottom() - 1));

        painter.setPen(QPen(shadow, 1.0));
        painter.drawLine(QPointF(r.left(), r.bottom() - 1), QPointF(r.right() - 1, r.bottom() - 1));
        painter.drawLine(QPointF(r.right() - 1, r.top()), QPointF(r.right() - 1, r.bottom() - 1));

        if (note % 12 == 0) {
            painter.setPen(black ? Qt::white : QColor(40, 40, 40));
            QFont f = painter.font();
            f.setPixelSize(std::max(8, static_cast<int>(noteRowHeight_ - 3)));
            f.setBold(true);
            painter.setFont(f);
            painter.drawText(r.adjusted(3, 0, 0, 0),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             noteName(note));
        }
    }

    painter.setPen(QPen(theme.pianoKeyBorder, 1.0));
    painter.drawLine(KEY_WIDTH - 1, 0, KEY_WIDTH - 1, h);
}

void PianoKeyboard::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    int note = noteFromY(event->position().y());
    if (note >= 0 && note < TOTAL_NOTES) {
        pressedNote_ = note;
        emit noteClicked(note);
    }
}

void PianoKeyboard::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    if (pressedNote_ >= 0) {
        emit noteReleased(pressedNote_);
        pressedNote_ = -1;
    }
}

void PianoKeyboard::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton) || pressedNote_ < 0) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    int note = noteFromY(event->position().y());
    note = std::clamp(note, 0, TOTAL_NOTES - 1);
    if (note != pressedNote_) {
        emit noteReleased(pressedNote_);
        pressedNote_ = note;
        emit noteClicked(note);
    }
}

void PianoKeyboard::leaveEvent(QEvent* event)
{
    if (pressedNote_ >= 0) {
        emit noteReleased(pressedNote_);
        pressedNote_ = -1;
    }
    QWidget::leaveEvent(event);
}

} // namespace freedaw
