#include "CableItem.h"
#include <QPainter>
#include <QPainterPathStroker>
#include <QGraphicsSceneHoverEvent>
#include <cmath>

namespace freedaw {

namespace {
constexpr qreal CABLE_THICKNESS = 3.5;
constexpr qreal CABLE_HOVER_THICKNESS = 5.5;
constexpr qreal HIT_TOLERANCE = 12.0;
constexpr qreal DROOP_BASE = 25.0;
constexpr qreal DROOP_FACTOR = 0.25;
constexpr qreal SHADOW_OFFSET = 1.5;
constexpr int   SHADOW_ALPHA = 45;
}

CableItem::CableItem(JackItem* sourceJack, JackItem* destJack,
                      const QColor& color, QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
    , sourceJack_(sourceJack), destJack_(destJack), color_(color)
{
    setAcceptHoverEvents(true);
    setZValue(100);
    updatePath();
}

QPainterPath CableItem::buildCablePath(QPointF p0, QPointF p3)
{
    const qreal dx = p3.x() - p0.x();
    const qreal dy = p3.y() - p0.y();
    const qreal dist = std::sqrt(dx * dx + dy * dy);
    const qreal droop = DROOP_BASE + DROOP_FACTOR * dist;

    const qreal hExtent = std::abs(dx) * 0.25;

    QPointF c1(p0.x() + hExtent, p0.y() + droop);
    QPointF c2(p3.x() - hExtent, p3.y() + droop);

    QPainterPath path;
    path.moveTo(p0);
    path.cubicTo(c1, c2, p3);
    return path;
}

void CableItem::updatePath()
{
    QPointF p0 = sourceJack_ ? sourceJack_->sceneCenterPos() : QPointF(0, 0);
    QPointF p3 = dangling_ ? danglingEnd_
                            : (destJack_ ? destJack_->sceneCenterPos() : QPointF(0, 0));

    setPath(buildCablePath(p0, p3));
}

void CableItem::setColor(const QColor& c)
{
    color_ = c;
    update();
}

void CableItem::setDangling(bool d)
{
    dangling_ = d;
}

void CableItem::setDanglingEnd(const QPointF& scenePos)
{
    danglingEnd_ = scenePos;
    updatePath();
}

QPainterPath CableItem::shape() const
{
    QPainterPathStroker stroker;
    stroker.setWidth(HIT_TOLERANCE);
    stroker.setCapStyle(Qt::RoundCap);
    return stroker.createStroke(path());
}

void CableItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);

    const qreal thickness = hovered_ ? CABLE_HOVER_THICKNESS : CABLE_THICKNESS;

    // Drop shadow
    QPainterPath shadowPath = path();
    shadowPath.translate(SHADOW_OFFSET, SHADOW_OFFSET);
    QColor shadowColor(0, 0, 0, SHADOW_ALPHA);
    painter->setPen(QPen(shadowColor, thickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(shadowPath);

    // Main cable
    QColor drawColor = color_;
    if (dangling_)
        drawColor.setAlpha(160);
    if (hovered_)
        drawColor = drawColor.lighter(130);

    QPen cablePen(drawColor, thickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(cablePen);
    painter->drawPath(path());
}

void CableItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
    hovered_ = true;
    update();
}

void CableItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    hovered_ = false;
    update();
}

} // namespace freedaw
