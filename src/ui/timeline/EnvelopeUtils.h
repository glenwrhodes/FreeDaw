#pragma once

#include <QPainterPath>
#include <QPointF>
#include <QVector>
#include <cmath>

namespace OpenDaw {

struct EnvelopePoint {
    double beat;
    float value;
    float curve;  // -1..1: 0=linear, >0=convex(ease out), <0=concave(ease in)
};

namespace EnvelopeUtils {

inline double beatToX(double beat, double pixelsPerBeat)
{
    return beat * pixelsPerBeat;
}

inline double xToBeat(double x, double pixelsPerBeat)
{
    return (pixelsPerBeat > 0.0) ? x / pixelsPerBeat : 0.0;
}

inline double valueToY(float value, float minVal, float maxVal, double height)
{
    if (maxVal <= minVal) return height * 0.5;
    double norm = double(value - minVal) / double(maxVal - minVal);
    return height - norm * height;
}

inline float yToValue(double y, float minVal, float maxVal, double height)
{
    if (height <= 0.0) return minVal;
    double norm = 1.0 - (y / height);
    norm = std::clamp(norm, 0.0, 1.0);
    return minVal + float(norm) * (maxVal - minVal);
}

inline QPainterPath buildEnvelopePath(
    const QVector<EnvelopePoint>& points,
    double pixelsPerBeat,
    float minVal, float maxVal,
    double laneHeight,
    double sceneXOffset,
    bool discrete)
{
    QPainterPath path;
    if (points.isEmpty()) return path;

    auto px = [&](const EnvelopePoint& p) -> QPointF {
        return { beatToX(p.beat, pixelsPerBeat) - sceneXOffset,
                 valueToY(p.value, minVal, maxVal, laneHeight) };
    };

    path.moveTo(px(points[0]));

    for (int i = 0; i < points.size() - 1; ++i) {
        QPointF p0 = px(points[i]);
        QPointF p1 = px(points[i + 1]);
        float c = points[i].curve;

        if (discrete || std::abs(c) > 0.95f) {
            path.lineTo(p1.x(), p0.y());
            path.lineTo(p1);
        } else if (std::abs(c) < 0.01f) {
            path.lineTo(p1);
        } else {
            double ctrlX, ctrlY;
            if (c > 0) {
                ctrlX = p0.x() + (p1.x() - p0.x()) * double(c);
                ctrlY = p0.y();
            } else {
                ctrlX = p1.x() + (p0.x() - p1.x()) * double(-c);
                ctrlY = p1.y();
            }
            path.quadTo(ctrlX, ctrlY, p1.x(), p1.y());
        }
    }

    return path;
}

inline int hitTestPoint(const QVector<EnvelopePoint>& points,
                        double pixelsPerBeat,
                        float minVal, float maxVal,
                        double laneHeight,
                        QPointF localPos,
                        double tolerance = 6.0)
{
    for (int i = 0; i < points.size(); ++i) {
        double px = beatToX(points[i].beat, pixelsPerBeat);
        double py = valueToY(points[i].value, minVal, maxVal, laneHeight);
        double dx = localPos.x() - px;
        double dy = localPos.y() - py;
        if (dx * dx + dy * dy <= tolerance * tolerance)
            return i;
    }
    return -1;
}

inline bool hitTestCurve(const QPainterPath& path, QPointF localPos, double tolerance = 5.0)
{
    QPainterPathStroker stroker;
    stroker.setWidth(tolerance * 2.0);
    QPainterPath strokedPath = stroker.createStroke(path);
    return strokedPath.contains(localPos);
}

} // namespace EnvelopeUtils
} // namespace OpenDaw
