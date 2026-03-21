#include "TrackLane.h"
#include "engine/EditManager.h"

namespace OpenDaw {

TrackLane::TrackLane(te::AudioTrack* track, EditManager* editMgr, QWidget* parent)
    : QWidget(parent), track_(track), editMgr_(editMgr)
{
    setAccessibleName("Track Lane");
}

QString TrackLane::trackName() const
{
    if (!track_) return {};
    return QString::fromStdString(track_->getName().toStdString());
}

bool TrackLane::isMuted() const
{
    return track_ && track_->isMuted(false);
}

bool TrackLane::isSoloed() const
{
    return track_ && track_->isSolo(false);
}

bool TrackLane::isArmed() const
{
    if (!track_ || !editMgr_) return false;
    return editMgr_->isTrackRecordEnabled(track_);
}

void TrackLane::setMuted(bool m)
{
    if (track_) {
        track_->setMute(m);
        emit muteChanged(m);
    }
}

void TrackLane::setSoloed(bool s)
{
    if (track_) {
        track_->setSolo(s);
        emit soloChanged(s);
    }
}

void TrackLane::setArmed(bool a)
{
    if (!track_ || !editMgr_) return;
    editMgr_->setTrackRecordEnabled(*track_, a);
    emit armChanged(a);
}

} // namespace OpenDaw
