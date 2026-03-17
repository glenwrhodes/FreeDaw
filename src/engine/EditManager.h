#pragma once

#include "AudioEngine.h"
#include <tracktion_engine/tracktion_engine.h>
#include <QObject>
#include <QString>
#include <QList>
#include <unordered_set>
#include <memory>

namespace freedaw {

struct InputSource {
    juce::String deviceName;
    QString displayName;
};

class EditManager : public QObject {
    Q_OBJECT

public:
    explicit EditManager(AudioEngine& engine, QObject* parent = nullptr);
    ~EditManager() override;

    void newEdit();
    bool loadEdit(const juce::File& file);
    bool saveEdit();
    bool saveEditAs(const juce::File& file);

    te::Edit*      edit()      { return edit_.get(); }
    te::TransportControl& transport();

    te::AudioTrack* addAudioTrack();
    te::AudioTrack* addMidiTrack();
    void removeTrack(te::Track* track);
    int  trackCount() const;
    te::AudioTrack* getAudioTrack(int index) const;
    juce::Array<te::AudioTrack*> getAudioTracks() const;

    void addAudioClipToTrack(te::AudioTrack& track,
                             const juce::File& audioFile,
                             double startBeat);

    te::MidiClip* addMidiClipToTrack(te::AudioTrack& track,
                                     double startBeat, double lengthBeats);
    te::MidiClip* importMidiFileToTrack(te::AudioTrack& track,
                                        const juce::File& midiFile,
                                        double startBeat);

    bool isMidiTrack(te::AudioTrack* track) const;
    bool isTrackMono(te::AudioTrack* track) const;
    void setTrackMono(te::AudioTrack& track, bool mono);
    te::Plugin* getTrackInstrument(te::AudioTrack* track) const;
    void setTrackInstrument(te::AudioTrack& track,
                            const juce::PluginDescription& desc);
    void removeTrackInstrument(te::AudioTrack& track);

    void markAsMidiTrack(te::AudioTrack* track);

    void trimNotesToClipBounds(te::MidiClip& clip);
    bool isClipValid(te::Clip* clip) const;

    QList<InputSource> getAvailableInputSources() const;
    void assignInputToTrack(te::AudioTrack& track, const juce::String& deviceName);
    void clearTrackInput(te::AudioTrack& track);
    QString getTrackInputName(te::AudioTrack* track) const;
    void setTrackRecordEnabled(te::AudioTrack& track, bool enabled);
    bool isTrackRecordEnabled(te::AudioTrack* track) const;

    void undo();
    void redo();

    double getBpm() const;
    void   setBpm(double bpm);
    int    getTimeSigNumerator() const;
    int    getTimeSigDenominator() const;
    void   setTimeSignature(int num, int den);

    juce::File currentFile() const { return currentFile_; }

signals:
    void aboutToChangeEdit();
    void editChanged();
    void tracksChanged();
    void transportStateChanged();
    void midiClipDoubleClicked(te::MidiClip* clip);
    void midiClipSelected(te::MidiClip* clip);
    void midiClipModified(te::MidiClip* clip);

private:
    void teardownCurrentEdit();
    void createDefaultEdit();
    void ensureLevelMetersOnAllTracks();
    void enableAllWaveInputDevices();

    AudioEngine& audioEngine_;
    std::unique_ptr<te::Edit> edit_;
    juce::File currentFile_;
    std::unordered_set<uint64_t> midiTrackIds_;
};

} // namespace freedaw
