#include "DelayEffectWidget.h"
#include "utils/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace {
    const juce::Identifier syncModeId("delaySyncMode");
    const juce::Identifier syncDivisionId("delaySyncDivision");
}

namespace OpenDaw {

// Musical divisions sorted longest to shortest.
// Beats are expressed as quarter-note durations so the conversion to
// milliseconds is simply: ms = beats * (60000 / BPM).
const std::vector<DelayEffectWidget::Division>& DelayEffectWidget::divisions()
{
    static const std::vector<Division> d = {
        {"1/1",   4.0},
        {"1/2D",  3.0},
        {"1/1T",  8.0 / 3.0},
        {"1/2",   2.0},
        {"1/4D",  1.5},
        {"1/2T",  4.0 / 3.0},
        {"1/4",   1.0},
        {"1/8D",  0.75},
        {"1/4T",  2.0 / 3.0},
        {"1/8",   0.5},
        {"1/16D", 0.375},
        {"1/8T",  1.0 / 3.0},
        {"1/16",  0.25},
        {"1/16T", 1.0 / 6.0},
        {"1/32",  0.125},
    };
    return d;
}

DelayEffectWidget::DelayEffectWidget(te::DelayPlugin* plugin,
                                     EditManager* editMgr,
                                     QWidget* parent)
    : QWidget(parent), plugin_(plugin), editMgr_(editMgr)
{
    auto& theme = ThemeManager::instance().current();

    setAutoFillBackground(true);
    QPalette pal;
    pal.setColor(QPalette::Window, theme.surfaceLight);
    setPalette(pal);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // ── Header row ──────────────────────────────────────────────────────────
    auto* headerRow = new QHBoxLayout();

    auto* nameLabel = new QLabel("Delay", this);
    nameLabel->setStyleSheet(
        QString("font-weight: bold; font-size: 11px; color: %1;")
            .arg(theme.text.name()));
    headerRow->addWidget(nameLabel, 1);

    auto* bypassBtn = new QPushButton("Byp", this);
    bypassBtn->setAccessibleName("Bypass Delay");
    bypassBtn->setCheckable(true);
    bypassBtn->setFixedSize(32, 20);
    bypassBtn->setChecked(!plugin_->isEnabled());
    bypassBtn->setStyleSheet(
        QString("QPushButton { font-size: 9px; background: %1; color: %2; "
                "border: 1px solid %3; border-radius: 2px; }"
                "QPushButton:checked { background: %4; color: #000; }")
            .arg(theme.surface.name(), theme.textDim.name(),
                 theme.border.name(), theme.muteButton.name()));
    connect(bypassBtn, &QPushButton::toggled, this, [this](bool bypassed) {
        plugin_->setEnabled(!bypassed);
    });
    headerRow->addWidget(bypassBtn);

    auto* removeBtn = new QPushButton("X", this);
    removeBtn->setAccessibleName("Remove Delay");
    removeBtn->setFixedSize(20, 20);
    removeBtn->setStyleSheet(
        QString("QPushButton { font-size: 9px; background: %1; color: %2; "
                "border: 1px solid %3; border-radius: 2px; }"
                "QPushButton:hover { background: %4; }")
            .arg(theme.surface.name(), theme.textDim.name(),
                 theme.border.name(), theme.recordArm.name()));
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
        emit removeRequested(plugin_);
    });
    headerRow->addWidget(removeBtn);

    mainLayout->addLayout(headerRow);

    // ── Time mode toggle (Free / Sync) ─────────────────────────────────────
    auto* modeRow = new QHBoxLayout();
    modeRow->setSpacing(0);

    QString modeStyle = QString(
        "QPushButton { font-size: 10px; background: %1; color: %2; "
        "border: 1px solid %3; padding: 2px 10px; }"
        "QPushButton:checked { background: %4; color: %5; }")
        .arg(theme.surface.name(), theme.textDim.name(),
             theme.border.name(), theme.accent.name(), theme.text.name());

    freeBtn_ = new QPushButton("Free", this);
    freeBtn_->setAccessibleName("Free delay time mode");
    freeBtn_->setCheckable(true);
    freeBtn_->setChecked(true);
    freeBtn_->setFixedHeight(22);
    freeBtn_->setStyleSheet(modeStyle);

    syncBtn_ = new QPushButton("Sync", this);
    syncBtn_->setAccessibleName("Tempo-synced delay time mode");
    syncBtn_->setCheckable(true);
    syncBtn_->setFixedHeight(22);
    syncBtn_->setStyleSheet(modeStyle);

    connect(freeBtn_, &QPushButton::clicked, this, [this]() { setTimeMode(false); });
    connect(syncBtn_, &QPushButton::clicked, this, [this]() { setTimeMode(true); });

    modeRow->addWidget(freeBtn_);
    modeRow->addWidget(syncBtn_);
    modeRow->addStretch();

    mainLayout->addLayout(modeRow);

    // ── Time control stack ──────────────────────────────────────────────────
    timeStack_ = new QStackedWidget(this);

    // Page 0: Free mode — millisecond spinbox
    auto* freeWidget = new QWidget();
    auto* freeLayout = new QHBoxLayout(freeWidget);
    freeLayout->setContentsMargins(0, 0, 0, 0);

    msSpin_ = new QSpinBox(freeWidget);
    msSpin_->setAccessibleName("Delay time in milliseconds");
    msSpin_->setRange(1, 5000);
    msSpin_->setValue(static_cast<int>(plugin_->lengthMs));
    msSpin_->setSuffix(" ms");
    msSpin_->setStyleSheet(
        QString("QSpinBox { background: %1; color: %2; border: 1px solid %3; "
                "border-radius: 2px; padding: 2px 4px; font-size: 11px; }"
                "QSpinBox::up-button, QSpinBox::down-button { width: 14px; }")
            .arg(theme.surface.name(), theme.text.name(), theme.border.name()));
    connect(msSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int val) {
        if (!syncMode_)
            plugin_->lengthMs = val;
    });

    freeLayout->addWidget(msSpin_, 1);
    timeStack_->addWidget(freeWidget);

    // Page 1: Sync mode — musical division combo + calculated ms readout
    auto* syncWidget = new QWidget();
    auto* syncLayout = new QHBoxLayout(syncWidget);
    syncLayout->setContentsMargins(0, 0, 0, 0);

    divCombo_ = new QComboBox(syncWidget);
    divCombo_->setAccessibleName("Musical note division for delay time");
    for (const auto& div : divisions())
        divCombo_->addItem(div.name);
    divCombo_->setCurrentIndex(6); // 1/4
    divCombo_->setStyleSheet(
        QString("QComboBox { background: %1; color: %2; border: 1px solid %3; "
                "border-radius: 2px; padding: 2px 4px; font-size: 11px; min-width: 60px; }"
                "QComboBox::drop-down { border: none; width: 16px; }"
                "QComboBox QAbstractItemView { background: %1; color: %2; "
                "border: 1px solid %3; selection-background-color: %4; }")
            .arg(theme.surface.name(), theme.text.name(),
                 theme.border.name(), theme.accent.name()));
    connect(divCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        plugin_->state.setProperty(syncDivisionId, idx, nullptr);
        if (syncMode_) {
            applySync();
            updateSyncDisplay();
        }
    });

    syncMsLabel_ = new QLabel(syncWidget);
    syncMsLabel_->setStyleSheet(
        QString("font-size: 10px; color: %1; padding-left: 4px;")
            .arg(theme.textDim.name()));

    syncLayout->addWidget(divCombo_);
    syncLayout->addWidget(syncMsLabel_, 1);
    timeStack_->addWidget(syncWidget);

    timeStack_->setCurrentIndex(0);
    mainLayout->addWidget(timeStack_);

    // ── Parameter knobs (Decay + Mix) ───────────────────────────────────────
    auto* knobsRow = new QHBoxLayout();
    knobsRow->setSpacing(8);

    auto* feedbackKnob = new RotaryKnob(this);
    feedbackKnob->setAccessibleName("Delay decay amount");
    feedbackKnob->setRange(0.0, 1.0);
    feedbackKnob->setValue(
        static_cast<double>(plugin_->feedbackDb->getCurrentNormalisedValue()));
    feedbackKnob->setLabel("Decay");
    feedbackKnob->setFixedSize(44, 52);
    connect(feedbackKnob, &RotaryKnob::valueChanged, this, [this](double v) {
        plugin_->feedbackDb->setNormalisedParameter(
            static_cast<float>(v), juce::sendNotification);
    });
    knobsRow->addWidget(feedbackKnob);

    auto* mixKnob = new RotaryKnob(this);
    mixKnob->setAccessibleName("Delay wet/dry mix level");
    mixKnob->setRange(0.0, 1.0);
    mixKnob->setValue(
        static_cast<double>(plugin_->mixProportion->getCurrentNormalisedValue()));
    mixKnob->setLabel("Mix");
    mixKnob->setFixedSize(44, 52);
    connect(mixKnob, &RotaryKnob::valueChanged, this, [this](double v) {
        plugin_->mixProportion->setNormalisedParameter(
            static_cast<float>(v), juce::sendNotification);
    });
    knobsRow->addWidget(mixKnob);

    knobsRow->addStretch();
    mainLayout->addLayout(knobsRow);

    // Restore persisted time mode from plugin state
    int savedDiv = plugin_->state.getProperty(syncDivisionId, 6);
    if (savedDiv >= 0 && savedDiv < static_cast<int>(divisions().size()))
        divCombo_->setCurrentIndex(savedDiv);
    if (static_cast<bool>(plugin_->state.getProperty(syncModeId, false)))
        setTimeMode(true);

    // Recalculate sync delay when BPM changes
    connect(editMgr_, &EditManager::editChanged, this, [this]() {
        if (syncMode_) {
            applySync();
            updateSyncDisplay();
        }
    });
}

