#include "TransportBar.h"
#include "ui/timeline/GridSnapper.h"
#include "utils/ThemeManager.h"
#include "utils/IconFont.h"
#include <QDialog>
#include <QVBoxLayout>
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

    const QFont iconFont = icons::fontAudio(16);
    const QSize transportButtonSize(34, 32);

    auto makeTransportBtn = [&](const QChar& glyph, const QString& name,
                                const QColor& activeColor, bool checkable = false) {
        auto* btn = new QPushButton(glyph, this);
        btn->setAccessibleName(name);
        btn->setToolTip(name);
        btn->setFont(iconFont);
        btn->setCheckable(checkable);
        btn->setFixedSize(transportButtonSize);
        applyButtonStyle(btn, activeColor);
        return btn;
    };

    stopBtn_   = makeTransportBtn(icons::fa::Stop,   "Stop",   theme.transportStop);
    playBtn_   = makeTransportBtn(icons::fa::Play,   "Play",   theme.transportPlay, true);
    recordBtn_ = makeTransportBtn(icons::fa::Record, "Record", theme.transportRecord, true);
    loopBtn_   = makeTransportBtn(icons::fa::Loop,   "Loop",   theme.accent, true);

    const QFont miFont = icons::materialIcons(16);
    panicBtn_ = new QPushButton(this);
    panicBtn_->setAccessibleName("MIDI Panic - Stop All Notes");
    panicBtn_->setToolTip("MIDI Panic — stop all notes & reset (Ctrl+Shift+P)");
    panicBtn_->setFont(miFont);
    panicBtn_->setText(QString(icons::mi::Warning));
    panicBtn_->setFixedSize(transportButtonSize);
    panicBtn_->setStyleSheet(
        QString("QPushButton { background: %1; color: %2; border: 1px solid %3; "
                "border-radius: 4px; font-weight: bold; }"
                "QPushButton:hover { background: %4; color: #ff3333; }"
                "QPushButton:pressed { background: #cc2222; color: white; }")
            .arg(theme.surface.name(), QColor(220, 80, 60).name(),
                 theme.border.name(), theme.surfaceLight.name()));

    engineBtn_ = new QPushButton(this);
    engineBtn_->setAccessibleName("Audio Engine Toggle");
    engineBtn_->setToolTip("Audio Engine — click to disable/enable");
    engineBtn_->setFont(iconFont);
    engineBtn_->setText(QString(icons::fa::Powerswitch));
    engineBtn_->setCheckable(true);
    engineBtn_->setChecked(true);
    engineBtn_->setFixedSize(transportButtonSize);
    updateEngineButtonStyle();

    connect(stopBtn_,   &QPushButton::clicked, this, &TransportBar::onStop);
    connect(playBtn_,   &QPushButton::clicked, this, &TransportBar::onPlay);
    connect(recordBtn_, &QPushButton::clicked, this, &TransportBar::onRecord);
    connect(loopBtn_,   &QPushButton::clicked, this, &TransportBar::onLoop);
    connect(panicBtn_,  &QPushButton::clicked, this, &TransportBar::onPanic);
    connect(engineBtn_, &QPushButton::clicked, this, &TransportBar::onEngineToggle);

    // Position display
    positionLabel_ = new QLabel("00:00.000", this);
    positionLabel_->setAccessibleName("Position Time");
    positionLabel_->setFixedWidth(90);
    positionLabel_->setAlignment(Qt::AlignCenter);
    positionLabel_->setStyleSheet(
        QString("QLabel { color: %1; background: %2; border: 1px solid %3; "
                "border-radius: 3px; font-family: monospace; font-size: 10pt; padding: 2px; }")
            .arg(theme.accentLight.name(), theme.background.name(), theme.border.name()));

    beatLabel_ = new QLabel("1.1.000", this);
    beatLabel_->setAccessibleName("Position Bars Beats");
    beatLabel_->setFixedWidth(80);
    beatLabel_->setAlignment(Qt::AlignCenter);
    beatLabel_->setStyleSheet(positionLabel_->styleSheet());

    layout->addWidget(positionLabel_);
    layout->addWidget(beatLabel_);

    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet(QString("color: %1;").arg(theme.border.name()));
    layout->addWidget(sep1);

    layout->addWidget(stopBtn_);
    layout->addWidget(playBtn_);
    layout->addWidget(recordBtn_);
    layout->addWidget(loopBtn_);

    auto* sepPanic = new QFrame(this);
    sepPanic->setFrameShape(QFrame::VLine);
    sepPanic->setStyleSheet(QString("color: %1;").arg(theme.border.name()));
    layout->addWidget(sepPanic);
    layout->addWidget(panicBtn_);
    layout->addWidget(engineBtn_);

    auto* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setStyleSheet(sep1->styleSheet());
    layout->addWidget(sep2);

    // BPM
    auto* bpmLabel = new QLabel("BPM", this);
    bpmLabel->setStyleSheet(
        QString("QLabel { color: %1; background: transparent; border: none; }")
            .arg(theme.text.name()));
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
    auto* tsLabel = new QLabel("Time", this);
    tsLabel->setStyleSheet(
        QString("QLabel { color: %1; background: transparent; border: none; }")
            .arg(theme.text.name()));
    timeSigNumSpin_ = new QSpinBox(this);
    timeSigNumSpin_->setAccessibleName("Time Signature Numerator");
    timeSigNumSpin_->setRange(1, 16);
    timeSigNumSpin_->setValue(editMgr_->getTimeSigNumerator());
    timeSigNumSpin_->setFixedWidth(50);
    timeSigNumSpin_->setFixedHeight(28);
    connect(timeSigNumSpin_, qOverload<int>(&QSpinBox::valueChanged),
            this, &TransportBar::onTimeSigNumChanged);

    auto* slash = new QLabel("/", this);
    slash->setStyleSheet(
        QString("QLabel { color: %1; background: transparent; border: none; }")
            .arg(theme.text.name()));

    timeSigDenCombo_ = new QComboBox(this);
    timeSigDenCombo_->setAccessibleName("Time Signature Denominator");
    for (int d : {1, 2, 4, 8, 16})
        timeSigDenCombo_->addItem(QString::number(d), d);
    int denIdx = timeSigDenCombo_->findData(editMgr_->getTimeSigDenominator());
    if (denIdx >= 0) timeSigDenCombo_->setCurrentIndex(denIdx);
    timeSigDenCombo_->setFixedWidth(50);
    timeSigDenCombo_->setFixedHeight(28);
    connect(timeSigDenCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                if (!editMgr_ || !editMgr_->edit()) return;
                int den = timeSigDenCombo_->itemData(idx).toInt();
                editMgr_->setTimeSignature(timeSigNumSpin_->value(), den);
            });

    layout->addWidget(tsLabel);
    layout->addWidget(timeSigNumSpin_);
    layout->addWidget(slash);
    layout->addWidget(timeSigDenCombo_);

    // Snap mode
    auto* snapLabel = new QLabel("Snap", this);
    snapLabel->setStyleSheet(
        QString("QLabel { color: %1; background: transparent; border: none; }")
            .arg(theme.text.name()));
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

    connect(&positionTimer_, &QTimer::timeout, this, &TransportBar::updatePosition);
    positionTimer_.start(50);
}

