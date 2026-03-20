#include "VelocityLane.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

namespace freedaw {

VelocityLane::VelocityLane(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Velocity Lane");
    setFixedHeight(60);
    setMouseTracking(true);
}

void VelocityLane::setClip(te::MidiClip* clip)
{
    clip_ = clip;
    update();
}

void VelocityLane::refresh()
{
    update();
}

std::vector<te::MidiNote*> VelocityLane::notesAtX(double x) const
{
    std::vector<te::MidiNote*> result;
    if (!clip_) return result;

    auto& seq = clip_->getSequence();
    for (auto* note : seq.getNotes()) {
        double barX = note->getStartBeat().inBeats() * pixelsPerBeat_ - scrollOffset_;
        double barW = std::max(4.0, note->getLengthBeats().inBeats() * pixelsPerBeat_);
        if (x >= barX && x <= barX + barW)
            result.push_back(note);
    }
    return result;
}

void VelocityLane::paintEvent(QPaintEvent*)
{
    auto& theme = ThemeManager::instance().current();
    QPainter painter(this);

    painter.fillRect(rect(), theme.pianoRollBackground);

    if (!clip_) return;

    auto& seq = clip_->getSequence();
    int h = height();

    for (auto* note : seq.getNotes()) {
        double x = note->getStartBeat().inBeats() * pixelsPerBeat_ - scrollOffset_;
        double w = std::max(3.0, note->getLengthBeats().inBeats() * pixelsPerBeat_);
        double barH = std::max(2.0, (note->getVelocity() / 127.0) * (h - 4));

        if (x + w < 0 || x > width()) continue;

        QRectF bar(x, h - barH - 2, w, barH);
        painter.fillRect(bar, theme.pianoRollVelocityBar);
        painter.setPen(QPen(theme.pianoRollVelocityBar.darker(130), 0.5));
        painter.drawRect(bar);
    }

    painter.setPen(QPen(theme.pianoRollGrid, 0.5));
    painter.drawLine(0, 0, width(), 0);
}

void VelocityLane::mousePressEvent(QMouseEvent* event)
{
    if (height() <= 0) return;
    draggingNotes_ = notesAtX(event->position().x());
    if (!draggingNotes_.empty()) {
        int vel = static_cast<int>(
            127.0 * (1.0 - event->position().y() / height()));
        vel = std::clamp(vel, 1, 127);
        auto* um = clip_ ? &clip_->edit.getUndoManager() : nullptr;
        for (auto* note : draggingNotes_)
            note->setVelocity(vel, um);
        update();
        emit velocityChanged();
    }
}

void VelocityLane::mouseMoveEvent(QMouseEvent* event)
{
    if (height() <= 0) return;
    if (!(event->buttons() & Qt::LeftButton)) return;

    auto notes = notesAtX(event->position().x());
    if (!notes.empty())
        draggingNotes_ = notes;

    if (draggingNotes_.empty()) return;

    int vel = static_cast<int>(
        127.0 * (1.0 - event->position().y() / height()));
    vel = std::clamp(vel, 1, 127);
    auto* um = clip_ ? &clip_->edit.getUndoManager() : nullptr;
    for (auto* note : draggingNotes_)
        note->setVelocity(vel, um);
    update();
    emit velocityChanged();
}

void VelocityLane::mouseReleaseEvent(QMouseEvent* /*event*/)
{
    draggingNotes_.clear();
}

} // namespace freedaw
