#include "EffectChainWidget.h"
#include "DelayEffectWidget.h"
#include "EffectSelectorDialog.h"
#include "utils/ThemeManager.h"
#include <QHBoxLayout>

namespace freedaw {

// ── EffectSlotWidget ────────────────────────────────────────────────────────

EffectSlotWidget::EffectSlotWidget(te::Plugin* plugin, QWidget* parent)
    : QWidget(parent), plugin_(plugin)
{
    auto& theme = ThemeManager::instance().current();

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(4, 4, 4, 4);
    layout_->setSpacing(2);

    setAutoFillBackground(true);
    QPalette pal;
    pal.setColor(QPalette::Window, theme.surfaceLight);
    setPalette(pal);

    // Header row
    auto* headerRow = new QHBoxLayout();

    nameLabel_ = new QLabel(
        QString::fromStdString(plugin->getName().toStdString()), this);
    nameLabel_->setStyleSheet(
        QString("font-weight: bold; font-size: 11px; color: %1;")
            .arg(theme.text.name()));
    headerRow->addWidget(nameLabel_, 1);

    bypassBtn_ = new QPushButton("Byp", this);
    bypassBtn_->setAccessibleName("Bypass Effect");
    bypassBtn_->setCheckable(true);
    bypassBtn_->setFixedSize(32, 20);
    bypassBtn_->setChecked(!plugin_->isEnabled());
    bypassBtn_->setStyleSheet(
        QString("QPushButton { font-size: 9px; background: %1; color: %2; border: 1px solid %3; border-radius: 2px; }"
                "QPushButton:checked { background: %4; color: #000; }")
            .arg(theme.surface.name(), theme.textDim.name(),
                 theme.border.name(), theme.muteButton.name()));
    connect(bypassBtn_, &QPushButton::toggled, this, [this](bool bypassed) {
        if (plugin_) plugin_->setEnabled(!bypassed);
    });
    headerRow->addWidget(bypassBtn_);

    removeBtn_ = new QPushButton("X", this);
    removeBtn_->setAccessibleName("Remove Effect");
    removeBtn_->setFixedSize(20, 20);
    removeBtn_->setStyleSheet(
        QString("QPushButton { font-size: 9px; background: %1; color: %2; "
                "border: 1px solid %3; border-radius: 2px; }"
                "QPushButton:hover { background: %4; }")
            .arg(theme.surface.name(), theme.textDim.name(),
                 theme.border.name(), theme.recordArm.name()));
    connect(removeBtn_, &QPushButton::clicked, this, [this]() {
        emit removeRequested(plugin_);
    });
    headerRow->addWidget(removeBtn_);

    layout_->addLayout(headerRow);

    buildControls();
}

void EffectSlotWidget::buildControls()
{
    if (!plugin_) return;

    auto params = plugin_->getAutomatableParameters();

    auto* paramsRow = new QHBoxLayout();
    paramsRow->setSpacing(4);

    int maxKnobs = std::min(4, params.size());
    for (int i = 0; i < maxKnobs; ++i) {
        auto* param = params[i];
        auto* knob = new RotaryKnob(this);
        knob->setRange(0.0, 1.0);
        knob->setValue(param->getCurrentValue());
        knob->setLabel(QString::fromStdString(param->getParameterName().toStdString()));
        knob->setFixedSize(44, 52);

        connect(knob, &RotaryKnob::valueChanged, this, [param](double v) {
            param->setParameter(float(v), juce::sendNotification);
        });

        paramsRow->addWidget(knob);
    }
    paramsRow->addStretch();

    layout_->addLayout(paramsRow);
}

// ── EffectChainWidget ───────────────────────────────────────────────────────

EffectChainWidget::EffectChainWidget(EditManager* editMgr, QWidget* parent)
    : QWidget(parent), editMgr_(editMgr)
{
    setAccessibleName("Effect Chain");
    auto& theme = ThemeManager::instance().current();

    setAutoFillBackground(true);
    QPalette pal;
    pal.setColor(QPalette::Window, theme.background);
    setPalette(pal);

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(4, 4, 4, 4);
    mainLayout_->setSpacing(4);

    trackLabel_ = new QLabel("No track selected", this);
    trackLabel_->setStyleSheet(
        QString("font-weight: bold; font-size: 12px; color: %1;")
            .arg(theme.text.name()));
    mainLayout_->addWidget(trackLabel_);

    addBtn_ = new QPushButton("+ Add Effect", this);
    addBtn_->setAccessibleName("Add Effect");
    addBtn_->setFixedHeight(28);
    addBtn_->setStyleSheet(
        QString("QPushButton { background: %1; color: %2; border: 1px solid %3; "
                "border-radius: 3px; font-size: 11px; }"
                "QPushButton:hover { background: %4; }")
            .arg(theme.surface.name(), theme.accent.name(),
                 theme.border.name(), theme.surfaceLight.name()));
    connect(addBtn_, &QPushButton::clicked, this, [this]() {
        if (!track_) return;
        EffectSelectorDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted && !dlg.selectedEffect().isEmpty()) {
            addEffectToTrack(dlg.selectedEffect());
        }
    });
    mainLayout_->addWidget(addBtn_);

    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameStyle(QFrame::NoFrame);
    scrollArea_->setStyleSheet(
        QString("QScrollArea { background: %1; border: none; }")
            .arg(theme.background.name()));

