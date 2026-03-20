#include "AutomationPointItem.h"
#include "AutomationLaneItem.h"
#include "EnvelopeUtils.h"
#include "GridSnapper.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QToolTip>
#include <QGraphicsView>
#include <QTimer>
#include <cmath>

namespace freedaw {

AutomationPointItem::AutomationPointItem(int pointIndex, te::AutomatableParameter* param,
                                         AutomationLaneItem* lane, QGraphicsItem* parent)
    : QGraphicsItem(parent), pointIndex_(pointIndex), param_(param), lane_(lane)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setZValue(5);
}

void AutomationPointItem::updatePosition(double pixelsPerBeat, float minVal, float maxVal, double laneHeight)
{
    if (!param_ || !lane_ || !lane_->edit()) return;
    auto& curve = param_->getCurve();
    if (pointIndex_ < 0 || pointIndex_ >= curve.getNumPoints()) return;

    auto pt = curve.getPoint(pointIndex_);
    double beat = te::toBeats(pt.time, lane_->edit()->tempoSequence).inBeats();
    double x = EnvelopeUtils::beatToX(beat, pixelsPerBeat);
    double y = EnvelopeUtils::valueToY(pt.value, minVal, maxVal, laneHeight);
    setPos(x, y);
}

QRectF AutomationPointItem::boundingRect() const
{
    double s = hovered_ ? kHoverSize : kSize;
    return QRectF(-s / 2.0, -s / 2.0, s, s);
}

void AutomationPointItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    auto& theme = ThemeManager::instance().current();
    double s = hovered_ ? kHoverSize : kSize;
    double half = s / 2.0;

    QColor fill = isSelected() ? theme.pianoRollNoteSelected : (hovered_ ? theme.accentLight : theme.accent);
    QColor border = hovered_ ? theme.text : theme.accentLight;

    painter->setRenderHint(QPainter::Antialiasing, true);

    QPolygonF diamond;
    diamond << QPointF(0, -half)
            << QPointF(half, 0)
            << QPointF(0, half)
            << QPointF(-half, 0);

    painter->setPen(QPen(border, 1.0));
    painter->setBrush(fill);
    painter->drawPolygon(diamond);
}

void AutomationPointItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
    hovered_ = true;
    prepareGeometryChange();
    setCursor(Qt::SizeAllCursor);
    update();
}

void AutomationPointItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    hovered_ = false;
    prepareGeometryChange();
    unsetCursor();
    update();
}

void AutomationPointItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && param_ && lane_) {
        dragging_ = true;
        shiftHeld_ = (event->modifiers() & Qt::ShiftModifier);
        dragStartScene_ = event->scenePos();

        auto& curve = param_->getCurve();
        if (pointIndex_ >= 0 && pointIndex_ < curve.getNumPoints()) {
            auto pt = curve.getPoint(pointIndex_);
            dragStartBeat_ = te::toBeats(pt.time, lane_->edit()->tempoSequence).inBeats();
            dragStartValue_ = pt.value;
        }
        event->accept();
    } else {
        QGraphicsItem::mousePressEvent(event);
    }
}

void AutomationPointItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!dragging_ || !param_ || !lane_) return;

    auto* snapper = lane_->snapper();
    double ppb = lane_->pixelsPerBeat();
    double laneHeight = lane_->laneHeight();
    float minVal = param_->getValueRange().getStart();
    float maxVal = param_->getValueRange().getEnd();

    QPointF delta = event->scenePos() - dragStartScene_;

    double newBeat = dragStartBeat_;
    float newValue = dragStartValue_;

    if (!shiftHeld_ || std::abs(delta.x()) > std::abs(delta.y())) {
        newBeat = dragStartBeat_ + EnvelopeUtils::xToBeat(delta.x(), ppb);
        if (newBeat < 0) newBeat = 0;
        if (snapper) newBeat = snapper->snapBeat(newBeat);
    }

    if (!shiftHeld_ || std::abs(delta.y()) >= std::abs(delta.x())) {
        double yInLane = EnvelopeUtils::valueToY(dragStartValue_, minVal, maxVal, laneHeight) + delta.y();
        newValue = EnvelopeUtils::yToValue(yInLane, minVal, maxVal, laneHeight);

        if (param_->isDiscrete())
            newValue = param_->snapToState(newValue);
    }

    if (shiftHeld_) {
        if (std::abs(delta.x()) > std::abs(delta.y()))
            newValue = dragStartValue_;
        else
            newBeat = dragStartBeat_;
    }

    newValue = std::clamp(newValue, minVal, maxVal);

    auto& curve = param_->getCurve();
    auto* edit = lane_->edit();
    auto* um = edit ? &edit->getUndoManager() : nullptr;

    auto timePos = te::EditPosition(tracktion::BeatPosition::fromBeats(newBeat));
    auto valRange = juce::Range<float>(minVal, maxVal);
    pointIndex_ = curve.movePoint(pointIndex_, timePos, newValue,
                                  valRange, false, um);

    // Update only this point's visual position (don't rebuild, as that deletes us)
    double newX = EnvelopeUtils::beatToX(newBeat, ppb);
    double newY = EnvelopeUtils::valueToY(newValue, minVal, maxVal, laneHeight);
    setPos(newX, newY);

    // Redraw the curve path on the lane without recreating point items
    lane_->updateCurvePathOnly();

    if (scene() && !scene()->views().isEmpty()) {
        auto* view = scene()->views().first();
        auto juceLabel = param_->getLabelForValue(newValue);
        QString label = juceLabel.isEmpty()
            ? QString::number(double(newValue), 'f', 2)
            : QString::fromStdString(juceLabel.toStdString());
        QPoint screenPos = view->mapToGlobal(view->mapFromScene(event->scenePos()));
        QToolTip::showText(screenPos, label);
    }

    event->accept();
}

void AutomationPointItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (dragging_) {
        dragging_ = false;
        QToolTip::hideText();
        // rebuildFromCurve() destroys and recreates all point items including
        // this one, so we must not access any members after the call.
        auto* lane = lane_;
        event->accept();
        if (lane)
            lane->rebuildFromCurve();
        return;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void AutomationPointItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    if (!param_ || !lane_) return;

    auto& curve = param_->getCurve();
    auto* edit = lane_->edit();
    auto* um = edit ? &edit->getUndoManager() : nullptr;

    QMenu menu;
    menu.setAccessibleName("Automation Point Menu");

    menu.addAction("Delete Point", [this, &curve, um]() {
        if (pointIndex_ >= 0 && pointIndex_ < curve.getNumPoints()) {
            curve.removePoint(pointIndex_, um);
            lane_->rebuildFromCurve();
        }
    });

    if (!param_->isDiscrete()) {
        auto* curveMenu = menu.addMenu("Curve Shape");
        curveMenu->setAccessibleName("Curve Shape Submenu");

        auto addPreset = [this, &curve, um, curveMenu](const QString& name, float val) {
            curveMenu->addAction(name, [this, &curve, um, val]() {
                if (pointIndex_ >= 0 && pointIndex_ < curve.getNumPoints()) {
                    curve.setCurveValue(pointIndex_, val, um);
                    lane_->rebuildFromCurve();
                }
            });
        };

        addPreset("Linear", 0.0f);
        addPreset("Ease In (slow start)", -0.5f);
        addPreset("Ease Out (slow end)", 0.5f);
        addPreset("Sharp Ease In", -0.9f);
        addPreset("Sharp Ease Out", 0.9f);
        addPreset("Step (hold then jump)", 1.0f);
    }

    menu.exec(event->screenPos());
    event->accept();
}

} // namespace freedaw
