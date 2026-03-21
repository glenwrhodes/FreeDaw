#pragma once

#include <QWidget>
#include <functional>

namespace OpenDaw {

class PianoRollRuler : public QWidget {
    Q_OBJECT

public:
    explicit PianoRollRuler(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {800, 24}; }

    void setPixelsPerBeat(double ppb) { pixelsPerBeat_ = ppb; update(); }
    void setScrollX(double sx) { scrollX_ = sx; update(); }
    void setCursorBeat(double beat) { cursorBeat_ = beat; update(); }
    void setSnapFunction(std::function<double(double)> fn) { snapFn_ = std::move(fn); }

signals:
    void cursorPositionClicked(double beat);
    void cursorPositionDragged(double beat);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    double xToBeat(int x) const;

    double pixelsPerBeat_ = 60.0;
    double scrollX_       = 0.0;
    double cursorBeat_    = 0.0;
    int    timeSigNum_    = 4;
    bool   dragging_      = false;
    std::function<double(double)> snapFn_;
};

} // namespace OpenDaw
