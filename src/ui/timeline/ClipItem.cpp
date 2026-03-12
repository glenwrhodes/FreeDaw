#include "ClipItem.h"
#include "GridSnapper.h"
#include "engine/EditManager.h"
#include "utils/ThemeManager.h"
#include "utils/WaveformCache.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <algorithm>

namespace freedaw {

ClipItem::ClipItem(te::Clip* clip, int trackIndex, double pixelsPerBeat,
                   double trackHeight, QGraphicsItem* parent)
    : QGraphicsRectItem(parent), clip_(clip), trackIndex_(trackIndex)
{
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges);
    setCursor(QCursor(Qt::OpenHandCursor));
    updateGeometry(pixelsPerBeat, trackHeight, 0);
    setAcceptedMouseButtons(Qt::LeftButton);
    setToolTip(QString::fromStdString(clip->getName().toStdString()));
}

void ClipItem::setDragContext(GridSnapper* snapper, EditManager* editMgr,
                               double* pixelsPerBeatPtr, double* trackHeightPtr,
                               int trackCount)
{
    snapper_ = snapper;
    editMgr_ = editMgr;
    pixelsPerBeatPtr_ = pixelsPerBeatPtr;
    trackHeightPtr_ = trackHeightPtr;
    trackCount_ = trackCount;
}

void ClipItem::updateGeometry(double pixelsPerBeat, double trackHeight, double scrollY)
{
    if (!clip_) return;

    auto& edit = clip_->edit;
    auto& ts = edit.tempoSequence;

    auto startTime = clip_->getPosition().getStart();
    auto endTime   = clip_->getPosition().getEnd();
    double startBeat = ts.toBeats(startTime).inBeats();
    double endBeat   = ts.toBeats(endTime).inBeats();

    double x = startBeat * pixelsPerBeat;
    double w = (endBeat - startBeat) * pixelsPerBeat;
    double y = trackIndex_ * trackHeight - scrollY;

    setRect(0, 0, w, trackHeight - 2);
    setPos(x, y);
}

void ClipItem::loadWaveform(int numPoints)
{
    if (!clip_) return;

    if (auto* audioClip = dynamic_cast<te::WaveAudioClip*>(clip_)) {
        auto file = audioClip->getOriginalFile();
        if (file.existsAsFile()) {
            auto data = WaveformCache::instance().getWaveform(file, numPoints);
            waveMin_ = data.minValues;
            waveMax_ = data.maxValues;
        }
    }
}

void ClipItem::paint(QPainter* painter,
                     const QStyleOptionGraphicsItem*,
                     QWidget*)
{
    auto& theme = ThemeManager::instance().current();
    QRectF r = rect();

    QColor bg = isSelected() ? theme.clipBodySelected : theme.clipBody;
    painter->fillRect(r, bg);

    if (!waveMin_.empty() && !waveMax_.empty()) {
        int n = static_cast<int>(waveMin_.size());
        double xScale = r.width() / n;
        double midY   = r.height() / 2.0;
        double amp    = r.height() / 2.0 * 0.9;

        QPainterPath path;
        path.moveTo(r.left(), r.top() + midY);
        for (int i = 0; i < n; ++i)
            path.lineTo(r.left() + i * xScale,
                        r.top() + midY - waveMax_[size_t(i)] * amp);
        for (int i = n - 1; i >= 0; --i)
            path.lineTo(r.left() + i * xScale,
                        r.top() + midY - waveMin_[size_t(i)] * amp);
        path.closeSubpath();

        painter->setPen(Qt::NoPen);
        painter->setBrush(theme.waveform);
        painter->drawPath(path);
    }

    painter->setPen(QPen(theme.border, 0.5));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(r);

    if (clip_) {
        painter->setPen(theme.text);
        QFont f = painter->font();
        f.setPixelSize(10);
        painter->setFont(f);
        QString name = QString::fromStdString(clip_->getName().toStdString());
        painter->drawText(r.adjusted(4, 2, -2, 0),
                          Qt::AlignLeft | Qt::AlignTop, name);
    }
}

void ClipItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        dragStartScene_ = event->scenePos();
        dragStartBeat_ = 0;
        dragStartTrack_ = trackIndex_;

        if (clip_ && pixelsPerBeatPtr_) {
            auto& ts = clip_->edit.tempoSequence;
            auto startTime = clip_->getPosition().getStart();
            dragStartBeat_ = ts.toBeats(startTime).inBeats();
        }

        setCursor(QCursor(Qt::ClosedHandCursor));
        setSelected(true);
        event->accept();
    }
    QGraphicsRectItem::mousePressEvent(event);
}

void ClipItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!dragging_ || !pixelsPerBeatPtr_ || !trackHeightPtr_)
        return;

    double ppb = *pixelsPerBeatPtr_;
    double th  = *trackHeightPtr_;

    QPointF delta = event->scenePos() - dragStartScene_;

    double beatDelta = delta.x() / ppb;
    double newBeat = dragStartBeat_ + beatDelta;
    if (newBeat < 0) newBeat = 0;

    if (snapper_)
        newBeat = snapper_->snapBeat(newBeat);

    int newTrack = dragStartTrack_ + int(std::round(delta.y() / th));
    newTrack = std::clamp(newTrack, 0, std::max(0, trackCount_ - 1));

    double x = newBeat * ppb;
    double y = newTrack * th;
    setPos(x, y);

    event->accept();
}

void ClipItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && dragging_) {
        dragging_ = false;
        setCursor(QCursor(Qt::OpenHandCursor));

        if (clip_ && editMgr_ && pixelsPerBeatPtr_ && trackHeightPtr_) {
            double ppb = *pixelsPerBeatPtr_;
            double th  = *trackHeightPtr_;

            double newBeat = pos().x() / ppb;
            if (newBeat < 0) newBeat = 0;

            int newTrack = int(std::round(pos().y() / th));
            newTrack = std::clamp(newTrack, 0, std::max(0, trackCount_ - 1));

            auto& ts = clip_->edit.tempoSequence;
            auto clipDuration = clip_->getPosition().getLength();

            auto newStartTime = ts.toTime(tracktion::BeatPosition::fromBeats(newBeat));
            auto newEndTime   = newStartTime + clipDuration;
            clip_->setStart(newStartTime, false, true);

            if (newTrack != trackIndex_) {
                auto tracks = editMgr_->getAudioTracks();
                if (newTrack >= 0 && newTrack < tracks.size()) {
                    auto* dstTrack = tracks[newTrack];
                    if (dstTrack) {
                        dstTrack->addClip(clip_);
                        trackIndex_ = newTrack;
                    }
                }
            }

            editMgr_->edit()->restartPlayback();
        }

        event->accept();
    }
    QGraphicsRectItem::mouseReleaseEvent(event);
}

} // namespace freedaw
