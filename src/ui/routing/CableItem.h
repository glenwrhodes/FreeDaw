#pragma once

#include "RoutingNode.h"
#include <QGraphicsPathItem>
#include <QColor>

namespace freedaw {

class CableItem : public QGraphicsPathItem {
public:
    CableItem(JackItem* sourceJack, JackItem* destJack,
              const QColor& color, QGraphicsItem* parent = nullptr);

    JackItem* sourceJack() const { return sourceJack_; }
    JackItem* destJack() const { return destJack_; }

    void updatePath();
    void setColor(const QColor& c);
    void setDangling(bool d);
    void setDanglingEnd(const QPointF& scenePos);

    bool stepPhysics(qreal dt);
    void resetPhysics();

    QPainterPath shape() const override;

    static QPainterPath buildCablePath(QPointF p0, QPointF p3);
    static QPainterPath buildCablePath(QPointF p0, QPointF c1, QPointF c2, QPointF p3);
    static void computeTargetControlPoints(QPointF p0, QPointF p3,
                                           QPointF& c1Out, QPointF& c2Out);

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    JackItem* sourceJack_;
    JackItem* destJack_;
    QColor color_;
    bool hovered_ = false;
    bool dangling_ = false;
    QPointF danglingEnd_;

    QPointF c1Current_;
    QPointF c2Current_;
    QPointF c1Velocity_;
    QPointF c2Velocity_;
    bool physicsInitialized_ = false;
};

} // namespace freedaw
