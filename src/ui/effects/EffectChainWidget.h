#pragma once

#include "engine/EditManager.h"
#include "ui/controls/RotaryKnob.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <juce_audio_processors/juce_audio_processors.h>
#include <tracktion_engine/tracktion_engine.h>

namespace freedaw {

class EffectSlotWidget : public QWidget {
    Q_OBJECT

public:
    EffectSlotWidget(te::Plugin* plugin, QWidget* parent = nullptr);

    te::Plugin* plugin() const { return plugin_; }

signals:
    void removeRequested(te::Plugin* plugin);

private:
    void buildControls();

    te::Plugin* plugin_;
    QVBoxLayout* layout_;
    QLabel* nameLabel_;
    QPushButton* bypassBtn_;
    QPushButton* editBtn_ = nullptr;
    QPushButton* removeBtn_;
};

class EffectChainWidget : public QWidget {
    Q_OBJECT

public:
    explicit EffectChainWidget(EditManager* editMgr, QWidget* parent = nullptr);
    void setPluginList(juce::KnownPluginList* list) { pluginList_ = list; }

    void setTrack(te::AudioTrack* track);
    te::AudioTrack* currentTrack() const { return track_; }

public slots:
    void rebuild();

private:
    void addEffectToTrack(const QString& effectName);
    void addVstEffectToTrack(const juce::PluginDescription& desc);
    void removeEffectFromTrack(te::Plugin* plugin);

    juce::KnownPluginList* pluginList_ = nullptr;

    EditManager* editMgr_;
    te::AudioTrack* track_ = nullptr;

    QVBoxLayout* mainLayout_;
    QLabel* trackLabel_;
    QPushButton* addBtn_;
    QScrollArea* scrollArea_;
    QWidget* slotsContainer_;
    QVBoxLayout* slotsLayout_;
};

} // namespace freedaw
