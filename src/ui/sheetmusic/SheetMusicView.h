#pragma once

#include "ScoreScene.h"
#include <QWidget>
#include <QGraphicsView>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QToolBar>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion::engine;

namespace OpenDaw {

class EditManager;

class SheetMusicView : public QWidget {
    Q_OBJECT

public:
    explicit SheetMusicView(QWidget* parent = nullptr);

    void setClip(te::MidiClip* clip, EditManager* editMgr);
    void refresh();
    te::MidiClip* clip() const { return clip_; }

private:
    void buildToolbar();
    void rebuildScore();
    void onZoomChanged(int value);

    te::MidiClip* clip_ = nullptr;
    EditManager* editMgr_ = nullptr;
    int keySig_ = 0;

    QToolBar* toolbar_ = nullptr;
    QLabel* clipNameLabel_ = nullptr;
    QComboBox* keySigCombo_ = nullptr;
    QSlider* zoomSlider_ = nullptr;
    QGraphicsView* view_ = nullptr;
    ScoreScene* scene_ = nullptr;
};

} // namespace OpenDaw
