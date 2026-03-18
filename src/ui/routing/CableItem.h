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

    QPainterPath shape() const override;

    static QPainterPath buildCablePath(QPointF p0, QPointF p3);

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
};

} // namespace freedaw
