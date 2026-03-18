#include "NoteGrid.h"
#include "utils/ThemeManager.h"
#include <QScrollBar>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QShowEvent>
#include <QTimer>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <cmath>

namespace freedaw {

NoteGridScene::NoteGridScene(QObject* parent) : QGraphicsScene(parent) {}

void NoteGridScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mousePressEvent(event);
}

void NoteGridScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mouseDoubleClickEvent(event);
}

NoteGrid::NoteGrid(QWidget* parent) : QGraphicsView(parent)
{
    setAccessibleName("Note Grid");
    scene_ = new NoteGridScene(this);
    setScene(scene_);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setRenderHint(QPainter::Antialiasing, false);
    setFocusPolicy(Qt::StrongFocus);
    viewport()->setFocusPolicy(Qt::StrongFocus);

    auto& theme = ThemeManager::instance().current();
    setBackgroundBrush(theme.pianoRollBackground);

    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &NoteGrid::verticalScrollChanged);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &NoteGrid::horizontalScrollChanged);

    setEditMode(EditMode::Edit);
    updateSceneSize();
}

void NoteGrid::setClip(te::MidiClip* clip)
{
    clip_ = clip;
    rebuildNotes();
}

void NoteGrid::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat_ = std::clamp(ppb, 10.0, 300.0);
    updateSceneSize();
    rebuildNotes();
    emit zoomChanged();
}

void NoteGrid::setNoteRowHeight(double h)
{
    noteRowHeight_ = std::clamp(h, 6.0, 40.0);
    updateSceneSize();
    rebuildNotes();
}

void NoteGrid::setEditMode(EditMode mode)
{
    editMode_ = mode;
    setDragMode(editMode_ == EditMode::Edit ? QGraphicsView::RubberBandDrag
                                            : QGraphicsView::NoDrag);
    if (editMode_ != EditMode::Draw)
        clearDrawPreview();
}

void NoteGrid::updateSceneSize()
{
    double clipLenBeats = 16.0;
    if (clip_) {
        auto& ts = clip_->edit.tempoSequence;
        double startBeat = ts.toBeats(clip_->getPosition().getStart()).inBeats();
        double endBeat = ts.toBeats(clip_->getPosition().getEnd()).inBeats();
        clipLenBeats = endBeat - startBeat;
    }
    double totalBeats = std::max(clipLenBeats + 16.0, 32.0);
    double w = totalBeats * pixelsPerBeat_;
    double h = TOTAL_NOTES * noteRowHeight_;
    scene_->setSceneRect(0, 0, w, h);
}

void NoteGrid::drawBackground()
{
    auto& theme = ThemeManager::instance().current();
    double sceneW = scene_->sceneRect().width();
    double sceneH = TOTAL_NOTES * noteRowHeight_;

    for (auto* item : bgItems_) scene_->removeItem(item);
    for (auto* item : gridLines_) scene_->removeItem(item);
    qDeleteAll(bgItems_);
    qDeleteAll(gridLines_);
    bgItems_.clear();
    gridLines_.clear();

    if (clipRegionLeft_) { scene_->removeItem(clipRegionLeft_); delete clipRegionLeft_; clipRegionLeft_ = nullptr; }
    if (clipRegionRight_) { scene_->removeItem(clipRegionRight_); delete clipRegionRight_; clipRegionRight_ = nullptr; }

    double clipLenBeats = 4.0;
    if (clip_) {
        auto& ts = clip_->edit.tempoSequence;
        double startBeat = ts.toBeats(clip_->getPosition().getStart()).inBeats();
        double endBeat = ts.toBeats(clip_->getPosition().getEnd()).inBeats();
        clipLenBeats = endBeat - startBeat;
    }
    double clipEndX = clipLenBeats * pixelsPerBeat_;

    for (int note = 0; note < TOTAL_NOTES; ++note) {
        int row = (TOTAL_NOTES - 1) - note;
        double y = row * noteRowHeight_;
        bool black = (note % 12 == 1 || note % 12 == 3 || note % 12 == 6
                     || note % 12 == 8 || note % 12 == 10);
        QColor bg = black ? theme.pianoRollBlackKey : theme.pianoRollWhiteKey;
        auto* rect = scene_->addRect(0, y, sceneW, noteRowHeight_,
                                     QPen(Qt::NoPen), QBrush(bg));
        rect->setZValue(-2);
        bgItems_.push_back(rect);

        if (note % 12 == 0) {
            auto* line = scene_->addLine(0, y + noteRowHeight_, sceneW, y + noteRowHeight_,
                                         QPen(theme.pianoRollGrid.lighter(120), 0.8));
            line->setZValue(-1);
            gridLines_.push_back(line);
        }
    }

    QColor dimOverlay(0, 0, 0, 100);
    if (clipEndX < sceneW) {
        clipRegionRight_ = scene_->addRect(clipEndX, 0, sceneW - clipEndX, sceneH,
                                           QPen(Qt::NoPen), QBrush(dimOverlay));
        clipRegionRight_->setZValue(0);
    }

    auto* rightBorder = scene_->addLine(clipEndX, 0, clipEndX, sceneH,
                                        QPen(QColor(255, 255, 255, 60), 1.5));
    rightBorder->setZValue(0);
    gridLines_.push_back(rightBorder);

    double beatsPerBar = 4.0;
    double totalBeats = sceneW / pixelsPerBeat_;
    for (double beat = 0; beat < totalBeats; beat += 1.0) {
        double x = beat * pixelsPerBeat_;
        bool isMajor = (std::fmod(beat, beatsPerBar) < 0.01);
        QPen pen(isMajor ? theme.pianoRollGrid.lighter(130) : theme.pianoRollGrid,
                 isMajor ? 0.8 : 0.4);
        auto* line = scene_->addLine(x, 0, x, sceneH, pen);
        line->setZValue(-1);
        gridLines_.push_back(line);
    }
}

