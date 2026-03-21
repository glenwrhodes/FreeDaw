#include "WaveformCache.h"
#include <algorithm>

namespace OpenDaw {

WaveformCache& WaveformCache::instance()
{
    static WaveformCache cache;
    return cache;
}

WaveformData WaveformCache::getWaveform(const juce::File& file, int numPoints)
{
    QString key = QString::fromStdString(file.getFullPathName().toStdString());

    auto it = cache_.find(key);
    if (it != cache_.end() && it->numPoints == numPoints)
        return it->data;

    WaveformData result;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (!reader)
        return result;

    result.sampleRate = reader->sampleRate;
    result.totalSamples = reader->lengthInSamples;
    result.numChannels = static_cast<int>(reader->numChannels);
    result.bitsPerSample = static_cast<int>(reader->bitsPerSample);
    result.minValues.resize(size_t(numPoints));
    result.maxValues.resize(size_t(numPoints));

    result.channelMin.resize(size_t(result.numChannels));
    result.channelMax.resize(size_t(result.numChannels));
    for (int ch = 0; ch < result.numChannels; ++ch) {
        result.channelMin[size_t(ch)].resize(size_t(numPoints));
        result.channelMax[size_t(ch)].resize(size_t(numPoints));
    }

    int64_t samplesPerPoint = reader->lengthInSamples / numPoints;
    if (samplesPerPoint < 1)
        samplesPerPoint = 1;

    juce::AudioBuffer<float> buffer(int(reader->numChannels),
                                     int(samplesPerPoint));

    for (int i = 0; i < numPoints; ++i) {
        int64_t start = i * samplesPerPoint;
        int64_t count = std::min(samplesPerPoint,
                                  reader->lengthInSamples - start);
        if (count <= 0) break;

        reader->read(&buffer, 0, int(count), start, true, true);

        float mixMin = 0.0f, mixMax = 0.0f;
        for (int ch = 0; ch < int(reader->numChannels); ++ch) {
            auto* data = buffer.getReadPointer(ch);
            float chMin = 0.0f, chMax = 0.0f;
            for (int s = 0; s < int(count); ++s) {
                chMin = std::min(chMin, data[s]);
                chMax = std::max(chMax, data[s]);
            }
            result.channelMin[size_t(ch)][size_t(i)] = chMin;
            result.channelMax[size_t(ch)][size_t(i)] = chMax;
            mixMin = std::min(mixMin, chMin);
            mixMax = std::max(mixMax, chMax);
        }
        result.minValues[size_t(i)] = mixMin;
        result.maxValues[size_t(i)] = mixMax;
    }

    cache_[key] = CacheEntry{result, numPoints};
    return result;
}

void WaveformCache::invalidate(const juce::File& file)
{
    QString key = QString::fromStdString(file.getFullPathName().toStdString());
    cache_.remove(key);
}

void WaveformCache::clear()
{
    cache_.clear();
}

} // namespace OpenDaw