    slotsContainer_ = new QWidget();
    slotsContainer_->setAutoFillBackground(true);
    QPalette slotsPal;
    slotsPal.setColor(QPalette::Window, theme.background);
    slotsContainer_->setPalette(slotsPal);
    slotsLayout_ = new QVBoxLayout(slotsContainer_);
    slotsLayout_->setContentsMargins(0, 0, 0, 0);
    slotsLayout_->setSpacing(2);
    slotsLayout_->addStretch();

    scrollArea_->setWidget(slotsContainer_);
    mainLayout_->addWidget(scrollArea_, 1);
}

void EffectChainWidget::setTrack(te::AudioTrack* track)
{
    track_ = track;
    if (track_) {
        trackLabel_->setText(
            QString("FX: %1").arg(
                QString::fromStdString(track_->getName().toStdString())));
    } else {
        trackLabel_->setText("No track selected");
    }
    rebuild();
}

void EffectChainWidget::rebuild()
{
    QLayoutItem* item;
    while ((item = slotsLayout_->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    if (!track_) {
        slotsLayout_->addStretch();
        return;
    }

    for (auto* plugin : track_->pluginList.getPlugins()) {
        if (dynamic_cast<te::VolumeAndPanPlugin*>(plugin)) continue;
        if (dynamic_cast<te::LevelMeterPlugin*>(plugin)) continue;

        if (auto* delayPlugin = dynamic_cast<te::DelayPlugin*>(plugin)) {
            auto* delayWidget = new DelayEffectWidget(
                delayPlugin, editMgr_, slotsContainer_);
            connect(delayWidget, &DelayEffectWidget::removeRequested,
                    this, &EffectChainWidget::removeEffectFromTrack);
            slotsLayout_->addWidget(delayWidget);
        } else {
            auto* slot = new EffectSlotWidget(plugin, slotsContainer_);
            connect(slot, &EffectSlotWidget::removeRequested,
                    this, &EffectChainWidget::removeEffectFromTrack);
            slotsLayout_->addWidget(slot);
        }
    }
    slotsLayout_->addStretch();
}

void EffectChainWidget::addEffectToTrack(const QString& effectName)
{
    if (!track_ || !editMgr_ || !editMgr_->edit()) return;

    auto& cache = editMgr_->edit()->getPluginCache();
    const char* xmlType = nullptr;

    if (effectName == "Reverb")           xmlType = te::ReverbPlugin::xmlTypeName;
    else if (effectName == "EQ")          xmlType = te::EqualiserPlugin::xmlTypeName;
    else if (effectName == "Compressor")  xmlType = te::CompressorPlugin::xmlTypeName;
    else if (effectName == "Delay")       xmlType = te::DelayPlugin::xmlTypeName;
    else if (effectName == "Chorus")      xmlType = te::ChorusPlugin::xmlTypeName;
    else if (effectName == "Phaser")      xmlType = te::PhaserPlugin::xmlTypeName;
    else if (effectName == "Low Pass Filter") xmlType = te::LowPassPlugin::xmlTypeName;
    else if (effectName == "Pitch Shift") xmlType = te::PitchShiftPlugin::xmlTypeName;

    if (xmlType) {
        auto plugin = cache.createNewPlugin(juce::String(xmlType), {});
        if (plugin)
            track_->pluginList.insertPlugin(plugin, -1, nullptr);
    }

    rebuild();
}

void EffectChainWidget::removeEffectFromTrack(te::Plugin* plugin)
{
    if (!track_ || !plugin) return;
    plugin->deleteFromParent();
    rebuild();
}

} // namespace freedaw
