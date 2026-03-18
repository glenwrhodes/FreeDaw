#include "GridSnapper.h"
#include <algorithm>
#include <cmath>

namespace freedaw {

double GridSnapper::gridIntervalBeats() const
{
    switch (mode_) {
    case SnapMode::Off:          return 0.0;
    case SnapMode::QuarterBeat:  return 0.25;
    case SnapMode::HalfBeat:     return 0.5;
    case SnapMode::Beat:         return 1.0;
    case SnapMode::Bar:          return double(timeSigNum_);
    }
    return 1.0;
}

double GridSnapper::snapBeat(double beat) const
{
    double interval = gridIntervalBeats();
    if (interval <= 0.0 || mode_ == SnapMode::Off)
        return beat;
    return std::max(0.0, std::round(beat / interval) * interval);
}

} // namespace freedaw
