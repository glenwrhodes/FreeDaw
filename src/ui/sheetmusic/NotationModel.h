#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion::engine;

namespace OpenDaw {

enum class NoteValue { Whole, Half, Quarter, Eighth, Sixteenth };
enum class Accidental { None, Sharp, Flat, Natural };
enum class StaffKind { Treble, Bass };

struct NoteSpelling {
    int diatonic = 0;           // 0=C, 1=D, 2=E, 3=F, 4=G, 5=A, 6=B
    Accidental display = Accidental::None;
};

struct NotationNote {
    int midiNote = 60;
    NoteValue value = NoteValue::Quarter;
    bool dotted = false;
    double beatInMeasure = 0.0;
    int staffPosition = 0;
    Accidental accidental = Accidental::None;
    StaffKind staff = StaffKind::Treble;
    int stemDirection = -1;       // +1 = up, -1 = down
};

struct NotationRest {
    NoteValue value = NoteValue::Quarter;
    bool dotted = false;
    double beatInMeasure = 0.0;
    StaffKind staff = StaffKind::Treble;
};

struct NotationEvent {
    bool isRest = false;
    bool beamed = false;
    std::vector<NotationNote> notes;
    NotationRest rest;
    double beatInMeasure = 0.0;
    NoteValue value = NoteValue::Quarter;
    bool dotted = false;
    StaffKind staff = StaffKind::Treble;
};

struct BeamGroup {
    std::vector<int> eventIndices;
    StaffKind staff = StaffKind::Treble;
};

struct NotationMeasure {
    int measureNumber = 0;
    std::vector<NotationEvent> trebleEvents;
    std::vector<NotationEvent> bassEvents;
    std::vector<BeamGroup> trebleBeams;
    std::vector<BeamGroup> bassBeams;
};

double noteValueBeats(NoteValue v);
NoteValue quantizeDuration(double beats, bool& dotted);
NoteSpelling spellMidiNote(int midiNote, int keySig);
int spelledNoteToStaffPosition(int midiNote, int diatonic, StaffKind staff);
int stemDirectionForPosition(int staffPosition);

class NotationModel {
public:
    void buildFromClip(te::MidiClip* clip, int timeSigNum, int timeSigDen,
                       int keySig = 0);
    void clear();

    const std::vector<NotationMeasure>& measures() const { return measures_; }
    int timeSigNum() const { return timeSigNum_; }
    int timeSigDen() const { return timeSigDen_; }
    int keySig() const { return keySig_; }
    int measureCount() const { return static_cast<int>(measures_.size()); }

private:
    void insertRests(std::vector<NotationEvent>& events,
                     StaffKind staff, double beatsPerMeasure);
    void buildBeamGroups(const std::vector<NotationEvent>& events,
                         StaffKind staff,
                         std::vector<BeamGroup>& beamsOut);

    std::vector<NotationMeasure> measures_;
    int timeSigNum_ = 4;
    int timeSigDen_ = 4;
    int keySig_ = 0;
};

} // namespace OpenDaw
