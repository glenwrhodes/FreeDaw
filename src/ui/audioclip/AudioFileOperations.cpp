#include "AudioFileOperations.h"
#include <cmath>
#include <algorithm>

namespace freedaw {

bool AudioFileOperations::readEntireFile(const juce::File& file,
                                          juce::AudioBuffer<float>& buffer,
                                          FileInfo& info)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
    if (!reader) return false;

    info.numChannels = static_cast<int>(reader->numChannels);
    info.numSamples = static_cast<juce::int64>(reader->lengthInSamples);
    info.sampleRate = reader->sampleRate;
    info.bitsPerSample = static_cast<int>(reader->bitsPerSample);

    buffer.setSize(info.numChannels, static_cast<int>(info.numSamples));
    reader->read(&buffer, 0, static_cast<int>(info.numSamples), 0, true, true);
    return true;
}

bool AudioFileOperations::writeEntireFile(const juce::File& file,
                                           const juce::AudioBuffer<float>& buffer,
                                           const FileInfo& info)
{
    auto tempFile = file.getSiblingFile(file.getFileNameWithoutExtension() + "_tmp.wav");

    {
        juce::WavAudioFormat wavFmt;
        auto outStream = std::unique_ptr<juce::OutputStream>(tempFile.createOutputStream());
        if (!outStream) return false;

        auto opts = juce::AudioFormatWriterOptions{}
            .withSampleRate(info.sampleRate)
            .withNumChannels(buffer.getNumChannels())
            .withBitsPerSample(info.bitsPerSample);

        auto writer = wavFmt.createWriterFor(outStream, opts);
        if (!writer) return false;
        writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }

    return tempFile.moveFileTo(file);
}

bool AudioFileOperations::deleteSelection(const juce::File& file, const WaveformSelection& sel)
{
    if (sel.isEmpty()) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto start = static_cast<int>(std::clamp(sel.startSample, juce::int64(0), info.numSamples));
    auto end = static_cast<int>(std::clamp(sel.endSample, juce::int64(0), info.numSamples));
    if (start >= end) return false;

    int deleteLen = end - start;
    int newLen = buf.getNumSamples() - deleteLen;
    if (newLen <= 0) return false;

    juce::AudioBuffer<float> result(info.numChannels, newLen);
    for (int ch = 0; ch < info.numChannels; ++ch) {
        if (start > 0)
            result.copyFrom(ch, 0, buf, ch, 0, start);
        int remaining = buf.getNumSamples() - end;
        if (remaining > 0)
            result.copyFrom(ch, start, buf, ch, end, remaining);
    }

    return writeEntireFile(file, result, info);
}

bool AudioFileOperations::silenceSelection(const juce::File& file, const WaveformSelection& sel)
{
    if (sel.isEmpty()) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto start = static_cast<int>(std::clamp(sel.startSample, juce::int64(0), info.numSamples));
    auto end = static_cast<int>(std::clamp(sel.endSample, juce::int64(0), info.numSamples));

    for (int ch = 0; ch < info.numChannels; ++ch)
        buf.clear(ch, start, end - start);

    return writeEntireFile(file, buf, info);
}

bool AudioFileOperations::adjustSelectionGain(const juce::File& file,
                                               const WaveformSelection& sel, float dB)
{
    if (sel.isEmpty()) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto start = static_cast<int>(std::clamp(sel.startSample, juce::int64(0), info.numSamples));
    auto end = static_cast<int>(std::clamp(sel.endSample, juce::int64(0), info.numSamples));

    float linear = std::pow(10.0f, dB / 20.0f);
    for (int ch = 0; ch < info.numChannels; ++ch)
        buf.applyGain(ch, start, end - start, linear);

    return writeEntireFile(file, buf, info);
}

