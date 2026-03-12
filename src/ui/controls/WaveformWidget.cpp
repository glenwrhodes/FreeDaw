#include "WaveformWidget.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <QPainterPath>

namespace freedaw {

WaveformWidget::WaveformWidget(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Waveform Display");
    color_ = ThemeManager::instance().current().waveform;
}

void WaveformWidget::setWaveformData(const std::vector<float>& minV,
                                      const std::vector<float>& maxV)
{
    minValues_ = minV;
    maxValues_ = maxV;
    update();
}

void WaveformWidget::paintEvent(QPaintEvent*)
{
    auto& theme = ThemeManager::instance().current();
    QPainter p(this);

    QColor bg = selected_ ? theme.clipBodySelected : theme.clipBody;
    p.fillRect(rect(), bg);

    if (minValues_.empty() || maxValues_.empty())
        return;

    int numSamples = static_cast<int>(minValues_.size());
    double xScale = double(width()) / numSamples;
    double midY   = height() / 2.0;
    double ampScale = height() / 2.0;

    p.setPen(Qt::NoPen);
    p.setBrush(color_);

    QPainterPath path;
    path.moveTo(0, midY);

    for (int i = 0; i < numSamples; ++i) {
        double x = i * xScale;
        double yTop = midY - maxValues_[size_t(i)] * ampScale;
        path.lineTo(x, yTop);
    }
    for (int i = numSamples - 1; i >= 0; --i) {
        double x = i * xScale;
        double yBot = midY - minValues_[size_t(i)] * ampScale;
        path.lineTo(x, yBot);
    }
    path.closeSubpath();
    p.drawPath(path);
}

} // namespace freedaw
