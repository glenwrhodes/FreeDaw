#include "LevelMeter.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <cmath>

namespace OpenDaw {

LevelMeter::LevelMeter(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Level Meter");

    connect(&decayTimer_, &QTimer::timeout, this, [this]() {
        const float decay = 0.92f;
        peakL_ *= decay;
        peakR_ *= decay;
        levelL_ *= 0.85f;
        levelR_ *= 0.85f;
        update();
    });
    decayTimer_.start(30);
}

void LevelMeter::setLevel(float l, float r)
{
    levelL_ = std::clamp(l, 0.0f, 1.0f);
    levelR_ = std::clamp(r, 0.0f, 1.0f);
    if (levelL_ > peakL_) peakL_ = levelL_;
    if (levelR_ > peakR_) peakR_ = levelR_;
    update();
}

void LevelMeter::paintEvent(QPaintEvent*)
{
    auto& theme = ThemeManager::instance().current();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    auto drawBar = [&](QRectF rect, float level, float peak) {
        // Background
        p.fillRect(rect, theme.surface);

        // Level bar (bottom to top)
        double levelH = rect.height() * level;
        QRectF levelRect(rect.left(), rect.bottom() - levelH,
                         rect.width(), levelH);

        // Gradient: green → yellow → red
        QLinearGradient grad(rect.bottomLeft(), rect.topLeft());
        grad.setColorAt(0.0, theme.meterGreen);
        grad.setColorAt(0.6, theme.meterGreen);
        grad.setColorAt(0.8, theme.meterYellow);
        grad.setColorAt(1.0, theme.meterRed);

        p.fillRect(levelRect, grad);

        // Peak indicator
        if (peak > 0.01f) {
            double peakY = rect.bottom() - rect.height() * peak;
            p.setPen(QPen(peak > 0.95f ? theme.meterRed : theme.meterYellow, 1));
            p.drawLine(QPointF(rect.left(), peakY), QPointF(rect.right(), peakY));
        }
    };

    if (stereo_) {
        double halfW = (width() - 1) / 2.0;
        drawBar(QRectF(0, 0, halfW, height()), levelL_, peakL_);
        drawBar(QRectF(halfW + 1, 0, halfW, height()), levelR_, peakR_);
    } else {
        float mono = std::max(levelL_, levelR_);
        float peakMono = std::max(peakL_, peakR_);
        drawBar(QRectF(0, 0, width(), height()), mono, peakMono);
    }
}

} // namespace OpenDaw
