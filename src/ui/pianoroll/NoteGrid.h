#pragma once

#include "NoteItem.h"
#include "ui/timeline/GridSnapper.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <tracktion_engine/tracktion_engine.h>
#include <vector>

class QShowEvent;

namespace te = tracktion::engine;

namespace freedaw {

class NoteGridScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit NoteGridScene(QObject* parent = nullptr);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

signals:
    void emptyAreaClicked(double beat, int pitch);
    void emptyAreaDoubleClicked(double sceneX, double sceneY);
};

class NoteGrid : public QGraphicsView {
    Q_OBJECT

public:
    enum class EditMode {
        Draw = 0,
        Edit = 1
    };

    explicit NoteGrid(QWidget* parent = nullptr);

    void setClip(te::MidiClip* clip);
    void setPixelsPerBeat(double ppb);
    void setNoteRowHeight(double h);
    double pixelsPerBeat() const { return pixelsPerBeat_; }
    double noteRowHeight() const { return noteRowHeight_; }

    GridSnapper& snapper() { return snapper_; }
    void setEditMode(EditMode mode);
    EditMode editMode() const { return editMode_; }

    void rebuildNotes();
    void deleteSelectedNotes();
    void selectAllNotes();
    void quantizeNotes();
    void scrollToMidiNote(int midiNote);

signals:
    void notesChanged();
    void zoomChanged();
    void verticalScrollChanged(int value);
    void horizontalScrollChanged(int value);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void drawBackground();
    void updateSceneSize();
    void expandClipToFitNotes();
    void beginDrawPreview(const QPointF& scenePos);
    void updateDrawPreview(const QPointF& scenePos);
    void commitDrawPreview(const QPointF& scenePos);
    void clearDrawPreview();
    QString midiNoteName(int midiNote) const;
    void updateDrawPreviewAppearance();

    te::MidiClip* clip_ = nullptr;
    NoteGridScene* scene_;
    GridSnapper snapper_;

    double pixelsPerBeat_ = 60.0;
    double noteRowHeight_ = 14.0;
    static constexpr int TOTAL_NOTES = 128;

    std::vector<NoteItem*> noteItems_;
    std::vector<QGraphicsRectItem*> bgItems_;
    std::vector<QGraphicsLineItem*> gridLines_;
    QGraphicsRectItem* clipRegionLeft_ = nullptr;
    QGraphicsRectItem* clipRegionRight_ = nullptr;
    QGraphicsLineItem* playheadLine_ = nullptr;
    bool initialVerticalScrollSet_ = false;

    EditMode editMode_ = EditMode::Edit;
    bool isDrawingNote_ = false;
    int drawPitch_ = 60;
    int drawVelocity_ = 100;
    int defaultDrawVelocity_ = 100;
    double drawStartBeat_ = 0.0;
    double drawCurrentBeat_ = 0.0;
    QGraphicsRectItem* drawPreviewItem_ = nullptr;
    QGraphicsSimpleTextItem* drawPreviewText_ = nullptr;
};

} // namespace freedaw
