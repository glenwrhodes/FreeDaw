#pragma once

#include <QWidget>

namespace freedaw {

class RotaryKnob : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)

public:
    explicit RotaryKnob(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {48, 48}; }
    QSize minimumSizeHint() const override { return {32, 32}; }

    double value()   const { return value_; }
    double minimum() const { return min_; }
    double maximum() const { return max_; }

    void setValue(double v);
    void setMinimum(double v) { min_ = v; update(); }
    void setMaximum(double v) { max_ = v; update(); }
    void setRange(double lo, double hi) { min_ = lo; max_ = hi; update(); }
    void setLabel(const QString& l) { label_ = l; update(); }

signals:
    void valueChanged(double value);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    double normalised() const;

    double value_ = 0.0;
    double min_   = -1.0;
    double max_   = 1.0;
    QString label_;
    bool   dragging_ = false;
    int    dragStartY_ = 0;
    double dragStartVal_ = 0.0;
};

} // namespace freedaw
