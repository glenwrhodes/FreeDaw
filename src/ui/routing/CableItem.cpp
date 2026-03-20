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

constexpr qreal SPRING_STIFFNESS = 35.0;
constexpr qreal SPRING_DAMPING   = 8.0;
constexpr qreal SETTLE_THRESHOLD = 0.5;
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

void CableItem::computeTargetControlPoints(QPointF p0, QPointF p3,
                                            QPointF& c1Out, QPointF& c2Out)
{
    const qreal dx = p3.x() - p0.x();
    const qreal dy = p3.y() - p0.y();
    const qreal dist = std::sqrt(dx * dx + dy * dy);
    const qreal droop = DROOP_BASE + DROOP_FACTOR * dist;
    const qreal hExtent = std::abs(dx) * 0.25;

    c1Out = QPointF(p0.x() + hExtent, p0.y() + droop);
    c2Out = QPointF(p3.x() - hExtent, p3.y() + droop);
}

QPainterPath CableItem::buildCablePath(QPointF p0, QPointF p3)
{
    QPointF c1, c2;
    computeTargetControlPoints(p0, p3, c1, c2);
    return buildCablePath(p0, c1, c2, p3);
}

QPainterPath CableItem::buildCablePath(QPointF p0, QPointF c1, QPointF c2, QPointF p3)
{
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

    QPointF t1, t2;
    computeTargetControlPoints(p0, p3, t1, t2);

    if (!physicsInitialized_) {
        c1Current_ = t1;
        c2Current_ = t2;
        c1Velocity_ = QPointF(0, 0);
        c2Velocity_ = QPointF(0, 0);
        physicsInitialized_ = true;
    }

    setPath(buildCablePath(p0, c1Current_, c2Current_, p3));
}

bool CableItem::stepPhysics(qreal dt)
{
    QPointF p0 = sourceJack_ ? sourceJack_->sceneCenterPos() : QPointF(0, 0);
    QPointF p3 = dangling_ ? danglingEnd_
                            : (destJack_ ? destJack_->sceneCenterPos() : QPointF(0, 0));

    QPointF t1, t2;
    computeTargetControlPoints(p0, p3, t1, t2);

    if (!physicsInitialized_) {
        c1Current_ = t1;
        c2Current_ = t2;
        c1Velocity_ = QPointF(0, 0);
        c2Velocity_ = QPointF(0, 0);
        physicsInitialized_ = true;
        setPath(buildCablePath(p0, c1Current_, c2Current_, p3));
        return false;
    }

    auto springStep = [&](QPointF& pos, QPointF& vel, const QPointF& target) {
        QPointF force = SPRING_STIFFNESS * (target - pos) - SPRING_DAMPING * vel;
        vel += force * dt;
        pos += vel * dt;
    };

    springStep(c1Current_, c1Velocity_, t1);
    springStep(c2Current_, c2Velocity_, t2);

    setPath(buildCablePath(p0, c1Current_, c2Current_, p3));

    qreal err = QPointF(c1Current_ - t1).manhattanLength()
              + QPointF(c2Current_ - t2).manhattanLength();
    qreal spd = c1Velocity_.manhattanLength() + c2Velocity_.manhattanLength();
    return (err > SETTLE_THRESHOLD || spd > SETTLE_THRESHOLD);
}

void CableItem::resetPhysics()
{
    physicsInitialized_ = false;
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

    QPainterPath shadowPath = path();
    shadowPath.translate(SHADOW_OFFSET, SHADOW_OFFSET);
    QColor shadowColor(0, 0, 0, SHADOW_ALPHA);
    painter->setPen(QPen(shadowColor, thickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(shadowPath);

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
