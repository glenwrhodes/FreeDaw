#pragma once

#include <QWidget>

namespace OpenDaw {

class VolumeFader : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit VolumeFader(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {36, 160}; }
    QSize minimumSizeHint() const override { return {28, 80}; }

    double value() const { return value_; }
    void setValue(double v);

    void setRange(double minDb, double maxDb);
    double valueDb() const;

signals:
    void valueChanged(double value);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    double yToValue(int y) const;
    int    valueToY(double v) const;
    QRectF trackRect() const;
    QRectF thumbRect() const;

    double value_   = 0.75;
    double minDb_   = -60.0;
    double maxDb_   = 6.0;
    bool   dragging_ = false;
    int    dragOffsetY_ = 0;
};

} // namespace OpenDaw
