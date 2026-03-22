#pragma once

#include "EnvelopeUtils.h"
#include <QGraphicsItem>
#include <QVector>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion::engine;

namespace OpenDaw {

class GridSnapper;
class AutomationPointItem;

class AutomationLaneItem : public QGraphicsItem {
public:
    AutomationLaneItem(te::AutomatableParameter* param, te::Edit* edit,
                       double pixelsPerBeat, double laneHeight,
                       GridSnapper* snapper, QGraphicsItem* parent = nullptr);
    ~AutomationLaneItem() override;

    te::AutomatableParameter* param() const { return param_; }
    te::Edit* edit() const { return edit_; }
    GridSnapper* snapper() const { return snapper_; }
    double pixelsPerBeat() const { return pixelsPerBeat_; }
    double laneHeight() const { return laneHeight_; }

    void setParam(te::AutomatableParameter* param);
    void setPixelsPerBeat(double ppb);
    void setLaneHeight(double h);
    void setSceneWidth(double w);
    void setPlayheadBeat(double beat);
    void rebuildFromCurve();
    void updateCurvePathOnly();

    // Point selection and group drag (called by AutomationPointItem)
    void clearPointSelection();
    int selectedPointCount() const;
    void togglePointSelection(int index);
    void selectPoint(int index, bool clearOthers);
    void beginPointDrag(int anchorIndex, QPointF startScene, bool shiftHeld);
    void updatePointDrag(QPointF currentScene, Qt::KeyboardModifiers mods);
    void endPointDrag();
    bool isPointDragging() const { return pointDragging_; }

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    enum { Type = QGraphicsItem::UserType + 11 };
    int type() const override { return Type; }

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    void clearPointItems();
    void rebuildPointItems();
    QVector<EnvelopePoint> getPointsFromCurve() const;
    QPainterPath buildSampledPath() const;

    te::AutomatableParameter* param_ = nullptr;
    te::Edit* edit_ = nullptr;
    GridSnapper* snapper_ = nullptr;

    double pixelsPerBeat_ = 40.0;
    double laneHeight_ = 80.0;
    double sceneWidth_ = 2000.0;

    QVector<AutomationPointItem*> pointItems_;
    QPainterPath curvePath_;
    QVector<EnvelopePoint> cachedPoints_;
    double playheadBeat_ = -1.0;

    // Freehand draw state
    bool freehandDrawing_ = false;
    QVector<QPointF> freehandPath_;

    // Curve segment drag state
    bool curveDragging_ = false;
    int curveDragSegmentIndex_ = -1;
    float curveDragStartValue_ = 0.0f;
    QPointF curveDragStartPos_;

    // Point drag state (single or group)
    bool pointDragging_ = false;
    int pointDragAnchorIdx_ = -1;
    QPointF pointDragStartScene_;
    double pointDragStartBeat_ = 0.0;
    float pointDragStartValue_ = 0.0f;
    bool pointDragShiftHeld_ = false;
    QVector<QPair<double, float>> pointDragStartPositions_;
    QVector<int> pointDragSelectedIndices_;
    double pointDragMinBeatDelta_ = -1e9;
    double pointDragMaxBeatDelta_ = 1e9;

    // Rubber band selection state
    bool rubberBanding_ = false;
    QPointF rubberBandStart_;
    QRectF rubberBandRect_;
};

} // namespace OpenDaw
