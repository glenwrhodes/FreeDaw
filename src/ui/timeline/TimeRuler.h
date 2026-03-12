#pragma once

#include <QWidget>

namespace freedaw {

class TimeRuler : public QWidget {
    Q_OBJECT

public:
    explicit TimeRuler(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {800, 28}; }

    void setPixelsPerBeat(double ppb) { pixelsPerBeat_ = ppb; update(); }
    void setScrollX(double sx) { scrollX_ = sx; update(); }
    void setBpm(double bpm) { bpm_ = bpm; update(); }
    void setTimeSig(int num, int den) { timeSigNum_ = num; timeSigDen_ = den; update(); }
    void setPlayheadBeat(double beat) { playheadBeat_ = beat; update(); }

signals:
    void positionClicked(double beat);
    void positionDragged(double beat);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    double xToBeat(int x) const;

    double pixelsPerBeat_ = 40.0;
    double scrollX_       = 0.0;
    double bpm_           = 120.0;
    int    timeSigNum_    = 4;
    int    timeSigDen_    = 4;
    double playheadBeat_  = 0.0;
    bool   dragging_      = false;
};

} // namespace freedaw
