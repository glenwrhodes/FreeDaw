#pragma once

#include "AudioWaveformView.h"
#include "AudioFileUndoManager.h"
#include "AudioFileOperations.h"
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QSet>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion::engine;

namespace freedaw {

class EditManager;

class AudioClipEditor : public QWidget {
    Q_OBJECT

public:
    explicit AudioClipEditor(QWidget* parent = nullptr);

    QSize minimumSizeHint() const override { return {200, 100}; }

    void setClip(te::WaveAudioClip* clip, EditManager* editMgr);
    void refresh();
    te::WaveAudioClip* clip() const { return clip_; }

signals:
    void clipModified();

private:
    void onSnapChanged(int index);
    void onGainSliderChanged(int value);
    void onAutoTempoToggled(bool checked);
    void onLoopToggled(bool checked);
    void onReverseToggled(bool checked);
    void onClipBpmChanged(double bpm);
    void onNormalizeClicked();
    void onBakeGainClicked();
    void onPlayFromStart();
    void onPlayPause();
    void onStop();
    void updateTransportButtons();
    void updatePositionDisplay();
    void updateInfoBar();
    void updateGainLabel();
    void updateSelectionInfo();
    void updateUndoButtons();

    bool confirmDestructiveEdit(const juce::File& file);
    void updateWarningBanner();
    void executeEdit(const QString& description,
                     std::function<bool(const juce::File&)> operation);

    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDeleteSel();
    void onSilence();
    void onFadeIn();
    void onFadeOut();
    void onBakeClipFades();
    void onNormalizeSel();
    void onReverseSel();
    void onAdjustVolume();
    void onInsertSilence();
    void onDCRemove();
    void onSwapChannels();
    void onMixToMono();
    void onCrossfade();

    void buildContextMenu(const QPoint& globalPos);

    te::WaveAudioClip* clip_ = nullptr;
    EditManager* editMgr_ = nullptr;
    AudioFileUndoManager* undoManager_ = nullptr;
    juce::AudioBuffer<float> clipboard_;
    QSet<QString> confirmedFiles_;

    // Warning banner
    QWidget* warningBanner_ = nullptr;
    QLabel* warningLabel_ = nullptr;

    // Mini transport
    QPushButton* playFromStartBtn_ = nullptr;
    QPushButton* playPauseBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QLabel* positionLabel_ = nullptr;
    QTimer* positionTimer_ = nullptr;

    // Toolbar row 1
    QLabel* clipNameLabel_ = nullptr;
    QComboBox* snapCombo_ = nullptr;
    QSlider* gainSlider_ = nullptr;
    QLabel* gainLabel_ = nullptr;
    QPushButton* normalizeBtn_ = nullptr;
    QPushButton* bakeGainBtn_ = nullptr;
    QDoubleSpinBox* bpmSpinBox_ = nullptr;
    QPushButton* autoTempoBtn_ = nullptr;
    QPushButton* loopBtn_ = nullptr;
    QPushButton* reverseBtn_ = nullptr;
    QPushButton* stereoSplitBtn_ = nullptr;

    // Toolbar row 2 (edit operations)
    QPushButton* undoBtn_ = nullptr;
    QPushButton* redoBtn_ = nullptr;
    QPushButton* cutBtn_ = nullptr;
    QPushButton* copyBtn_ = nullptr;
    QPushButton* pasteBtn_ = nullptr;
    QPushButton* deleteBtn_ = nullptr;
    QPushButton* silenceBtn_ = nullptr;
    QPushButton* fadeInBtn_ = nullptr;
    QPushButton* fadeOutBtn_ = nullptr;
    QPushButton* bakeClipFadesBtn_ = nullptr;
    QPushButton* normalizeSelBtn_ = nullptr;
    QPushButton* reverseSelBtn_ = nullptr;
    QPushButton* volumeBtn_ = nullptr;
    QPushButton* insertSilBtn_ = nullptr;
    QPushButton* dcRemoveBtn_ = nullptr;
    QPushButton* swapChBtn_ = nullptr;
    QPushButton* monoBtn_ = nullptr;
    QPushButton* crossfadeBtn_ = nullptr;
    QLabel* selectionInfoLabel_ = nullptr;

    // Waveform view
    AudioWaveformView* waveformView_ = nullptr;

    // Info bar
    QLabel* infoBpmLabel_ = nullptr;
    QLabel* infoBeatsLabel_ = nullptr;
    QLabel* infoDurationLabel_ = nullptr;
    QLabel* infoFormatLabel_ = nullptr;
    QLabel* infoFileLabel_ = nullptr;
    QLabel* infoModeLabel_ = nullptr;
};

} // namespace freedaw
