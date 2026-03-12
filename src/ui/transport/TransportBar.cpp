#include "TransportBar.h"
#include "ui/timeline/GridSnapper.h"
#include "utils/ThemeManager.h"
#include <QStyle>
#include <cmath>

namespace freedaw {

TransportBar::TransportBar(EditManager* editMgr, QWidget* parent)
    : QWidget(parent), editMgr_(editMgr)
{
    setAccessibleName("Transport Bar");
    setFixedHeight(52);

    auto& theme = ThemeManager::instance().current();
    setAutoFillBackground(true);
    QPalette pal;
    pal.setColor(QPalette::Window, theme.surface);
    setPalette(pal);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    // Transport buttons
    stopBtn_ = new QPushButton("Stop", this);
    stopBtn_->setAccessibleName("Stop");
    stopBtn_->setFixedSize(56, 32);
    applyButtonStyle(stopBtn_, theme.transportStop);
    connect(stopBtn_, &QPushButton::clicked, this, &TransportBar::onStop);

    playBtn_ = new QPushButton("Play", this);
    playBtn_->setAccessibleName("Play");
    playBtn_->setCheckable(true);
    playBtn_->setFixedSize(56, 32);
    applyButtonStyle(playBtn_, theme.transportPlay);
    connect(playBtn_, &QPushButton::clicked, this, &TransportBar::onPlay);

    recordBtn_ = new QPushButton("Rec", this);
    recordBtn_->setAccessibleName("Record");
    recordBtn_->setCheckable(true);
    recordBtn_->setFixedSize(48, 32);
    applyButtonStyle(recordBtn_, theme.transportRecord);
    connect(recordBtn_, &QPushButton::clicked, this, &TransportBar::onRecord);

    loopBtn_ = new QPushButton("Loop", this);
    loopBtn_->setAccessibleName("Loop Toggle");
    loopBtn_->setCheckable(true);
    loopBtn_->setFixedSize(48, 32);
    applyButtonStyle(loopBtn_, theme.accent);
    connect(loopBtn_, &QPushButton::clicked, this, &TransportBar::onLoop);

    layout->addWidget(stopBtn_);
    layout->addWidget(playBtn_);
    layout->addWidget(recordBtn_);
    layout->addWidget(loopBtn_);

    // Separator
    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet(QString("color: %1;").arg(theme.border.name()));
    layout->addWidget(sep1);

    // Position display
    positionLabel_ = new QLabel("00:00.000", this);
    positionLabel_->setAccessibleName("Position Time");
    positionLabel_->setFixedWidth(90);
    positionLabel_->setAlignment(Qt::AlignCenter);
    positionLabel_->setStyleSheet(
        QString("QLabel { color: %1; background: %2; border: 1px solid %3; "
                "border-radius: 3px; font-family: monospace; font-size: 13px; padding: 2px; }")
            .arg(theme.accentLight.name(), theme.background.name(), theme.border.name()));

    beatLabel_ = new QLabel("1.1.000", this);
    beatLabel_->setAccessibleName("Position Bars Beats");
    beatLabel_->setFixedWidth(80);
    beatLabel_->setAlignment(Qt::AlignCenter);
    beatLabel_->setStyleSheet(positionLabel_->styleSheet());

    layout->addWidget(positionLabel_);
    layout->addWidget(beatLabel_);

    // Separator
    auto* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setStyleSheet(sep1->styleSheet());
    layout->addWidget(sep2);

    // BPM
    auto* bpmLabel = new QLabel("BPM:", this);
    bpmLabel->setStyleSheet(QString("color: %1;").arg(theme.text.name()));
    bpmSpin_ = new QDoubleSpinBox(this);
    bpmSpin_->setAccessibleName("Tempo BPM");
    bpmSpin_->setRange(20.0, 300.0);
    bpmSpin_->setDecimals(1);
    bpmSpin_->setValue(editMgr_->getBpm());
    bpmSpin_->setFixedWidth(80);
    bpmSpin_->setFixedHeight(28);
    connect(bpmSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &TransportBar::onBpmChanged);

    layout->addWidget(bpmLabel);
    layout->addWidget(bpmSpin_);

    // Time Signature
    auto* tsLabel = new QLabel("Time:", this);
    tsLabel->setStyleSheet(QString("color: %1;").arg(theme.text.name()));
    timeSigNumSpin_ = new QSpinBox(this);
    timeSigNumSpin_->setAccessibleName("Time Signature Numerator");
    timeSigNumSpin_->setRange(1, 16);
    timeSigNumSpin_->setValue(editMgr_->getTimeSigNumerator());
    timeSigNumSpin_->setFixedWidth(50);
    timeSigNumSpin_->setFixedHeight(28);
    connect(timeSigNumSpin_, qOverload<int>(&QSpinBox::valueChanged),
            this, &TransportBar::onTimeSigNumChanged);

    auto* slash = new QLabel("/", this);
    slash->setStyleSheet(QString("color: %1;").arg(theme.text.name()));

    timeSigDenSpin_ = new QSpinBox(this);
    timeSigDenSpin_->setAccessibleName("Time Signature Denominator");
    timeSigDenSpin_->setRange(1, 16);
    timeSigDenSpin_->setValue(editMgr_->getTimeSigDenominator());
    timeSigDenSpin_->setFixedWidth(50);
    timeSigDenSpin_->setFixedHeight(28);
    connect(timeSigDenSpin_, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int den) { editMgr_->setTimeSignature(timeSigNumSpin_->value(), den); });

