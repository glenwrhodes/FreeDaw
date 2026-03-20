#pragma once

#include <QGraphicsItem>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion::engine;

namespace freedaw {

class GridSnapper;
class AutomationLaneItem;

class AutomationPointItem : public QGraphicsItem {
public:
    AutomationPointItem(int pointIndex, te::AutomatableParameter* param,
                        AutomationLaneItem* lane, QGraphicsItem* parent = nullptr);

    void setPointIndex(int idx) { pointIndex_ = idx; }
    int pointIndex() const { return pointIndex_; }
    void updatePosition(double pixelsPerBeat, float minVal, float maxVal, double laneHeight);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    enum { Type = QGraphicsItem::UserType + 10 };
    int type() const override { return Type; }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    int pointIndex_ = 0;
    te::AutomatableParameter* param_ = nullptr;
    AutomationLaneItem* lane_ = nullptr;
    bool hovered_ = false;
    bool dragging_ = false;
    bool shiftHeld_ = false;
    QPointF dragStartScene_;
    double dragStartBeat_ = 0.0;
    float dragStartValue_ = 0.0f;
    static constexpr double kSize = 8.0;
    static constexpr double kHoverSize = 10.0;
};

} // namespace freedaw
