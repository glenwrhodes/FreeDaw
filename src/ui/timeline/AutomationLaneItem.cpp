#include "AutomationLaneItem.h"
#include "AutomationPointItem.h"
#include "GridSnapper.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>
#include <QToolTip>
#include <QGraphicsView>
#include <cmath>

namespace OpenDaw {

AutomationLaneItem::AutomationLaneItem(te::AutomatableParameter* param, te::Edit* edit,
                                       double pixelsPerBeat, double laneHeight,
                                       GridSnapper* snapper, QGraphicsItem* parent)
    : QGraphicsItem(parent), param_(param), edit_(edit),
      snapper_(snapper), pixelsPerBeat_(pixelsPerBeat), laneHeight_(laneHeight)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemClipsToShape, false);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    rebuildFromCurve();
}

AutomationLaneItem::~AutomationLaneItem()
{
    clearPointItems();
}

void AutomationLaneItem::setParam(te::AutomatableParameter* param)
{
    param_ = param;
    rebuildFromCurve();
}

void AutomationLaneItem::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat_ = ppb;
    rebuildFromCurve();
}

void AutomationLaneItem::setLaneHeight(double h)
{
    prepareGeometryChange();
    laneHeight_ = h;
    rebuildFromCurve();
}

void AutomationLaneItem::setSceneWidth(double w)
{
    prepareGeometryChange();
    sceneWidth_ = w;
    rebuildFromCurve();
}

void AutomationLaneItem::setPlayheadBeat(double beat)
{
    if (std::abs(playheadBeat_ - beat) > 0.001) {
        playheadBeat_ = beat;
        update();
    }
}

QRectF AutomationLaneItem::boundingRect() const
{
    return QRectF(0, 0, sceneWidth_, laneHeight_);
}

QVector<EnvelopePoint> AutomationLaneItem::getPointsFromCurve() const
{
    QVector<EnvelopePoint> pts;
    if (!param_ || !edit_) return pts;

    auto& curve = param_->getCurve();
    auto& ts = edit_->tempoSequence;
    int n = curve.getNumPoints();
    pts.reserve(n);
    for (int i = 0; i < n; ++i) {
        auto pt = curve.getPoint(i);
        double beat = te::toBeats(pt.time, ts).inBeats();
        pts.append({beat, pt.value, pt.curve});
    }
    return pts;
}

void AutomationLaneItem::clearPointItems()
{
    for (auto* item : pointItems_) {
        item->setParentItem(nullptr);
        if (item->scene())
            item->scene()->removeItem(item);
        delete item;
    }
    pointItems_.clear();
}

void AutomationLaneItem::rebuildPointItems()
{
    clearPointItems();
    if (!param_ || !edit_) return;

    auto& curve = param_->getCurve();
    float minVal = param_->getValueRange().getStart();
    float maxVal = param_->getValueRange().getEnd();

    for (int i = 0; i < curve.getNumPoints(); ++i) {
        auto* ptItem = new AutomationPointItem(i, param_, this, this);
        ptItem->updatePosition(pixelsPerBeat_, minVal, maxVal, laneHeight_);
        pointItems_.append(ptItem);
    }
}

QPainterPath AutomationLaneItem::buildSampledPath() const
{
    QPainterPath path;
    if (!param_ || !edit_ || cachedPoints_.size() < 2) {
        if (cachedPoints_.size() == 1) {
            float minVal = param_->getValueRange().getStart();
            float maxVal = param_->getValueRange().getEnd();
            double x = EnvelopeUtils::beatToX(cachedPoints_[0].beat, pixelsPerBeat_);
            double y = EnvelopeUtils::valueToY(cachedPoints_[0].value, minVal, maxVal, laneHeight_);
            path.moveTo(x, y);
        }
        return path;
    }

    float minVal = param_->getValueRange().getStart();
    float maxVal = param_->getValueRange().getEnd();
    auto& curve = param_->getCurve();
    auto& ts = edit_->tempoSequence;

    double firstBeat = cachedPoints_.first().beat;
    double lastBeat = cachedPoints_.last().beat;

    double startX = EnvelopeUtils::beatToX(firstBeat, pixelsPerBeat_);
    auto firstTimePos = ts.toTime(tracktion::BeatPosition::fromBeats(firstBeat));
    float firstVal = curve.getValueAt(te::EditPosition(firstTimePos), param_->getCurrentBaseValue());
    path.moveTo(startX, EnvelopeUtils::valueToY(firstVal, minVal, maxVal, laneHeight_));

    constexpr double kPixelsPerSample = 3.0;
    double totalPixels = (lastBeat - firstBeat) * pixelsPerBeat_;
    int numSamples = std::max(2, static_cast<int>(totalPixels / kPixelsPerSample));

    for (int i = 1; i <= numSamples; ++i) {
        double t = double(i) / double(numSamples);
        double beat = firstBeat + t * (lastBeat - firstBeat);
        auto timePos = ts.toTime(tracktion::BeatPosition::fromBeats(beat));
        float val = curve.getValueAt(te::EditPosition(timePos), param_->getCurrentBaseValue());
        double px = EnvelopeUtils::beatToX(beat, pixelsPerBeat_);
        double py = EnvelopeUtils::valueToY(val, minVal, maxVal, laneHeight_);
        path.lineTo(px, py);
    }

    return path;
}

