#pragma once

#include <QWidget>
#include <QColor>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion::engine;

namespace freedaw {

class EditManager;

class TrackLane : public QWidget {
    Q_OBJECT

public:
    explicit TrackLane(te::AudioTrack* track, EditManager* editMgr,
                       QWidget* parent = nullptr);

    te::AudioTrack* track() const { return track_; }
    QString trackName() const;
    QColor  trackColor() const { return color_; }
    void    setTrackColor(const QColor& c) { color_ = c; update(); }

    bool isMuted() const;
    bool isSoloed() const;
    bool isArmed() const;

    void setMuted(bool m);
    void setSoloed(bool s);
    void setArmed(bool a);

signals:
    void muteChanged(bool);
    void soloChanged(bool);
    void armChanged(bool);
    void nameChanged(const QString&);

private:
    te::AudioTrack* track_ = nullptr;
    EditManager* editMgr_ = nullptr;
    QColor color_{60, 120, 110};
};

} // namespace freedaw