bool AudioFileOperations::fadeInSelection(const juce::File& file, const WaveformSelection& sel)
{
    if (sel.isEmpty()) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto start = static_cast<int>(std::clamp(sel.startSample, juce::int64(0), info.numSamples));
    auto end = static_cast<int>(std::clamp(sel.endSample, juce::int64(0), info.numSamples));
    int len = end - start;
    if (len <= 0) return false;

    for (int ch = 0; ch < info.numChannels; ++ch) {
        auto* data = buf.getWritePointer(ch);
        for (int i = 0; i < len; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(len);
            float gain = t * t;
            data[start + i] *= gain;
        }
    }

    return writeEntireFile(file, buf, info);
}

bool AudioFileOperations::fadeOutSelection(const juce::File& file, const WaveformSelection& sel)
{
    if (sel.isEmpty()) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto start = static_cast<int>(std::clamp(sel.startSample, juce::int64(0), info.numSamples));
    auto end = static_cast<int>(std::clamp(sel.endSample, juce::int64(0), info.numSamples));
    int len = end - start;
    if (len <= 0) return false;

    for (int ch = 0; ch < info.numChannels; ++ch) {
        auto* data = buf.getWritePointer(ch);
        for (int i = 0; i < len; ++i) {
            float t = 1.0f - static_cast<float>(i) / static_cast<float>(len);
            float gain = t * t;
            data[start + i] *= gain;
        }
    }

    return writeEntireFile(file, buf, info);
}

bool AudioFileOperations::reverseSelection(const juce::File& file, const WaveformSelection& sel)
{
    if (sel.isEmpty()) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto start = static_cast<int>(std::clamp(sel.startSample, juce::int64(0), info.numSamples));
    auto end = static_cast<int>(std::clamp(sel.endSample, juce::int64(0), info.numSamples));

    for (int ch = 0; ch < info.numChannels; ++ch) {
        auto* data = buf.getWritePointer(ch);
        std::reverse(data + start, data + end);
    }

    return writeEntireFile(file, buf, info);
}

bool AudioFileOperations::normalizeSelection(const juce::File& file, const WaveformSelection& sel)
{
    if (sel.isEmpty()) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto start = static_cast<int>(std::clamp(sel.startSample, juce::int64(0), info.numSamples));
    auto end = static_cast<int>(std::clamp(sel.endSample, juce::int64(0), info.numSamples));

    float peak = 0.0f;
    for (int ch = 0; ch < info.numChannels; ++ch) {
        auto range = juce::FloatVectorOperations::findMinAndMax(
            buf.getReadPointer(ch) + start, end - start);
        peak = std::max(peak, std::max(std::abs(range.getStart()), std::abs(range.getEnd())));
    }

    if (peak < 0.0001f) return true;

    float gain = 1.0f / peak;
    for (int ch = 0; ch < info.numChannels; ++ch)
        buf.applyGain(ch, start, end - start, gain);

    return writeEntireFile(file, buf, info);
}

juce::AudioBuffer<float> AudioFileOperations::copySelection(const juce::File& file,
                                                             const WaveformSelection& sel)
{
    juce::AudioBuffer<float> result;
    if (sel.isEmpty()) return result;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return result;

    auto start = static_cast<int>(std::clamp(sel.startSample, juce::int64(0), info.numSamples));
    auto end = static_cast<int>(std::clamp(sel.endSample, juce::int64(0), info.numSamples));
    int len = end - start;
    if (len <= 0) return result;

    result.setSize(info.numChannels, len);
    for (int ch = 0; ch < info.numChannels; ++ch)
        result.copyFrom(ch, 0, buf, ch, start, len);

    return result;
}

bool AudioFileOperations::pasteAtPosition(const juce::File& file, juce::int64 samplePos,
                                           const juce::AudioBuffer<float>& pasteBuffer,
                                           double /*sampleRate*/)
{
    if (pasteBuffer.getNumSamples() == 0) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto pos = static_cast<int>(std::clamp(samplePos, juce::int64(0), info.numSamples));
    int pasteLen = pasteBuffer.getNumSamples();
    int newLen = buf.getNumSamples() + pasteLen;
    int ch = std::min(info.numChannels, pasteBuffer.getNumChannels());

    juce::AudioBuffer<float> result(info.numChannels, newLen);
    for (int c = 0; c < info.numChannels; ++c) {
        if (pos > 0)
            result.copyFrom(c, 0, buf, c, 0, pos);
        if (c < ch)
            result.copyFrom(c, pos, pasteBuffer, c, 0, pasteLen);
        else
            result.clear(c, pos, pasteLen);
        int remaining = buf.getNumSamples() - pos;
        if (remaining > 0)
            result.copyFrom(c, pos + pasteLen, buf, c, pos, remaining);
    }

    return writeEntireFile(file, result, info);
}