void NoteGrid::expandClipToFitNotes()
{
    if (!clip_) return;

    auto& ts = clip_->edit.tempoSequence;
    double clipStartBeat = ts.toBeats(clip_->getPosition().getStart()).inBeats();
    double clipEndBeat = ts.toBeats(clip_->getPosition().getEnd()).inBeats();
    double clipLenBeats = clipEndBeat - clipStartBeat;

    auto& seq = clip_->getSequence();
    double maxNoteEnd = clipLenBeats;
    bool needsExpand = false;

    for (auto* note : seq.getNotes()) {
        double noteEnd = note->getStartBeat().inBeats() + note->getLengthBeats().inBeats();
        if (noteEnd > clipLenBeats) {
            maxNoteEnd = std::max(maxNoteEnd, noteEnd);
            needsExpand = true;
        }
    }

    if (needsExpand) {
        double newEndAbsBeat = clipStartBeat + maxNoteEnd;
        auto newEndTime = ts.toTime(tracktion::BeatPosition::fromBeats(newEndAbsBeat));
        clip_->setEnd(newEndTime, false);
        updateSceneSize();
    }
}

void NoteGrid::rebuildNotes()
{
    for (auto* item : noteItems_) scene_->removeItem(item);
    qDeleteAll(noteItems_);
    noteItems_.clear();

    expandClipToFitNotes();
    drawBackground();

    if (!clip_) return;

    auto& seq = clip_->getSequence();
    for (auto* note : seq.getNotes()) {
        auto* item = new NoteItem(note, clip_, pixelsPerBeat_, noteRowHeight_, 0);
        item->updateGeometry(pixelsPerBeat_, noteRowHeight_, 0, TOTAL_NOTES);
        item->setGridSnapper(&snapper_);
        item->setRefreshCallback([this]() {
            QTimer::singleShot(0, this, [this]() {
                expandClipToFitNotes();
                rebuildNotes();
                emit notesChanged();
            });
        });
        item->setZValue(2);
        scene_->addItem(item);
        noteItems_.push_back(item);
    }
}

