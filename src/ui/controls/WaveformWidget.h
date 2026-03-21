#pragma once

#include <QWidget>
#include <QColor>
#include <vector>

namespace OpenDaw {

class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget* parent = nullptr);

    void setWaveformData(const std::vector<float>& minValues,
                         const std::vector<float>& maxValues);
    void setColor(const QColor& c) { color_ = c; update(); }
    void setSelected(bool s) { selected_ = s; update(); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<float> minValues_;
    std::vector<float> maxValues_;
    QColor color_;
    bool selected_ = false;
};

} // namespace OpenDaw