bool AudioFileOperations::insertSilence(const juce::File& file, juce::int64 samplePos,
                                         juce::int64 numSamples)
{
    if (numSamples <= 0) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    auto pos = static_cast<int>(std::clamp(samplePos, juce::int64(0), info.numSamples));
    int silLen = static_cast<int>(numSamples);
    int newLen = buf.getNumSamples() + silLen;

    juce::AudioBuffer<float> result(info.numChannels, newLen);
    for (int ch = 0; ch < info.numChannels; ++ch) {
        if (pos > 0)
            result.copyFrom(ch, 0, buf, ch, 0, pos);
        result.clear(ch, pos, silLen);
        int remaining = buf.getNumSamples() - pos;
        if (remaining > 0)
            result.copyFrom(ch, pos + silLen, buf, ch, pos, remaining);
    }

    return writeEntireFile(file, result, info);
}

bool AudioFileOperations::removeDCOffset(const juce::File& file)
{
    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    for (int ch = 0; ch < info.numChannels; ++ch) {
        auto* data = buf.getWritePointer(ch);
        int n = buf.getNumSamples();

        double sum = 0.0;
        for (int i = 0; i < n; ++i)
            sum += data[i];
        float dc = static_cast<float>(sum / n);

        for (int i = 0; i < n; ++i)
            data[i] -= dc;
    }

    return writeEntireFile(file, buf, info);
}

bool AudioFileOperations::swapChannels(const juce::File& file)
{
    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    if (info.numChannels < 2) return false;

    int n = buf.getNumSamples();
    auto* left = buf.getWritePointer(0);
    auto* right = buf.getWritePointer(1);
    for (int i = 0; i < n; ++i)
        std::swap(left[i], right[i]);

    return writeEntireFile(file, buf, info);
}

bool AudioFileOperations::mixToMono(const juce::File& file)
{
    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    if (info.numChannels < 2) return true;

    int n = buf.getNumSamples();
    juce::AudioBuffer<float> mono(1, n);
    auto* out = mono.getWritePointer(0);

    for (int i = 0; i < n; ++i) {
        float sum = 0.0f;
        for (int ch = 0; ch < info.numChannels; ++ch)
            sum += buf.getReadPointer(ch)[i];
        out[i] = sum / info.numChannels;
    }

    info.numChannels = 1;
    return writeEntireFile(file, mono, info);
}

bool AudioFileOperations::crossfadeAtPoint(const juce::File& file, juce::int64 samplePos,
                                            int crossfadeSamples)
{
    if (crossfadeSamples <= 0) return false;

    juce::AudioBuffer<float> buf;
    FileInfo info;
    if (!readEntireFile(file, buf, info)) return false;

    int pos = static_cast<int>(std::clamp(samplePos, juce::int64(0), info.numSamples));
    int half = crossfadeSamples / 2;
    int fadeStart = std::max(0, pos - half);
    int fadeEnd = std::min(buf.getNumSamples(), pos + half);
    int fadeLen = fadeEnd - fadeStart;
    if (fadeLen <= 1) return true;

    for (int ch = 0; ch < info.numChannels; ++ch) {
        auto* data = buf.getWritePointer(ch);
        for (int i = 0; i < fadeLen; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(fadeLen);
            int idx = fadeStart + i;
            if (idx < pos)
                data[idx] *= (1.0f - t * 0.5f);
            else
                data[idx] *= (0.5f + t * 0.5f);
        }
    }

    return writeEntireFile(file, buf, info);
}

} // namespace freedaw
