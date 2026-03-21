#pragma once

#include <QWidget>
#include <QTimer>

namespace OpenDaw {

class LevelMeter : public QWidget {
    Q_OBJECT

public:
    explicit LevelMeter(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {12, 120}; }
    QSize minimumSizeHint() const override { return {6, 40}; }

    void setLevel(float levelL, float levelR);
    void setStereo(bool stereo) { stereo_ = stereo; update(); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float peakDecay(float current, float peak) const;

    float levelL_    = 0.0f;
    float levelR_    = 0.0f;
    float peakL_     = 0.0f;
    float peakR_     = 0.0f;
    bool  stereo_    = false;
    QTimer decayTimer_;
};

} // namespace OpenDaw
