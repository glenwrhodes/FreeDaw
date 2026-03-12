#include "EditManager.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace freedaw {

EditManager::EditManager(AudioEngine& engine, QObject* parent)
    : QObject(parent), audioEngine_(engine)
{
    createDefaultEdit();
}

EditManager::~EditManager() = default;

void EditManager::createDefaultEdit()
{
    auto& eng = audioEngine_.engine();
    edit_ = te::createEmptyEdit(eng, juce::File());
    edit_->ensureNumberOfAudioTracks(1);
    ensureLevelMetersOnAllTracks();
    currentFile_ = juce::File();
    emit editChanged();
    emit tracksChanged();
}

void EditManager::newEdit()
{
    createDefaultEdit();
}

bool EditManager::loadEdit(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    auto& eng = audioEngine_.engine();
    edit_ = te::loadEditFromFile(eng, file);
    if (!edit_)
        return false;

    ensureLevelMetersOnAllTracks();
    currentFile_ = file;
    emit editChanged();
    emit tracksChanged();
    return true;
}

bool EditManager::saveEdit()
{
    if (!edit_ || currentFile_ == juce::File())
        return false;
    te::EditFileOperations(*edit_).save(true, true, false);
    return true;
}

bool EditManager::saveEditAs(const juce::File& file)
{
    if (!edit_)
        return false;
    edit_->editFileRetriever = [file]() { return file; };
    te::EditFileOperations(*edit_).save(true, true, false);
    currentFile_ = file;
    return true;
}

te::TransportControl& EditManager::transport()
{
    jassert(edit_ != nullptr);
    return edit_->getTransport();
}

te::AudioTrack* EditManager::addAudioTrack()
{
    if (!edit_)
        return nullptr;
    edit_->ensureNumberOfAudioTracks(trackCount() + 1);
    auto tracks = getAudioTracks();
    auto* newTrack = tracks.getLast();
    if (newTrack) {
        bool hasLevelMeter = false;
        for (auto* p : newTrack->pluginList.getPlugins())
            if (dynamic_cast<te::LevelMeterPlugin*>(p))
                hasLevelMeter = true;
        if (!hasLevelMeter) {
            if (auto p = edit_->getPluginCache().createNewPlugin(
                    juce::String(te::LevelMeterPlugin::xmlTypeName), {}))
                newTrack->pluginList.insertPlugin(p, -1, nullptr);
        }
    }
    emit tracksChanged();
    return newTrack;
}

void EditManager::removeTrack(te::Track* track)
{
    if (!edit_ || !track)
        return;
    edit_->deleteTrack(track);
    emit tracksChanged();
}

int EditManager::trackCount() const
{
    return edit_ ? getAudioTracks().size() : 0;
}

te::AudioTrack* EditManager::getAudioTrack(int index) const
{
    auto tracks = getAudioTracks();
    if (index >= 0 && index < tracks.size())
        return tracks[index];
    return nullptr;
}

juce::Array<te::AudioTrack*> EditManager::getAudioTracks() const
{
    juce::Array<te::AudioTrack*> result;
    if (!edit_)
        return result;
    for (auto* track : te::getAudioTracks(*edit_))
        result.add(track);
    return result;
}

void EditManager::addAudioClipToTrack(te::AudioTrack& track,
                                       const juce::File& audioFile,
                                       double startBeat)
{
    double fileDurationSecs = 5.0;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    if (auto reader = std::unique_ptr<juce::AudioFormatReader>(
            formatManager.createReaderFor(audioFile))) {
        fileDurationSecs = double(reader->lengthInSamples) / reader->sampleRate;
    }

    auto& ts = edit_->tempoSequence;
    auto startTime = ts.toTime(tracktion::BeatPosition::fromBeats(startBeat));
    auto endTime   = tracktion::TimePosition::fromSeconds(
                         startTime.inSeconds() + fileDurationSecs);

    auto clipRef = te::insertWaveClip(track,
                                       audioFile.getFileNameWithoutExtension(),
                                       audioFile,
                                       {{startTime, endTime}},
                                       te::DeleteExistingClips::no);
    if (clipRef != nullptr) {
        clipRef->setOffset(tracktion::TimeDuration::fromSeconds(0.0));
        clipRef->setSpeedRatio(1.0);
    }

    emit editChanged();
}

double EditManager::getBpm() const
{
    if (!edit_)
        return 120.0;
    return edit_->tempoSequence.getTempo(0)->getBpm();
}

void EditManager::setBpm(double bpm)
{
    if (!edit_)
        return;
    edit_->tempoSequence.getTempo(0)->setBpm(bpm);
    emit editChanged();
}

int EditManager::getTimeSigNumerator() const
{
    if (!edit_)
        return 4;
    return edit_->tempoSequence.getTimeSig(0)->numerator;
}

int EditManager::getTimeSigDenominator() const
{
    if (!edit_)
        return 4;
    return edit_->tempoSequence.getTimeSig(0)->denominator;
}

void EditManager::setTimeSignature(int num, int den)
{
    if (!edit_)
        return;
    auto* ts = edit_->tempoSequence.getTimeSig(0);
    ts->numerator   = num;
    ts->denominator = den;
    emit editChanged();
}

void EditManager::ensureLevelMetersOnAllTracks()
{
    if (!edit_)
        return;
    for (auto* track : te::getAudioTracks(*edit_)) {
        bool hasLevelMeter = false;
        for (auto* plugin : track->pluginList.getPlugins()) {
            if (dynamic_cast<te::LevelMeterPlugin*>(plugin)) {
                hasLevelMeter = true;
                break;
            }
        }
        if (!hasLevelMeter) {
            if (auto p = edit_->getPluginCache().createNewPlugin(
                    juce::String(te::LevelMeterPlugin::xmlTypeName), {}))
                track->pluginList.insertPlugin(p, -1, nullptr);
        }
    }
}

} // namespace freedaw
