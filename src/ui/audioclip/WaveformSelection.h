#pragma once

#include <juce_core/juce_core.h>

namespace freedaw {

struct WaveformSelection {
    juce::int64 startSample = 0;
    juce::int64 endSample = 0;

    bool isEmpty() const { return startSample >= endSample; }
    juce::int64 lengthSamples() const { return endSample - startSample; }

    void normalize() {
        if (startSample > endSample)
            std::swap(startSample, endSample);
    }
};

} // namespace freedaw
