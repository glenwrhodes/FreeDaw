#pragma once

#include "engine/EditManager.h"
#include <QJsonObject>
#include <memory>
#include <unordered_map>

namespace freedaw {

class AiAudioAnalysis {
public:
    explicit AiAudioAnalysis(EditManager* editMgr);
    ~AiAudioAnalysis();

    QJsonObject analyzeTrackLevels(te::AudioTrack* track, double windowSeconds);
    QJsonObject analyzeMasterLevels(double windowSeconds);
    QJsonObject analyzeFrequencyBalance(te::AudioTrack* track);
    QJsonObject analyzeMasterFrequencyBalance();
    QJsonObject analyzeStereoImage(te::AudioTrack* track);
    QJsonObject analyzeMasterStereoImage();
    QJsonObject analyzeTransients(te::AudioTrack* track);
    QJsonObject analyzeMasking(te::AudioTrack* trackA, te::AudioTrack* trackB);

    static QString inferRoleFromName(const QString& trackName);

private:
    struct MeterClientEntry {
        std::unique_ptr<te::LevelMeasurer::Client> client;
        te::LevelMeterPlugin* plugin = nullptr;
    };

    MeterClientEntry* ensureClient(te::AudioTrack* track);
    std::pair<float, float> readTrackLevels(te::AudioTrack* track);
    static double linearToDb(double linear);
    static double clamp01(double v);

    EditManager* editMgr_ = nullptr;
    std::unordered_map<qulonglong, MeterClientEntry> meterClients_;
};

} // namespace freedaw

