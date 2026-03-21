#pragma once

#include "WaveformSelection.h"
#include "ui/timeline/GridSnapper.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QTimer>
#include <tracktion_engine/tracktion_engine.h>
#include <vector>
#include <functional>

namespace te = tracktion::engine;

namespace OpenDaw {

class EditManager;

class AudioWaveformView : public QGraphicsView {
    Q_OBJECT

public:
    explicit AudioWaveformView(QWidget* parent = nullptr);

    void setClip(te::WaveAudioClip* clip, EditManager* editMgr);
    void rebuild();

    GridSnapper& snapper() { return snapper_; }
    double pixelsPerBeat() const { return pixelsPerBeat_; }
    void setPixelsPerBeat(double ppb);

    void setGainMultiplier(double linear);
    void setReversed(bool reversed);
    void setSplitStereo(bool split);
    bool splitStereo() const { return splitStereo_; }

    WaveformSelection selection() const { return selection_; }
    void setSelection(const WaveformSelection& sel);
    void clearSelection();
    void selectAll();

    double sampleRate() const { return sampleRate_; }
    juce::int64 totalSamples() const { return totalSamples_; }
    int numChannels() const { return numChannels_; }
    int bitsPerSample() const { return bitsPerSample_; }

    using ContextMenuCallback = std::function<void(const QPoint& globalPos)>;
    void setContextMenuCallback(ContextMenuCallback cb) { contextMenuCb_ = std::move(cb); }

signals:
    void clipModified();
    void selectionChanged();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    enum class DragMode { None, Select, SelectHandleL, SelectHandleR,
                          TrimLeft, TrimRight, FadeIn, FadeOut, LoopStart, LoopEnd };

    void loadWaveformData();
    void redrawWaveformPath();
    void rebuildOverlays();
    void updatePlayhead();
    void updateSceneSize();
    void updateSelectionOverlay();
    double beatToX(double beat) const;
    double xToBeat(double x) const;
    double timeToX(double seconds) const;
    double xToTime(double x) const;
    double sourceTimeToX(double srcSeconds) const;
    double xToSourceTime(double x) const;
    juce::int64 xToSample(double x) const;
    double sampleToX(juce::int64 sample) const;

    void drawChannelWaveform(QPainterPath& path, const std::vector<float>& minV,
                             const std::vector<float>& maxV,
                             double xOff, double xScale, double topY,
                             double height, double amp) const;

    te::WaveAudioClip* clip_ = nullptr;
    EditManager* editMgr_ = nullptr;
    QGraphicsScene* scene_ = nullptr;
    GridSnapper snapper_;
    double pixelsPerBeat_ = 80.0;

    double clipStartBeat_ = 0.0;
    double gainMultiplier_ = 1.0;
    bool reversed_ = false;
    bool splitStereo_ = false;
    double sampleRate_ = 44100.0;
    juce::int64 totalSamples_ = 0;
    int numChannels_ = 0;
    int bitsPerSample_ = 0;

    QGraphicsPathItem* waveformItem_ = nullptr;
    QGraphicsPathItem* waveformItemR_ = nullptr;
    QGraphicsLineItem* playheadLine_ = nullptr;
    QTimer playheadTimer_;

    // Selection
    WaveformSelection selection_;
    QGraphicsRectItem* selectionOverlay_ = nullptr;
    QGraphicsRectItem* selHandleL_ = nullptr;
    QGraphicsRectItem* selHandleR_ = nullptr;

    // Trim handles
    QGraphicsRectItem* trimLeftHandle_ = nullptr;
    QGraphicsRectItem* trimRightHandle_ = nullptr;
    QGraphicsRectItem* trimLeftOverlay_ = nullptr;
    QGraphicsRectItem* trimRightOverlay_ = nullptr;

    // Fade overlays
    QGraphicsPathItem* fadeInOverlay_ = nullptr;
    QGraphicsPathItem* fadeOutOverlay_ = nullptr;
    QGraphicsRectItem* fadeInHandle_ = nullptr;
    QGraphicsRectItem* fadeOutHandle_ = nullptr;

    // Loop markers
    QGraphicsLineItem* loopStartLine_ = nullptr;
    QGraphicsLineItem* loopEndLine_ = nullptr;
    QGraphicsRectItem* loopStartHandle_ = nullptr;
    QGraphicsRectItem* loopEndHandle_ = nullptr;
    QGraphicsRectItem* loopHighlight_ = nullptr;

    // Waveform data (mixed)
    std::vector<float> waveMin_;
    std::vector<float> waveMax_;
    // Per-channel waveform data
    std::vector<std::vector<float>> chanMin_;
    std::vector<std::vector<float>> chanMax_;

    // Drag state
    DragMode dragMode_ = DragMode::None;
    double dragStartValue_ = 0.0;
    double dragStartX_ = 0.0;
    double originalLoopBeats_ = 0.0;

    ContextMenuCallback contextMenuCb_;
};

} // namespace OpenDaw