void NoteGrid::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        setFocus(Qt::MouseFocusReason);

    if (editMode_ == EditMode::Draw && event->button() == Qt::LeftButton) {
        beginDrawPreview(mapToScene(event->pos()));
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void NoteGrid::mouseMoveEvent(QMouseEvent* event)
{
    if (editMode_ == EditMode::Draw && isDrawingNote_) {
        updateDrawPreview(mapToScene(event->pos()));
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void NoteGrid::mouseReleaseEvent(QMouseEvent* event)
{
    if (editMode_ == EditMode::Draw && event->button() == Qt::LeftButton && isDrawingNote_) {
        commitDrawPreview(mapToScene(event->pos()));
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void NoteGrid::wheelEvent(QWheelEvent* event)
{
    if (editMode_ == EditMode::Draw && isDrawingNote_) {
        const int raw = event->angleDelta().y() / 120;
        const int velocityStep = (raw >= 0) ? std::max(1, raw) : std::min(-1, raw);
        drawVelocity_ = std::clamp(drawVelocity_ + velocityStep * 4, 1, 127);
        defaultDrawVelocity_ = drawVelocity_;
        updateDrawPreviewAppearance();
        event->accept();
        return;
    }

    if (event->modifiers() & Qt::ControlModifier) {
        double factor = std::pow(1.15, event->angleDelta().y() / 120.0);
        setPixelsPerBeat(pixelsPerBeat_ * factor);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void NoteGrid::deleteSelectedNotes()
{
    if (!clip_) return;
    auto& seq = clip_->getSequence();
    auto* um = &clip_->edit.getUndoManager();

    std::vector<te::MidiNote*> toDelete;
    for (auto* item : noteItems_) {
        if (item->isSelected())
            toDelete.push_back(item->note());
    }
    for (auto* note : toDelete)
        seq.removeNote(*note, um);

    rebuildNotes();
    emit notesChanged();
}

void NoteGrid::selectAllNotes()
{
    for (auto* item : noteItems_)
        item->setSelected(true);
}

void NoteGrid::quantizeNotes()
{
    if (!clip_) return;
    auto& seq = clip_->getSequence();
    auto* um = &clip_->edit.getUndoManager();

    double grid = snapper_.gridIntervalBeats();
    if (grid <= 0.0) grid = 0.25;

    um->beginNewTransaction("Quantize Notes");
    for (auto* note : seq.getNotes()) {
        double startBeat = note->getStartBeat().inBeats();
        double snapped = std::round(startBeat / grid) * grid;
        double len = note->getLengthBeats().inBeats();
        double snappedLen = std::max(std::min(len, grid), std::round(len / grid) * grid);
        note->setStartAndLength(
            tracktion::BeatPosition::fromBeats(snapped),
            tracktion::BeatDuration::fromBeats(snappedLen), um);
    }

    rebuildNotes();
    emit notesChanged();
}

void NoteGrid::scrollToMidiNote(int midiNote)
{
    midiNote = std::clamp(midiNote, 0, TOTAL_NOTES - 1);
    const int row = (TOTAL_NOTES - 1) - midiNote;
    const int targetY = static_cast<int>(row * noteRowHeight_ + noteRowHeight_ * 0.5);
    const int centeredValue = targetY - viewport()->height() / 2;
    verticalScrollBar()->setValue(std::max(0, centeredValue));
}

void NoteGrid::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelectedNotes();
        event->accept();
        return;
    }
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_A) {
        selectAllNotes();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void NoteGrid::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;
    menu.setAccessibleName("Note Grid Context Menu");

    menu.addAction("Select All", this, &NoteGrid::selectAllNotes);
    menu.addAction("Delete Selected", this, &NoteGrid::deleteSelectedNotes);
    menu.addSeparator();
    menu.addAction("Quantize Notes", this, &NoteGrid::quantizeNotes);

    if (clip_) {
        menu.addSeparator();
        menu.addAction("Add Note Here", [this, event]() {
            double sceneX = mapToScene(event->pos()).x();
            double sceneY = mapToScene(event->pos()).y();
            double beat = sceneX / pixelsPerBeat_;
            beat = snapper_.snapBeat(beat);
            if (beat < 0) beat = 0;

            int row = static_cast<int>(sceneY / noteRowHeight_);
            int pitch = (TOTAL_NOTES - 1) - row;
            pitch = std::clamp(pitch, 0, 127);

            double length = std::max(0.25, snapper_.gridIntervalBeats());
            auto& seq = clip_->getSequence();
            auto* um = &clip_->edit.getUndoManager();
            seq.addNote(pitch,
                        tracktion::BeatPosition::fromBeats(beat),
                        tracktion::BeatDuration::fromBeats(length),
                        100, 0, um);
            rebuildNotes();
            emit notesChanged();
        });
    }

    menu.exec(event->globalPos());
}

void NoteGrid::showEvent(QShowEvent* event)
{
    QGraphicsView::showEvent(event);

    if (!initialVerticalScrollSet_) {
        scrollToMidiNote(60); // Middle C
        initialVerticalScrollSet_ = true;
        emit verticalScrollChanged(verticalScrollBar()->value());
    }
}

void NoteGrid::beginDrawPreview(const QPointF& scenePos)
{
    if (!clip_) return;

    double beat = scenePos.x() / pixelsPerBeat_;
    beat = snapper_.snapBeat(beat);
    if (beat < 0.0) beat = 0.0;

    const int row = static_cast<int>(scenePos.y() / noteRowHeight_);
    drawPitch_ = std::clamp((TOTAL_NOTES - 1) - row, 0, 127);
    drawVelocity_ = std::clamp(defaultDrawVelocity_, 1, 127);
    drawStartBeat_ = beat;
    drawCurrentBeat_ = beat;
    isDrawingNote_ = true;

    if (!drawPreviewItem_) {
        auto& theme = ThemeManager::instance().current();
        drawPreviewItem_ = scene_->addRect(0, 0, 1.0, std::max(1.0, noteRowHeight_ - 1.0),
                                           QPen(theme.pianoRollNoteSelected.lighter(120), 1.0, Qt::DashLine),
                                           QBrush(theme.pianoRollNote));
        drawPreviewItem_->setZValue(3.5);
    }
    if (!drawPreviewText_) {
        drawPreviewText_ = scene_->addSimpleText("");
        drawPreviewText_->setZValue(3.6);
    }

    updateDrawPreview(scenePos);
}

void NoteGrid::updateDrawPreview(const QPointF& scenePos)
{
    if (!isDrawingNote_ || !drawPreviewItem_) return;

    double beat = scenePos.x() / pixelsPerBeat_;
    beat = snapper_.snapBeat(beat);
    if (beat < 0.0) beat = 0.0;
    drawCurrentBeat_ = beat;

    double minLength = snapper_.gridIntervalBeats();
    if (minLength <= 0.0) minLength = 0.25;

    const double leftBeat = std::min(drawStartBeat_, drawCurrentBeat_);
    const double rightBeat = std::max(drawStartBeat_, drawCurrentBeat_);
    const double lengthBeats = std::max(minLength, rightBeat - leftBeat);

    const double x = leftBeat * pixelsPerBeat_;
    const int row = (TOTAL_NOTES - 1) - drawPitch_;
    const double y = row * noteRowHeight_;
    const double width = std::max(1.0, lengthBeats * pixelsPerBeat_);
    const double height = std::max(1.0, noteRowHeight_ - 1.0);

    drawPreviewItem_->setRect(0, 0, width, height);
    drawPreviewItem_->setPos(x, y);
    updateDrawPreviewAppearance();
}

void NoteGrid::commitDrawPreview(const QPointF& scenePos)
{
    if (!clip_ || !isDrawingNote_) {
        clearDrawPreview();
        return;
    }

    updateDrawPreview(scenePos);

    double minLength = snapper_.gridIntervalBeats();
    if (minLength <= 0.0) minLength = 0.25;

    const double leftBeat = std::min(drawStartBeat_, drawCurrentBeat_);
    const double rightBeat = std::max(drawStartBeat_, drawCurrentBeat_);
    const double lengthBeats = std::max(minLength, rightBeat - leftBeat);

    auto& seq = clip_->getSequence();
    auto* um = &clip_->edit.getUndoManager();
    seq.addNote(drawPitch_,
                tracktion::BeatPosition::fromBeats(leftBeat),
                tracktion::BeatDuration::fromBeats(lengthBeats),
                drawVelocity_, 0, um);

    clearDrawPreview();
    rebuildNotes();
    emit notesChanged();
}

void NoteGrid::clearDrawPreview()
{
    isDrawingNote_ = false;

    if (drawPreviewItem_) {
        scene_->removeItem(drawPreviewItem_);
        delete drawPreviewItem_;
        drawPreviewItem_ = nullptr;
    }
    if (drawPreviewText_) {
        scene_->removeItem(drawPreviewText_);
        delete drawPreviewText_;
        drawPreviewText_ = nullptr;
    }
}

QString NoteGrid::midiNoteName(int midiNote) const
{
    static const char* kNames[12] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    midiNote = std::clamp(midiNote, 0, 127);
    const int octave = (midiNote / 12) - 1;
    const int semitone = midiNote % 12;
    return QString("%1%2").arg(kNames[semitone]).arg(octave);
}

void NoteGrid::updateDrawPreviewAppearance()
{
    if (!drawPreviewItem_ || !drawPreviewText_) return;

    auto& theme = ThemeManager::instance().current();
    QColor noteColor = theme.pianoRollNoteSelected;
    noteColor.setAlphaF(0.35 + 0.55 * (drawVelocity_ / 127.0));

    drawPreviewItem_->setBrush(QBrush(noteColor));
    drawPreviewItem_->setPen(QPen(theme.pianoRollNoteSelected.lighter(130), 1.0, Qt::DashLine));

    const QString previewText = QString("%1  Vel %2")
        .arg(midiNoteName(drawPitch_))
        .arg(drawVelocity_);
    drawPreviewText_->setText(previewText);
    drawPreviewText_->setBrush(theme.text);
    drawPreviewText_->setPos(drawPreviewItem_->pos().x() + 4.0,
                             drawPreviewItem_->pos().y() - 16.0);
}

} // namespace freedaw
