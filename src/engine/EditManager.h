#pragma once

#include "AudioEngine.h"
#include <tracktion_engine/tracktion_engine.h>
#include <QObject>
#include <memory>

namespace freedaw {

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
    void removeTrack(te::Track* track);
    int  trackCount() const;
    te::AudioTrack* getAudioTrack(int index) const;
    juce::Array<te::AudioTrack*> getAudioTracks() const;

    void addAudioClipToTrack(te::AudioTrack& track,
                             const juce::File& audioFile,
                             double startBeat);

    double getBpm() const;
    void   setBpm(double bpm);
    int    getTimeSigNumerator() const;
    int    getTimeSigDenominator() const;
    void   setTimeSignature(int num, int den);

    juce::File currentFile() const { return currentFile_; }

signals:
    void editChanged();
    void tracksChanged();
    void transportStateChanged();

private:
    void createDefaultEdit();
    void ensureLevelMetersOnAllTracks();

    AudioEngine& audioEngine_;
    std::unique_ptr<te::Edit> edit_;
    juce::File currentFile_;
};

} // namespace freedaw
