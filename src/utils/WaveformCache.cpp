#include "WaveformCache.h"

namespace freedaw {

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
    result.minValues.resize(size_t(numPoints));
    result.maxValues.resize(size_t(numPoints));

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

        float minVal = 0.0f, maxVal = 0.0f;
        for (int ch = 0; ch < int(reader->numChannels); ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int s = 0; s < int(count); ++s) {
                if (data[s] < minVal) minVal = data[s];
                if (data[s] > maxVal) maxVal = data[s];
            }
        }
        result.minValues[size_t(i)] = minVal;
        result.maxValues[size_t(i)] = maxVal;
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

} // namespace freedaw