void TransportBar::onPlay()
{
    if (!editMgr_ || !editMgr_->edit()) return;
    auto& t = editMgr_->transport();
    if (t.isPlaying()) {
        t.stop(false, false);
        if (isRecording_) {
            isRecording_ = false;
            recordBtn_->setChecked(false);
        }
    } else {
        t.play(false);
    }
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

void TransportBar::onPanic()
{
    if (!editMgr_) return;
    editMgr_->midiPanic();
    playBtn_->setChecked(false);
    recordBtn_->setChecked(false);
    isRecording_ = false;
}

void TransportBar::onEngineToggle()
{
    if (!editMgr_) return;

    bool turningOn = engineBtn_->isChecked();

    if (turningOn) {
        auto& theme = ThemeManager::instance().current();
        auto* busyDlg = new QDialog(window());
        busyDlg->setWindowTitle("Audio Engine");
        busyDlg->setAccessibleName("Audio Engine Initializing");
        busyDlg->setModal(true);
        busyDlg->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        busyDlg->setFixedSize(320, 100);

        auto* label = new QLabel("Initializing audio engine...", busyDlg);
        label->setAccessibleName("Engine Status");
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(
            QString("QLabel { color: %1; font-size: 12px; font-weight: bold; }")
                .arg(theme.text.name()));
        auto* dlgLayout = new QVBoxLayout(busyDlg);
        dlgLayout->addWidget(label);

        QTimer::singleShot(50, busyDlg, [this, busyDlg]() {
            editMgr_->resumeEngine();
            busyDlg->accept();
        });

        busyDlg->exec();
        delete busyDlg;
    } else {
        editMgr_->suspendEngine();
    }

    updateEngineButtonStyle();
    playBtn_->setChecked(false);
    recordBtn_->setChecked(false);
    isRecording_ = false;
}

void TransportBar::updateEngineButtonStyle()
{
    auto& theme = ThemeManager::instance().current();
    bool active = engineBtn_->isChecked();

    if (active) {
        engineBtn_->setStyleSheet(
            QString("QPushButton { background: %1; color: %2; border: 1px solid %3; "
                    "border-radius: 4px; font-weight: bold; }"
                    "QPushButton:hover { background: %4; }")
                .arg(QColor(25, 120, 50).name(), QColor(70, 230, 70).name(),
                     QColor(35, 150, 60).name(), QColor(30, 140, 55).name()));
    } else {
        engineBtn_->setStyleSheet(
            QString("QPushButton { background: %1; color: %2; border: 1px solid %3; "
                    "border-radius: 4px; font-weight: bold; }"
                    "QPushButton:hover { background: %4; }")
                .arg(theme.surface.name(), theme.textDim.name(),
                     theme.border.name(), theme.surfaceLight.name()));
    }
}

void TransportBar::onBpmChanged(double bpm)
{
    if (!editMgr_ || !editMgr_->edit()) return;
    auto& ts = editMgr_->edit()->tempoSequence;
    auto& transport = editMgr_->transport();
    double beatBefore = ts.toBeats(transport.getPosition()).inBeats();
    editMgr_->setBpm(bpm);
    auto newTime = ts.toTime(tracktion::BeatPosition::fromBeats(beatBefore));
    transport.setPosition(newTime);
}

void TransportBar::onTimeSigNumChanged(int num)
{
    if (!editMgr_ || !editMgr_->edit()) return;
    int den = timeSigDenCombo_->currentData().toInt();
    editMgr_->setTimeSignature(num, den);
}

void TransportBar::updatePosition()
{
    if (!editMgr_ || !editMgr_->edit()) return;

    if (editMgr_->isEngineSuspended()) {
        positionLabel_->setText("-- OFF --");
        beatLabel_->setText("---");
        return;
    }

    auto pos = editMgr_->transport().getPosition();
    double secs = std::max(0.0, pos.inSeconds());

    int mins = int(secs) / 60;
    double secsRem = secs - mins * 60;
    positionLabel_->setText(
        QString("%1:%2")
            .arg(mins, 2, 10, QChar('0'))
            .arg(secsRem, 6, 'f', 3, QChar('0')));

    auto& ts2 = editMgr_->edit()->tempoSequence;
    double beat = std::max(0.0, ts2.toBeats(pos).inBeats());
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
                "border-radius: 4px; font-weight: bold; font-size: 8pt; }"
                "QPushButton:hover { background: %4; }"
                "QPushButton:checked { background: %5; color: white; }"
                "QPushButton:pressed { background: %5; }")
            .arg(theme.surface.name(), theme.text.name(), theme.border.name(),
                 theme.surfaceLight.name(), activeColor.name()));
}

} // namespace freedaw