void AutomationLaneItem::rebuildFromCurve()
{
    cachedPoints_ = getPointsFromCurve();
    curvePath_ = buildSampledPath();
    rebuildPointItems();
    update();
}

void AutomationLaneItem::updateCurvePathOnly()
{
    cachedPoints_ = getPointsFromCurve();
    curvePath_ = buildSampledPath();
    update();
}

void AutomationLaneItem::clearPointSelection()
{
    for (auto* pt : pointItems_)
        pt->setSelected(false);
}

int AutomationLaneItem::selectedPointCount() const
{
    int n = 0;
    for (auto* pt : pointItems_)
        if (pt->isSelected()) ++n;
    return n;
}

void AutomationLaneItem::togglePointSelection(int index)
{
    if (index >= 0 && index < pointItems_.size())
        pointItems_[index]->setSelected(!pointItems_[index]->isSelected());
}

void AutomationLaneItem::selectPoint(int index, bool clearOthers)
{
    if (clearOthers) clearPointSelection();
    if (index >= 0 && index < pointItems_.size())
        pointItems_[index]->setSelected(true);
}

void AutomationLaneItem::beginPointDrag(int anchorIndex, QPointF startScene, bool shiftHeld)
{
    if (!param_ || !edit_) return;

    pointDragging_ = true;
    pointDragAnchorIdx_ = anchorIndex;
    pointDragStartScene_ = startScene;
    pointDragShiftHeld_ = shiftHeld;

    auto& curve = param_->getCurve();
    auto& ts = edit_->tempoSequence;

    if (anchorIndex >= 0 && anchorIndex < curve.getNumPoints()) {
        auto pt = curve.getPoint(anchorIndex);
        pointDragStartBeat_ = te::toBeats(pt.time, ts).inBeats();
        pointDragStartValue_ = pt.value;
    }

    pointDragStartPositions_.clear();
    pointDragSelectedIndices_.clear();
    for (int i = 0; i < pointItems_.size(); ++i) {
        if (pointItems_[i]->isSelected() && i < curve.getNumPoints()) {
            auto p = curve.getPoint(i);
            double beat = te::toBeats(p.time, ts).inBeats();
            pointDragStartPositions_.append({beat, p.value});
            pointDragSelectedIndices_.append(i);
        }
    }

    constexpr double kEpsilon = 0.001;
    pointDragMinBeatDelta_ = -1e9;
    pointDragMaxBeatDelta_ = 1e9;

    int numCurvePoints = curve.getNumPoints();
    auto isSelectedIdx = [&](int idx) -> bool {
        for (int si : pointDragSelectedIndices_)
            if (si == idx) return true;
        return false;
    };

    for (int si = 0; si < pointDragSelectedIndices_.size(); ++si) {
        int idx = pointDragSelectedIndices_[si];
        double origBeat = pointDragStartPositions_[si].first;

        for (int j = idx - 1; j >= 0; --j) {
            if (!isSelectedIdx(j)) {
                double neighborBeat = te::toBeats(curve.getPoint(j).time, ts).inBeats();
                pointDragMinBeatDelta_ = std::max(pointDragMinBeatDelta_,
                                                  neighborBeat - origBeat + kEpsilon);
                break;
            }
        }

        for (int j = idx + 1; j < numCurvePoints; ++j) {
            if (!isSelectedIdx(j)) {
                double neighborBeat = te::toBeats(curve.getPoint(j).time, ts).inBeats();
                pointDragMaxBeatDelta_ = std::min(pointDragMaxBeatDelta_,
                                                  neighborBeat - origBeat - kEpsilon);
                break;
            }
        }
    }
}

