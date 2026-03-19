#pragma once

#include "WaveformSelection.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace freedaw {

class AudioFileOperations {
public:
    static bool deleteSelection(const juce::File& file, const WaveformSelection& sel);
    static bool silenceSelection(const juce::File& file, const WaveformSelection& sel);
    static bool adjustSelectionGain(const juce::File& file, const WaveformSelection& sel, float dB);
    static bool fadeInSelection(const juce::File& file, const WaveformSelection& sel);
    static bool fadeOutSelection(const juce::File& file, const WaveformSelection& sel);
    static bool reverseSelection(const juce::File& file, const WaveformSelection& sel);
    static bool normalizeSelection(const juce::File& file, const WaveformSelection& sel);

    static juce::AudioBuffer<float> copySelection(const juce::File& file, const WaveformSelection& sel);
    static bool pasteAtPosition(const juce::File& file, juce::int64 samplePos,
                                const juce::AudioBuffer<float>& buffer, double sampleRate);

    static bool insertSilence(const juce::File& file, juce::int64 samplePos, juce::int64 numSamples);
    static bool removeDCOffset(const juce::File& file);
    static bool swapChannels(const juce::File& file);
    static bool mixToMono(const juce::File& file);
    static bool crossfadeAtPoint(const juce::File& file, juce::int64 samplePos, int crossfadeSamples);

private:
    struct FileInfo {
        int numChannels = 0;
        juce::int64 numSamples = 0;
        double sampleRate = 0;
        int bitsPerSample = 0;
    };

    static bool readEntireFile(const juce::File& file,
                               juce::AudioBuffer<float>& buffer, FileInfo& info);
    static bool writeEntireFile(const juce::File& file,
                                const juce::AudioBuffer<float>& buffer, const FileInfo& info);
};

} // namespace freedaw