void DelayEffectWidget::setTimeMode(bool sync)
{
    syncMode_ = sync;
    plugin_->state.setProperty(syncModeId, sync, nullptr);
    freeBtn_->setChecked(!sync);
    syncBtn_->setChecked(sync);
    timeStack_->setCurrentIndex(sync ? 1 : 0);

    if (sync) {
        applySync();
        updateSyncDisplay();
    } else {
        // Reflect the current plugin value in the spinbox
        msSpin_->setValue(static_cast<int>(plugin_->lengthMs));
    }
}

void DelayEffectWidget::applySync()
{
    const auto& divs = divisions();
    int idx = divCombo_->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(divs.size()))
        return;

    double bpm = editMgr_->getBpm();
    double quarterMs = 60000.0 / bpm;
    int ms = std::max(1, static_cast<int>(std::round(divs[idx].beats * quarterMs)));
    plugin_->lengthMs = ms;
}

void DelayEffectWidget::updateSyncDisplay()
{
    const auto& divs = divisions();
    int idx = divCombo_->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(divs.size()))
        return;

    double bpm = editMgr_->getBpm();
    double quarterMs = 60000.0 / bpm;
    int ms = std::max(1, static_cast<int>(std::round(divs[idx].beats * quarterMs)));
    syncMsLabel_->setText(QString("= %1 ms").arg(ms));
}

} // namespace OpenDaw
