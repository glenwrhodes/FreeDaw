#pragma once

namespace freedaw {

enum class SnapMode {
    Off,
    Beat,
    Bar,
    HalfBeat,
    QuarterBeat,
    EighthBeat
};

class GridSnapper {
public:
    void setMode(SnapMode mode) { mode_ = mode; }
    SnapMode mode() const { return mode_; }

    void setBpm(double bpm)      { bpm_ = bpm; }
    void setTimeSig(int num, int den) { timeSigNum_ = num; timeSigDen_ = den; }

    double snapBeat(double beat) const;
    double gridIntervalBeats() const;

private:
    SnapMode mode_ = SnapMode::Beat;
    double   bpm_  = 120.0;
    int      timeSigNum_ = 4;
    int      timeSigDen_ = 4;
};

} // namespace freedaw