void AutomationLaneItem::updatePointDrag(QPointF currentScene, Qt::KeyboardModifiers modifiers)
{
    if (!pointDragging_ || !param_ || !edit_) return;

    float minVal = param_->getValueRange().getStart();
    float maxVal = param_->getValueRange().getEnd();

    QPointF delta = currentScene - pointDragStartScene_;
    bool shiftNow = (modifiers & Qt::ShiftModifier);

    double beatDelta = 0.0;
    float valueDelta = 0.0f;

    if (!shiftNow || std::abs(delta.x()) > std::abs(delta.y()))
        beatDelta = EnvelopeUtils::xToBeat(delta.x(), pixelsPerBeat_);

    if (!shiftNow || std::abs(delta.y()) >= std::abs(delta.x())) {
        double yAtStart = EnvelopeUtils::valueToY(pointDragStartValue_, minVal, maxVal, laneHeight_);
        float newVal = EnvelopeUtils::yToValue(yAtStart + delta.y(), minVal, maxVal, laneHeight_);
        valueDelta = newVal - pointDragStartValue_;
    }

    if (shiftNow) {
        if (std::abs(delta.x()) > std::abs(delta.y()))
            valueDelta = 0.0f;
        else
            beatDelta = 0.0;
    }

    beatDelta = std::clamp(beatDelta, pointDragMinBeatDelta_, pointDragMaxBeatDelta_);

    double anchorNewBeat = pointDragStartBeat_ + beatDelta;
    if (anchorNewBeat < 0) anchorNewBeat = 0;
    if (snapper_) anchorNewBeat = snapper_->snapBeat(anchorNewBeat);
    beatDelta = anchorNewBeat - pointDragStartBeat_;
    beatDelta = std::clamp(beatDelta, pointDragMinBeatDelta_, pointDragMaxBeatDelta_);

    auto& curve = param_->getCurve();
    auto* um = &edit_->getUndoManager();
    auto valRange = juce::Range<float>(minVal, maxVal);

    for (int i = 0; i < pointDragSelectedIndices_.size(); ++i) {
        int idx = pointDragSelectedIndices_[i];
        double origBeat = pointDragStartPositions_[i].first;
        float origValue = pointDragStartPositions_[i].second;

        double newBeat = std::max(0.0, origBeat + beatDelta);
        float newValue = std::clamp(origValue + valueDelta, minVal, maxVal);

        if (param_->isDiscrete())
            newValue = param_->snapToState(newValue);

        auto timePos = te::EditPosition(tracktion::BeatPosition::fromBeats(newBeat));
        int newIdx = curve.movePoint(idx, timePos, newValue, valRange, false, um);
        pointDragSelectedIndices_[i] = newIdx;

        if (newIdx >= 0 && newIdx < pointItems_.size()) {
            double px = EnvelopeUtils::beatToX(newBeat, pixelsPerBeat_);
            double py = EnvelopeUtils::valueToY(newValue, minVal, maxVal, laneHeight_);
            pointItems_[newIdx]->setPos(px, py);
            pointItems_[newIdx]->setPointIndex(newIdx);
        }
    }

    updateCurvePathOnly();

    float anchorNewValue = std::clamp(pointDragStartValue_ + valueDelta, minVal, maxVal);
    if (param_->isDiscrete())
        anchorNewValue = param_->snapToState(anchorNewValue);

    if (scene() && !scene()->views().isEmpty()) {
        auto* view = scene()->views().first();
        auto juceLabel = param_->getLabelForValue(anchorNewValue);
        QString label = juceLabel.isEmpty()
            ? QString::number(double(anchorNewValue), 'f', 2)
            : QString::fromStdString(juceLabel.toStdString());
        QPoint screenPos = view->mapToGlobal(view->mapFromScene(currentScene));
        QToolTip::showText(screenPos, label);
    }
}

void AutomationLaneItem::endPointDrag()
{
    if (!pointDragging_) return;
    pointDragging_ = false;
    QToolTip::hideText();

    QVector<int> selectedIndices = pointDragSelectedIndices_;
    pointDragSelectedIndices_.clear();
    pointDragStartPositions_.clear();

    rebuildFromCurve();

    for (int idx : selectedIndices) {
        if (idx >= 0 && idx < pointItems_.size())
            pointItems_[idx]->setSelected(true);
    }
}

void AutomationLaneItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    auto& theme = ThemeManager::instance().current();
    QRectF r = boundingRect();

    // Background
    QColor bg = theme.surface;
    bg.setAlpha(200);
    painter->fillRect(r, bg);

    // Reference lines at 25%, 50%, 75%
    QPen refPen(theme.gridLine, 0.5, Qt::DotLine);
    painter->setPen(refPen);
    for (double frac : {0.25, 0.5, 0.75}) {
        double y = r.height() * frac;
        painter->drawLine(QPointF(0, y), QPointF(r.width(), y));
    }

    // Value labels on left edge
    if (param_) {
        QFont labelFont;
        labelFont.setPixelSize(9);
        painter->setFont(labelFont);
        painter->setPen(theme.textDim);

        auto paramName = param_->getParameterName();
        bool isVolume = (paramName == "Volume" || param_->paramID == juce::String("volume"));
        bool isPan = (paramName == "Pan" || param_->paramID == juce::String("pan"));

        QString topLabel, midLabel, bottomLabel;
        if (isVolume) {
            topLabel = "+6 dB";
            midLabel = "-12 dB";
            bottomLabel = QString::fromUtf8("-\xe2\x88\x9e");
        } else if (isPan) {
            topLabel = "R";
            midLabel = "C";
            bottomLabel = "L";
        } else {
            topLabel = "100%";
            midLabel = "50%";
            bottomLabel = "0%";
        }

        painter->drawText(QPointF(3, 10), topLabel);
        painter->drawText(QPointF(3, r.height() * 0.5 + 3), midLabel);
        painter->drawText(QPointF(3, r.height() - 3), bottomLabel);
    }

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Draw existing curve if we have points
    if (param_ && !cachedPoints_.isEmpty()) {
        float minVal = param_->getValueRange().getStart();
        float maxVal = param_->getValueRange().getEnd();

        if (!curvePath_.isEmpty()) {
            QPainterPath fillPath = curvePath_;
            double lastX = EnvelopeUtils::beatToX(cachedPoints_.last().beat, pixelsPerBeat_);
            double firstX = EnvelopeUtils::beatToX(cachedPoints_.first().beat, pixelsPerBeat_);
            fillPath.lineTo(lastX, laneHeight_);
            fillPath.lineTo(firstX, laneHeight_);
            fillPath.closeSubpath();

            QColor fillColor = theme.accent;
            fillColor.setAlpha(38);
            painter->fillPath(fillPath, fillColor);

            // Drop shadow
            constexpr double kShadowOffset = 1.5;
            constexpr int kShadowAlpha = 50;
            QPainterPath shadowPath = curvePath_;
            shadowPath.translate(kShadowOffset, kShadowOffset);
            painter->setPen(QPen(QColor(0, 0, 0, kShadowAlpha), 2.8,
                                 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(Qt::NoBrush);
            painter->drawPath(shadowPath);

            // Main curve line
            QPen curvePen(theme.accentLight, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(curvePen);
            painter->drawPath(curvePath_);
        }

        // Extend line to edges if points don't cover full range
        QPen extPen(theme.accent, 1.2, Qt::DashLine, Qt::RoundCap);
        painter->setPen(extPen);

        double firstY = EnvelopeUtils::valueToY(cachedPoints_.first().value, minVal, maxVal, laneHeight_);
        double firstX = EnvelopeUtils::beatToX(cachedPoints_.first().beat, pixelsPerBeat_);
        if (firstX > 0)
            painter->drawLine(QPointF(0, firstY), QPointF(firstX, firstY));

        double lastY = EnvelopeUtils::valueToY(cachedPoints_.last().value, minVal, maxVal, laneHeight_);
        double lastX = EnvelopeUtils::beatToX(cachedPoints_.last().beat, pixelsPerBeat_);
        if (lastX < r.width())
            painter->drawLine(QPointF(lastX, lastY), QPointF(r.width(), lastY));
    }

    // Freehand preview (always drawn, even on empty lane)
    if (freehandDrawing_ && freehandPath_.size() > 1) {
        QPen drawPen(theme.accentLight, 2.0);
        painter->setPen(drawPen);
        for (int i = 1; i < freehandPath_.size(); ++i)
            painter->drawLine(freehandPath_[i - 1], freehandPath_[i]);
    }

    // Playback position indicator dot
    if (param_ && playheadBeat_ >= 0.0) {
        float minVal = param_->getValueRange().getStart();
        float maxVal = param_->getValueRange().getEnd();
        auto& ts = edit_->tempoSequence;
        auto beatPos = tracktion::BeatPosition::fromBeats(playheadBeat_);
        auto timePos = ts.toTime(beatPos);
        auto posForLookup = te::EditPosition(timePos);
        float val = param_->getCurve().getValueAt(posForLookup, param_->getCurrentBaseValue());

        double dotX = EnvelopeUtils::beatToX(playheadBeat_, pixelsPerBeat_);
        double dotY = EnvelopeUtils::valueToY(val, minVal, maxVal, laneHeight_);

        if (dotX >= 0 && dotX <= r.width()) {
            constexpr double kDotRadius = 4.5;
            painter->setPen(QPen(theme.text, 1.5));
            painter->setBrush(theme.accentLight);
            painter->drawEllipse(QPointF(dotX, dotY), kDotRadius, kDotRadius);
        }
    }

    // Rubber band selection rectangle
    if (rubberBanding_ && !rubberBandRect_.isNull()) {
        QColor rbFill = theme.accent;
        rbFill.setAlpha(40);
        painter->fillRect(rubberBandRect_, rbFill);
        painter->setPen(QPen(theme.accent, 1.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rubberBandRect_);
    }
}

void AutomationLaneItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (!param_ || !edit_ || event->button() != Qt::LeftButton) return;

    // Cancel any freehand draw that the first press started
    freehandDrawing_ = false;
    freehandPath_.clear();

    QPointF local = event->pos();
    float minVal = param_->getValueRange().getStart();
    float maxVal = param_->getValueRange().getEnd();

    double beat = EnvelopeUtils::xToBeat(local.x(), pixelsPerBeat_);
    if (beat < 0) beat = 0;
    if (snapper_) beat = snapper_->snapBeat(beat);

    auto& curve = param_->getCurve();
    auto* um = &edit_->getUndoManager();
    auto& ts = edit_->tempoSequence;
    auto beatPos = tracktion::BeatPosition::fromBeats(beat);
    auto timePos = ts.toTime(beatPos);

    // Use both representations: TimePosition for accurate curve lookup,
    // BeatPosition for adding the point
    auto posForLookup = te::EditPosition(timePos);
    auto posForInsert = te::EditPosition(beatPos);

    // If on the curve line, use the interpolated value to preserve shape
    float value;
    if (EnvelopeUtils::hitTestCurve(curvePath_, local, 6.0)) {
        value = curve.getValueAt(posForLookup, param_->getCurrentBaseValue());
    } else {
        value = EnvelopeUtils::yToValue(local.y(), minVal, maxVal, laneHeight_);
    }
    value = std::clamp(value, minVal, maxVal);

    if (param_->isDiscrete())
        value = param_->snapToState(value);

    float curveVal = param_->isDiscrete() ? 1.0f : 0.0f;
    curve.addPoint(posForInsert, value, curveVal, um);
    rebuildFromCurve();
    event->accept();
}

void AutomationLaneItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!param_ || !edit_ || event->button() != Qt::LeftButton) {
        QGraphicsItem::mousePressEvent(event);
        return;
    }

    QPointF local = event->pos();
    bool ctrlHeld = (event->modifiers() & Qt::ControlModifier);

    // Ctrl + click on lane background = rubber band selection
    if (ctrlHeld) {
        rubberBanding_ = true;
        rubberBandStart_ = local;
        rubberBandRect_ = QRectF();
        event->accept();
        return;
    }

    // Click on curve line (no Ctrl) = segment bending
    if (!param_->isDiscrete() && EnvelopeUtils::hitTestCurve(curvePath_, local, 5.0)) {
        int segIdx = -1;
        for (int i = 0; i < cachedPoints_.size() - 1; ++i) {
            double x0 = EnvelopeUtils::beatToX(cachedPoints_[i].beat, pixelsPerBeat_);
            double x1 = EnvelopeUtils::beatToX(cachedPoints_[i + 1].beat, pixelsPerBeat_);
            if (local.x() >= x0 && local.x() <= x1) {
                segIdx = i;
                break;
            }
        }
        if (segIdx >= 0) {
            curveDragging_ = true;
            curveDragSegmentIndex_ = segIdx;
            curveDragStartValue_ = cachedPoints_[segIdx].curve;
            curveDragStartPos_ = event->scenePos();
            setCursor(Qt::SizeVerCursor);
            event->accept();
            return;
        }
    }

    // Click on empty space = clear selection, start freehand draw
    clearPointSelection();
    freehandDrawing_ = true;
    freehandPath_.clear();
    freehandPath_.append(local);
    event->accept();
}

void AutomationLaneItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (curveDragging_ && param_ && edit_) {
        double deltaY = event->scenePos().y() - curveDragStartPos_.y();

        // Flip drag direction for ascending segments so the visual curve
        // always follows the mouse direction regardless of segment slope
        if (curveDragSegmentIndex_ + 1 < cachedPoints_.size() &&
            cachedPoints_[curveDragSegmentIndex_ + 1].value >
            cachedPoints_[curveDragSegmentIndex_].value) {
            deltaY = -deltaY;
        }

        float newCurve = curveDragStartValue_ - float(deltaY / 100.0);
        newCurve = std::clamp(newCurve, -1.0f, 1.0f);

        auto& curve = param_->getCurve();
        auto* um = &edit_->getUndoManager();
        curve.setCurveValue(curveDragSegmentIndex_, newCurve, um);
        rebuildFromCurve();
        event->accept();
        return;
    }

    if (rubberBanding_) {
        QPointF local = event->pos();
        rubberBandRect_ = QRectF(rubberBandStart_, local).normalized();
        for (auto* ptItem : pointItems_)
            ptItem->setSelected(rubberBandRect_.contains(ptItem->pos()));
        update();
        event->accept();
        return;
    }

    if (freehandDrawing_) {
        freehandPath_.append(event->pos());
        update();
        event->accept();
        return;
    }
}

void AutomationLaneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (curveDragging_) {
        curveDragging_ = false;
        curveDragSegmentIndex_ = -1;
        unsetCursor();
        event->accept();
        return;
    }

    if (rubberBanding_) {
        rubberBanding_ = false;
        rubberBandRect_ = QRectF();
        update();
        event->accept();
        return;
    }

    if (freehandDrawing_ && param_ && edit_) {
        freehandDrawing_ = false;

        if (freehandPath_.size() >= 2) {
            auto& curve = param_->getCurve();
            auto* um = &edit_->getUndoManager();
            float minVal = param_->getValueRange().getStart();
            float maxVal = param_->getValueRange().getEnd();
            bool discrete = param_->isDiscrete();

            double startBeat = EnvelopeUtils::xToBeat(freehandPath_.first().x(), pixelsPerBeat_);
            double endBeat = EnvelopeUtils::xToBeat(freehandPath_.last().x(), pixelsPerBeat_);
            if (startBeat > endBeat) std::swap(startBeat, endBeat);

            auto beatStart = tracktion::BeatPosition::fromBeats(startBeat);
            auto beatEnd = tracktion::BeatPosition::fromBeats(endBeat);
            te::EditTimeRange range(beatStart, beatEnd);

            curve.removePoints(range, um);

            double gridInterval = snapper_ ? snapper_->gridIntervalBeats() : 0.25;
            double drawInterval = gridInterval * 0.25;
            if (drawInterval < 0.0625) drawInterval = 0.0625;

            for (auto& pt : freehandPath_) {
                double beat = EnvelopeUtils::xToBeat(pt.x(), pixelsPerBeat_);
                if (beat < 0) beat = 0;

                float value = EnvelopeUtils::yToValue(pt.y(), minVal, maxVal, laneHeight_);
                value = std::clamp(value, minVal, maxVal);
                if (discrete) value = param_->snapToState(value);

                float curveVal = discrete ? 1.0f : 0.0f;

                auto pos = te::EditPosition(tracktion::BeatPosition::fromBeats(beat));
                curve.addPoint(pos, value, curveVal, um);
            }

            auto simplifyDuration = tracktion::BeatDuration::fromBeats(drawInterval);
            te::EditDuration ed(simplifyDuration);
            curve.simplify(range, ed, 0.01f, um);
        }

        freehandPath_.clear();
        rebuildFromCurve();
        event->accept();
        return;
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void AutomationLaneItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!param_) {
        setCursor(Qt::CrossCursor);
        return;
    }

    QPointF local = event->pos();

    if (!param_->isDiscrete() && EnvelopeUtils::hitTestCurve(curvePath_, local, 5.0)) {
        setCursor(Qt::SizeVerCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
}

} // namespace OpenDaw