    layout->addWidget(tsLabel);
    layout->addWidget(timeSigNumSpin_);
    layout->addWidget(slash);
    layout->addWidget(timeSigDenSpin_);

    // Separator
    auto* sep3 = new QFrame(this);
    sep3->setFrameShape(QFrame::VLine);
    sep3->setStyleSheet(sep1->styleSheet());
    layout->addWidget(sep3);

    // Snap mode
    auto* snapLabel = new QLabel("Snap:", this);
    snapLabel->setStyleSheet(QString("color: %1;").arg(theme.text.name()));
    snapCombo_ = new QComboBox(this);
    snapCombo_->setAccessibleName("Grid Snap Mode");
    snapCombo_->addItem("Off",     int(SnapMode::Off));
    snapCombo_->addItem("1/4 Beat", int(SnapMode::QuarterBeat));
    snapCombo_->addItem("1/2 Beat", int(SnapMode::HalfBeat));
    snapCombo_->addItem("Beat",    int(SnapMode::Beat));
    snapCombo_->addItem("Bar",     int(SnapMode::Bar));
    snapCombo_->setCurrentIndex(3);
    snapCombo_->setFixedWidth(100);
    snapCombo_->setFixedHeight(28);
    connect(snapCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                emit snapModeRequested(snapCombo_->itemData(idx).toInt());
            });

    layout->addWidget(snapLabel);
    layout->addWidget(snapCombo_);

    layout->addStretch();

    // Position update timer
    connect(&positionTimer_, &QTimer::timeout, this, &TransportBar::updatePosition);
    positionTimer_.start(50);
}

void TransportBar::onPlay()
{
    if (!editMgr_ || !editMgr_->edit()) return;
    auto& t = editMgr_->transport();
    if (t.isPlaying())
        t.stop(false, false);
    else
        t.play(false);
    playBtn_->setChecked(t.isPlaying());
}

void TransportBar::onStop()
{
    if (!editMgr_ || !editMgr_->edit()) return;
    auto& t = editMgr_->transport();
    t.stop(false, false);
    t.setPosition(tracktion::TimePosition::fromSeconds(0));
    playBtn_->setChecked(false);
    recordBtn_->setChecked(false);
    isRecording_ = false;
}

void TransportBar::onRecord()
{
    if (!editMgr_ || !editMgr_->edit()) return;
    auto& t = editMgr_->transport();
    if (isRecording_) {
        t.stop(false, false);
        isRecording_ = false;
    } else {
        t.record(false);
        isRecording_ = true;
    }
    recordBtn_->setChecked(isRecording_);
    playBtn_->setChecked(t.isPlaying());
}

void TransportBar::onLoop()
{
    if (!editMgr_ || !editMgr_->edit()) return;
    auto& t = editMgr_->transport();
    t.looping.setValue(!t.looping.get(), nullptr);
    loopBtn_->setChecked(t.looping.get());
}

void TransportBar::onBpmChanged(double bpm)
{
    editMgr_->setBpm(bpm);
}

void TransportBar::onTimeSigNumChanged(int num)
{
    editMgr_->setTimeSignature(num, timeSigDenSpin_->value());
}

void TransportBar::updatePosition()
{
    if (!editMgr_ || !editMgr_->edit()) return;

    auto pos = editMgr_->transport().getPosition();
    double secs = pos.inSeconds();

    int mins = int(secs) / 60;
    double secsRem = secs - mins * 60;
    positionLabel_->setText(
        QString("%1:%2")
            .arg(mins, 2, 10, QChar('0'))
            .arg(secsRem, 6, 'f', 3, QChar('0')));

    auto& ts = editMgr_->edit()->tempoSequence;
    double beat = ts.toBeats(pos).inBeats();
    int beatsPerBar = editMgr_->getTimeSigNumerator();
    int bar = int(beat / beatsPerBar) + 1;
    int beatInBar = int(std::fmod(beat, beatsPerBar)) + 1;
    int ticks = int(std::fmod(beat, 1.0) * 1000);

    beatLabel_->setText(
        QString("%1.%2.%3")
            .arg(bar).arg(beatInBar).arg(ticks, 3, 10, QChar('0')));

    playBtn_->setChecked(editMgr_->transport().isPlaying());
}

void TransportBar::applyButtonStyle(QPushButton* btn, const QColor& activeColor)
{
    auto& theme = ThemeManager::instance().current();
    btn->setStyleSheet(
        QString("QPushButton { background: %1; color: %2; border: 1px solid %3; "
                "border-radius: 4px; font-weight: bold; font-size: 11px; }"
                "QPushButton:hover { background: %4; }"
                "QPushButton:checked { background: %5; color: white; }"
                "QPushButton:pressed { background: %5; }")
            .arg(theme.surface.name(), theme.text.name(), theme.border.name(),
                 theme.surfaceLight.name(), activeColor.name()));
}

} // namespace freedaw
