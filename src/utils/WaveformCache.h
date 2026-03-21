#pragma once

#include <tracktion_engine/tracktion_engine.h>
#include <QObject>
#include <QHash>
#include <QString>
#include <vector>
#include <memory>

namespace OpenDaw {

struct WaveformData {
    std::vector<float> minValues;
    std::vector<float> maxValues;
    std::vector<std::vector<float>> channelMin;
    std::vector<std::vector<float>> channelMax;
    int numChannels = 0;
    int bitsPerSample = 0;
    double sampleRate = 0;
    int64_t totalSamples = 0;
};

class WaveformCache : public QObject {
    Q_OBJECT

public:
    static WaveformCache& instance();

    WaveformData getWaveform(const juce::File& file, int numPoints);
    void invalidate(const juce::File& file);
    void clear();

private:
    WaveformCache() = default;

    struct CacheEntry {
        WaveformData data;
        int numPoints = 0;
    };
    QHash<QString, CacheEntry> cache_;
};

} // namespace OpenDaw
