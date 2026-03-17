#pragma once

#include "PianoKeyboard.h"
#include "NoteGrid.h"
#include "VelocityLane.h"
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSplitter>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion::engine;

namespace freedaw {

class PianoRollEditor : public QWidget {
    Q_OBJECT

public:
    explicit PianoRollEditor(QWidget* parent = nullptr);

    void setClip(te::MidiClip* clip);
    void refresh();
    te::MidiClip* clip() const { return clip_; }

signals:
    void notesChanged();

private:
    void syncKeyboardScroll();
    void syncVelocityScroll();
    void onSnapModeChanged(int index);
    void onDrawModeToggled(bool checked);
    void onEditModeToggled(bool checked);
    void onNotesChanged();

    te::MidiClip* clip_ = nullptr;

    QLabel* clipNameLabel_;
    QPushButton* drawModeBtn_ = nullptr;
    QPushButton* editModeBtn_ = nullptr;
    QComboBox* snapCombo_;
    PianoKeyboard* keyboard_;
    NoteGrid* noteGrid_;
    VelocityLane* velocityLane_;
};

} // namespace freedaw
