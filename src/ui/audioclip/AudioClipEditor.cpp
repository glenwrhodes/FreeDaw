#include "AudioClipEditor.h"
#include "engine/EditManager.h"
#include "utils/ThemeManager.h"
#include "utils/WaveformCache.h"
#include "utils/IconFont.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QShortcut>
#include <QKeySequence>
#include <cmath>

namespace freedaw {

AudioClipEditor::AudioClipEditor(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Audio Clip Editor");
    setFocusPolicy(Qt::StrongFocus);
    auto& theme = ThemeManager::instance().current();

    undoManager_ = new AudioFileUndoManager(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Destructive editing warning banner ───────────────────────────────────
    warningBanner_ = new QWidget(this);
    warningBanner_->setAccessibleName("Destructive Editing Warning");
    warningBanner_->setFixedHeight(26);
    warningBanner_->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #8B4000, stop:1 #A04800);"
        "border-bottom: 1px solid #C05500;");

    auto* bannerLayout = new QHBoxLayout(warningBanner_);
    bannerLayout->setContentsMargins(10, 2, 10, 2);
    bannerLayout->setSpacing(6);

    auto* warningIcon = new QLabel(QString::fromUtf8("\xe2\x9a\xa0"), warningBanner_);
    warningIcon->setAccessibleName("Warning Icon");
    warningIcon->setStyleSheet("color: #FFD54F; font-size: 13px; background: transparent; border: none;");
    bannerLayout->addWidget(warningIcon);

    warningLabel_ = new QLabel(warningBanner_);
    warningLabel_->setAccessibleName("Destructive Edit Warning Text");
    warningLabel_->setStyleSheet(
        "color: #FFE0B2; font-size: 11px; font-weight: bold; background: transparent; border: none;");
    bannerLayout->addWidget(warningLabel_);

    bannerLayout->addStretch();
    warningBanner_->setVisible(false);
    mainLayout->addWidget(warningBanner_);

    // ── Toolbar Row 1 ────────────────────────────────────────────────────────
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(34);
    toolbar->setAutoFillBackground(true);
    QPalette tbPal;
    tbPal.setColor(QPalette::Window, theme.surface);
    toolbar->setPalette(tbPal);

    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(6, 3, 6, 3);
    tbLayout->setSpacing(6);

    clipNameLabel_ = new QLabel("No clip selected", toolbar);
    clipNameLabel_->setAccessibleName("Clip Name");
    clipNameLabel_->setStyleSheet(
        QString("color: %1; font-weight: bold; font-size: 11px;").arg(theme.text.name()));
    tbLayout->addWidget(clipNameLabel_);

    tbLayout->addSpacing(12);

    const auto transportFont = icons::fontAudio(14);
    const int tBtnSize = 24;

    playFromStartBtn_ = new QPushButton(toolbar);
    playFromStartBtn_->setAccessibleName("Play From Clip Start");
    playFromStartBtn_->setFont(transportFont);
    playFromStartBtn_->setText(QString(icons::fa::Prev));
    playFromStartBtn_->setToolTip("Play from clip start");
    playFromStartBtn_->setFixedSize(tBtnSize, tBtnSize);
    connect(playFromStartBtn_, &QPushButton::clicked, this, &AudioClipEditor::onPlayFromStart);
    tbLayout->addWidget(playFromStartBtn_);

    playPauseBtn_ = new QPushButton(toolbar);
    playPauseBtn_->setAccessibleName("Play / Pause");
    playPauseBtn_->setFont(transportFont);
    playPauseBtn_->setText(QString(icons::fa::Play));
    playPauseBtn_->setToolTip("Play / Pause");
    playPauseBtn_->setFixedSize(tBtnSize, tBtnSize);
    connect(playPauseBtn_, &QPushButton::clicked, this, &AudioClipEditor::onPlayPause);
    tbLayout->addWidget(playPauseBtn_);

    stopBtn_ = new QPushButton(toolbar);
    stopBtn_->setAccessibleName("Stop");
    stopBtn_->setFont(transportFont);
    stopBtn_->setText(QString(icons::fa::Stop));
    stopBtn_->setToolTip("Stop and return to clip start");
    stopBtn_->setFixedSize(tBtnSize, tBtnSize);
    connect(stopBtn_, &QPushButton::clicked, this, &AudioClipEditor::onStop);
    tbLayout->addWidget(stopBtn_);

    tbLayout->addSpacing(4);

    positionLabel_ = new QLabel("0:00.000", toolbar);
    positionLabel_->setAccessibleName("Playback Position");
    positionLabel_->setFixedWidth(70);
    positionLabel_->setStyleSheet(
        QString("color: %1; font-size: 11px; font-family: monospace;").arg(theme.text.name()));
    tbLayout->addWidget(positionLabel_);

    positionTimer_ = new QTimer(this);
    positionTimer_->setInterval(50);
    connect(positionTimer_, &QTimer::timeout, this, &AudioClipEditor::updatePositionDisplay);
    connect(positionTimer_, &QTimer::timeout, this, &AudioClipEditor::updateTransportButtons);

    tbLayout->addStretch();

    auto* snapLabel = new QLabel("Snap:", toolbar);
    snapLabel->setAccessibleName("Snap Label");
    snapLabel->setStyleSheet(QString("color: %1;").arg(theme.textDim.name()));
    tbLayout->addWidget(snapLabel);

    snapCombo_ = new QComboBox(toolbar);
    snapCombo_->setAccessibleName("Snap Mode");
    snapCombo_->addItem("Off");
    snapCombo_->addItem("1/4 Beat");
    snapCombo_->addItem("1/2 Beat");
    snapCombo_->addItem("Beat");
    snapCombo_->addItem("Bar");
    snapCombo_->setCurrentIndex(3);
    snapCombo_->setFixedWidth(100);
    connect(snapCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &AudioClipEditor::onSnapChanged);
    tbLayout->addWidget(snapCombo_);
    tbLayout->addSpacing(4);

    auto* zoomInBtn = new QPushButton("+", toolbar);
    zoomInBtn->setAccessibleName("Zoom In");
    zoomInBtn->setFixedSize(24, 24);
    connect(zoomInBtn, &QPushButton::clicked, this, [this]() {
        waveformView_->setPixelsPerBeat(waveformView_->pixelsPerBeat() * 1.3);
    });
    tbLayout->addWidget(zoomInBtn);

    auto* zoomOutBtn = new QPushButton("-", toolbar);
    zoomOutBtn->setAccessibleName("Zoom Out");
    zoomOutBtn->setFixedSize(24, 24);
    connect(zoomOutBtn, &QPushButton::clicked, this, [this]() {
        waveformView_->setPixelsPerBeat(waveformView_->pixelsPerBeat() / 1.3);
    });
    tbLayout->addWidget(zoomOutBtn);
    tbLayout->addSpacing(8);

    const int btnSize = 24;
    const auto faFont = icons::fontAudio(16);
    const QString toggleStyle = QString(
        "QPushButton { min-width: 24px; min-height: 24px; font-size: 12px; }"
        "QPushButton:checked { background: %1; border-color: %2; }")
        .arg(theme.surfaceLight.name(), theme.accent.name());

    loopBtn_ = new QPushButton(toolbar);
    loopBtn_->setAccessibleName("Loop Toggle");
    loopBtn_->setCheckable(true);
    loopBtn_->setFont(faFont);
    loopBtn_->setText(QString(icons::fa::Loop));
    loopBtn_->setToolTip("Toggle loop");
    loopBtn_->setFixedSize(btnSize, btnSize);
    loopBtn_->setStyleSheet(toggleStyle);
    connect(loopBtn_, &QPushButton::toggled, this, &AudioClipEditor::onLoopToggled);
    tbLayout->addWidget(loopBtn_);

    reverseBtn_ = new QPushButton(toolbar);
    reverseBtn_->setAccessibleName("Reverse Toggle");
    reverseBtn_->setCheckable(true);
    reverseBtn_->setFont(faFont);
    reverseBtn_->setText(QString(icons::fa::Backward));
    reverseBtn_->setToolTip("Reverse audio");
    reverseBtn_->setFixedSize(btnSize, btnSize);
    reverseBtn_->setStyleSheet(toggleStyle);
    connect(reverseBtn_, &QPushButton::toggled, this, &AudioClipEditor::onReverseToggled);
    tbLayout->addWidget(reverseBtn_);

    stereoSplitBtn_ = new QPushButton(toolbar);
    stereoSplitBtn_->setAccessibleName("Split Stereo View");
    stereoSplitBtn_->setCheckable(true);
    stereoSplitBtn_->setFont(faFont);
    stereoSplitBtn_->setText(QString(icons::fa::Waveform));
    stereoSplitBtn_->setToolTip("Viewing combined waveform (click to see L/R channels)");
    stereoSplitBtn_->setFixedSize(btnSize, btnSize);
    stereoSplitBtn_->setStyleSheet(toggleStyle);
    connect(stereoSplitBtn_, &QPushButton::toggled, this, [this](bool checked) {
        if (waveformView_) waveformView_->setSplitStereo(checked);
        stereoSplitBtn_->setText(checked ? QString(icons::fa::Stereo)
                                         : QString(icons::fa::Waveform));
        stereoSplitBtn_->setToolTip(checked ? "Viewing L/R channels separately (click for combined view)"
                                             : "Viewing combined waveform (click to see L/R channels)");
    });
    tbLayout->addWidget(stereoSplitBtn_);
    tbLayout->addSpacing(8);

    auto* gainTitle = new QLabel("Gain:", toolbar);
    gainTitle->setAccessibleName("Gain Label");
    gainTitle->setStyleSheet(QString("color: %1;").arg(theme.textDim.name()));
    tbLayout->addWidget(gainTitle);

    gainSlider_ = new QSlider(Qt::Horizontal, toolbar);
    gainSlider_->setAccessibleName("Clip Gain");
    gainSlider_->setRange(-240, 120);
    gainSlider_->setValue(0);
    gainSlider_->setFixedWidth(100);
    gainSlider_->setToolTip("Clip gain (dB)");
    connect(gainSlider_, &QSlider::valueChanged, this, &AudioClipEditor::onGainSliderChanged);
    tbLayout->addWidget(gainSlider_);

    gainLabel_ = new QLabel("0.0 dB", toolbar);
    gainLabel_->setAccessibleName("Gain Value");
    gainLabel_->setFixedWidth(50);
    gainLabel_->setStyleSheet(QString("color: %1; font-size: 10px;").arg(theme.text.name()));
    tbLayout->addWidget(gainLabel_);

    normalizeBtn_ = new QPushButton("Norm", toolbar);
    normalizeBtn_->setAccessibleName("Normalize");
    normalizeBtn_->setToolTip("Normalize clip to peak level");
    normalizeBtn_->setFixedHeight(24);
    connect(normalizeBtn_, &QPushButton::clicked, this, &AudioClipEditor::onNormalizeClicked);
    tbLayout->addWidget(normalizeBtn_);

    bakeGainBtn_ = new QPushButton("Bake", toolbar);
    bakeGainBtn_->setAccessibleName("Bake Gain");
    bakeGainBtn_->setToolTip("Bake current gain into clip permanently");
    bakeGainBtn_->setFixedHeight(24);
    connect(bakeGainBtn_, &QPushButton::clicked, this, &AudioClipEditor::onBakeGainClicked);
    tbLayout->addWidget(bakeGainBtn_);
    tbLayout->addSpacing(8);

    auto* tempoLabel = new QLabel("BPM:", toolbar);
    tempoLabel->setAccessibleName("BPM Label");
    tempoLabel->setStyleSheet(QString("color: %1;").arg(theme.textDim.name()));
    tbLayout->addWidget(tempoLabel);

    bpmSpinBox_ = new QDoubleSpinBox(toolbar);
    bpmSpinBox_->setAccessibleName("Clip BPM");
    bpmSpinBox_->setRange(20.0, 400.0);
    bpmSpinBox_->setValue(120.0);
    bpmSpinBox_->setDecimals(1);
    bpmSpinBox_->setSingleStep(1.0);
    bpmSpinBox_->setFixedWidth(75);
    bpmSpinBox_->setToolTip("Clip tempo (BPM)");
    connect(bpmSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &AudioClipEditor::onClipBpmChanged);
    tbLayout->addWidget(bpmSpinBox_);

    autoTempoBtn_ = new QPushButton("Auto", toolbar);
    autoTempoBtn_->setAccessibleName("Auto-Tempo Toggle");
    autoTempoBtn_->setCheckable(true);
    autoTempoBtn_->setToolTip("Stretch clip to match project tempo");
    autoTempoBtn_->setFixedSize(42, 24);
    autoTempoBtn_->setStyleSheet(toggleStyle);
    connect(autoTempoBtn_, &QPushButton::toggled, this, &AudioClipEditor::onAutoTempoToggled);
    tbLayout->addWidget(autoTempoBtn_);

    mainLayout->addWidget(toolbar);

    // ── Toolbar Row 2 (Edit Operations) ──────────────────────────────────────
    auto* editBar = new QWidget(this);
    editBar->setFixedHeight(34);
    editBar->setAutoFillBackground(true);
    editBar->setPalette(tbPal);

    auto* ebLayout = new QHBoxLayout(editBar);
    ebLayout->setContentsMargins(6, 2, 6, 2);
    ebLayout->setSpacing(4);

    const int eBtnSz = 32;
    const auto eFaFont = icons::fontAudio(16);
    const auto eMiFont = icons::materialIcons(16);

    auto makeIconBtn = [&](const QFont& font, const QChar& glyph,
                           const QString& tooltip, const QString& accName) -> QPushButton* {
        auto* b = new QPushButton(QString(glyph), editBar);
        b->setFont(font);
        b->setAccessibleName(accName);
        b->setToolTip(tooltip);
        b->setFixedSize(eBtnSz, eBtnSz - 4);
        return b;
    };

    auto makeSep = [&]() {
        auto* sep = new QWidget(editBar);
        sep->setFixedSize(1, 22);
        sep->setStyleSheet(QString("background: %1;").arg(theme.border.name()));
        ebLayout->addWidget(sep);
    };

    undoBtn_ = makeIconBtn(eFaFont, icons::fa::Undo, "Undo (Ctrl+Z)", "Undo");
    connect(undoBtn_, &QPushButton::clicked, this, &AudioClipEditor::onUndo);
    ebLayout->addWidget(undoBtn_);

    redoBtn_ = makeIconBtn(eFaFont, icons::fa::Redo, "Redo (Ctrl+Y)", "Redo");
    connect(redoBtn_, &QPushButton::clicked, this, &AudioClipEditor::onRedo);
    ebLayout->addWidget(redoBtn_);

    makeSep();

    cutBtn_ = makeIconBtn(eFaFont, icons::fa::Scissors, "Cut (Ctrl+X)", "Cut");
    connect(cutBtn_, &QPushButton::clicked, this, &AudioClipEditor::onCut);
    ebLayout->addWidget(cutBtn_);

    copyBtn_ = makeIconBtn(eFaFont, icons::fa::Copy, "Copy (Ctrl+C)", "Copy");
    connect(copyBtn_, &QPushButton::clicked, this, &AudioClipEditor::onCopy);
    ebLayout->addWidget(copyBtn_);

    pasteBtn_ = makeIconBtn(eFaFont, icons::fa::Paste, "Paste (Ctrl+V)", "Paste");
    connect(pasteBtn_, &QPushButton::clicked, this, &AudioClipEditor::onPaste);
    ebLayout->addWidget(pasteBtn_);

    deleteBtn_ = makeIconBtn(eMiFont, icons::mi::Delete, "Delete selection (Del)", "Delete");
    connect(deleteBtn_, &QPushButton::clicked, this, &AudioClipEditor::onDeleteSel);
    ebLayout->addWidget(deleteBtn_);

    makeSep();

    silenceBtn_ = makeIconBtn(eFaFont, icons::fa::Mute, "Silence selection", "Silence");
    connect(silenceBtn_, &QPushButton::clicked, this, &AudioClipEditor::onSilence);
    ebLayout->addWidget(silenceBtn_);

    fadeInBtn_ = makeIconBtn(eFaFont, icons::fa::PunchIn, "Bake fade-in on selection (destructive)", "Bake Fade In");
    connect(fadeInBtn_, &QPushButton::clicked, this, &AudioClipEditor::onFadeIn);
    ebLayout->addWidget(fadeInBtn_);

    fadeOutBtn_ = makeIconBtn(eFaFont, icons::fa::PunchOut, "Bake fade-out on selection (destructive)", "Bake Fade Out");
    connect(fadeOutBtn_, &QPushButton::clicked, this, &AudioClipEditor::onFadeOut);
    ebLayout->addWidget(fadeOutBtn_);

    bakeClipFadesBtn_ = makeIconBtn(eFaFont, icons::fa::Lock, "Bake the clip's fade handles into the file", "Bake Clip Fades");
    connect(bakeClipFadesBtn_, &QPushButton::clicked, this, &AudioClipEditor::onBakeClipFades);
    ebLayout->addWidget(bakeClipFadesBtn_);

    normalizeSelBtn_ = makeIconBtn(eFaFont, icons::fa::VExpand, "Normalize selection to peak", "Normalize Selection");
    connect(normalizeSelBtn_, &QPushButton::clicked, this, &AudioClipEditor::onNormalizeSel);
    ebLayout->addWidget(normalizeSelBtn_);

    reverseSelBtn_ = makeIconBtn(eFaFont, icons::fa::Backward, "Reverse selection", "Reverse Selection");
    connect(reverseSelBtn_, &QPushButton::clicked, this, &AudioClipEditor::onReverseSel);
    ebLayout->addWidget(reverseSelBtn_);

    volumeBtn_ = makeIconBtn(eFaFont, icons::fa::Speaker, "Adjust volume of selection...", "Adjust Volume");
    connect(volumeBtn_, &QPushButton::clicked, this, &AudioClipEditor::onAdjustVolume);
    ebLayout->addWidget(volumeBtn_);

    makeSep();

    insertSilBtn_ = makeIconBtn(eMiFont, icons::mi::Add, "Insert silence at position...", "Insert Silence");
    connect(insertSilBtn_, &QPushButton::clicked, this, &AudioClipEditor::onInsertSilence);
    ebLayout->addWidget(insertSilBtn_);

    dcRemoveBtn_ = makeIconBtn(eMiFont, icons::mi::Tune, "Remove DC offset from file", "DC Remove");
    connect(dcRemoveBtn_, &QPushButton::clicked, this, &AudioClipEditor::onDCRemove);
    ebLayout->addWidget(dcRemoveBtn_);

    swapChBtn_ = makeIconBtn(eFaFont, icons::fa::ArrowsHorz, "Swap left/right channels", "Swap Channels");
    connect(swapChBtn_, &QPushButton::clicked, this, &AudioClipEditor::onSwapChannels);
    ebLayout->addWidget(swapChBtn_);

    monoBtn_ = makeIconBtn(eFaFont, icons::fa::Mono, "Mix to mono", "Mix to Mono");
    connect(monoBtn_, &QPushButton::clicked, this, &AudioClipEditor::onMixToMono);
    ebLayout->addWidget(monoBtn_);

    crossfadeBtn_ = makeIconBtn(eFaFont, icons::fa::Shuffle, "Crossfade at selection point", "Crossfade");
    connect(crossfadeBtn_, &QPushButton::clicked, this, &AudioClipEditor::onCrossfade);
    ebLayout->addWidget(crossfadeBtn_);

    makeSep();

    selectionInfoLabel_ = new QLabel("No selection", editBar);
    selectionInfoLabel_->setAccessibleName("Selection Info");
    selectionInfoLabel_->setStyleSheet(
        QString("color: %1; font-size: 10px;").arg(theme.textDim.name()));
    ebLayout->addWidget(selectionInfoLabel_);

    ebLayout->addStretch();
    mainLayout->addWidget(editBar);

    // ── Waveform view ────────────────────────────────────────────────────────
    waveformView_ = new AudioWaveformView(this);
    mainLayout->addWidget(waveformView_, 1);

    connect(waveformView_, &AudioWaveformView::clipModified, this, [this]() {
        updateInfoBar();
        emit clipModified();
    });

    connect(waveformView_, &AudioWaveformView::selectionChanged,
            this, &AudioClipEditor::updateSelectionInfo);

    waveformView_->setContextMenuCallback(
        [this](const QPoint& pos) { buildContextMenu(pos); });

    // ── Info bar ─────────────────────────────────────────────────────────────
    auto* infoBar = new QWidget(this);
    infoBar->setFixedHeight(22);
    infoBar->setAutoFillBackground(true);
    QPalette infoPal;
    infoPal.setColor(QPalette::Window, theme.surface);
    infoBar->setPalette(infoPal);

    auto* infoLayout = new QHBoxLayout(infoBar);
    infoLayout->setContentsMargins(8, 2, 8, 2);
    infoLayout->setSpacing(16);
    auto infoStyle = QString("color: %1; font-size: 10px;").arg(theme.textDim.name());

    infoBpmLabel_ = new QLabel("BPM: --", infoBar);
    infoBpmLabel_->setAccessibleName("Info BPM");
    infoBpmLabel_->setStyleSheet(infoStyle);
    infoLayout->addWidget(infoBpmLabel_);

    infoBeatsLabel_ = new QLabel("Beats: --", infoBar);
    infoBeatsLabel_->setAccessibleName("Info Beats");
    infoBeatsLabel_->setStyleSheet(infoStyle);
    infoLayout->addWidget(infoBeatsLabel_);

    infoDurationLabel_ = new QLabel("Duration: --", infoBar);
    infoDurationLabel_->setAccessibleName("Info Duration");
    infoDurationLabel_->setStyleSheet(infoStyle);
    infoLayout->addWidget(infoDurationLabel_);

    infoFormatLabel_ = new QLabel("Format: --", infoBar);
    infoFormatLabel_->setAccessibleName("Info Format");
    infoFormatLabel_->setStyleSheet(infoStyle);
    infoLayout->addWidget(infoFormatLabel_);

    infoFileLabel_ = new QLabel("File: --", infoBar);
    infoFileLabel_->setAccessibleName("Info File");
    infoFileLabel_->setStyleSheet(infoStyle);
    infoLayout->addWidget(infoFileLabel_);

    infoModeLabel_ = new QLabel("Mode: --", infoBar);
    infoModeLabel_->setAccessibleName("Info Mode");
    infoModeLabel_->setStyleSheet(infoStyle);
    infoLayout->addWidget(infoModeLabel_);

    infoLayout->addStretch();
    mainLayout->addWidget(infoBar);

    // ── Keyboard shortcuts (WidgetWithChildrenShortcut so they only fire
    //    when focus is inside this editor, not captured by main window) ────
    auto makeShortcut = [this](const QKeySequence& seq) {
        auto* sc = new QShortcut(seq, this);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        return sc;
    };

    connect(makeShortcut(QKeySequence("Ctrl+Z")), &QShortcut::activated,
            this, &AudioClipEditor::onUndo);
    connect(makeShortcut(QKeySequence("Ctrl+Y")), &QShortcut::activated,
            this, &AudioClipEditor::onRedo);
    connect(makeShortcut(QKeySequence("Ctrl+Shift+Z")), &QShortcut::activated,
            this, &AudioClipEditor::onRedo);
    connect(makeShortcut(QKeySequence("Ctrl+X")), &QShortcut::activated,
            this, &AudioClipEditor::onCut);
    connect(makeShortcut(QKeySequence("Ctrl+C")), &QShortcut::activated,
            this, &AudioClipEditor::onCopy);
    connect(makeShortcut(QKeySequence("Ctrl+V")), &QShortcut::activated,
            this, &AudioClipEditor::onPaste);
    connect(makeShortcut(QKeySequence(Qt::Key_Delete)), &QShortcut::activated,
            this, &AudioClipEditor::onDeleteSel);
    connect(makeShortcut(QKeySequence("Ctrl+A")), &QShortcut::activated,
            this, [this]() { if (waveformView_) waveformView_->selectAll(); });

    onSnapChanged(3);
}

// ── Destructive edit warning / confirmation ───────────────────────────────────

bool AudioClipEditor::confirmDestructiveEdit(const juce::File& file)
{
    auto path = QString::fromStdString(file.getFullPathName().toStdString());
    if (confirmedFiles_.contains(path))
        return true;

    QMessageBox dlg(this);
    dlg.setWindowTitle("Destructive Edit Warning");
    dlg.setIcon(QMessageBox::Warning);
    dlg.setText(QString("<b>This will permanently modify your source file:</b><br><br>"
                        "<code>%1</code><br><br>"
                        "A backup of the original will be saved, but all changes "
                        "are written directly to the file on disk.")
                    .arg(path.toHtmlEscaped()));
    dlg.setInformativeText("Do you want to continue?");
    dlg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    dlg.setDefaultButton(QMessageBox::No);
    dlg.button(QMessageBox::Yes)->setText("Edit Source File");
    dlg.button(QMessageBox::No)->setText("Cancel");
    dlg.setAccessibleName("Destructive Edit Confirmation");

    if (dlg.exec() != QMessageBox::Yes)
        return false;

    confirmedFiles_.insert(path);
    return true;
}

void AudioClipEditor::updateWarningBanner()
{
    if (!clip_) {
        warningBanner_->setVisible(false);
        return;
    }
    auto file = clip_->getOriginalFile();
    auto path = QString::fromStdString(file.getFullPathName().toStdString());
    warningLabel_->setText(
        QString("DESTRUCTIVE MODE \xe2\x80\x94 Edits modify source file: %1")
            .arg(QString::fromStdString(file.getFileName().toStdString())));
    warningBanner_->setVisible(true);
}

// ── executeEdit: the universal destructive-edit dispatch ──────────────────────

void AudioClipEditor::executeEdit(const QString& description,
                                   std::function<bool(const juce::File&)> operation)
{
    if (!clip_) return;
    auto file = clip_->getOriginalFile();
    if (!file.existsAsFile()) return;

    if (!confirmDestructiveEdit(file))
        return;

    undoManager_->savePristineBackup(file);
    undoManager_->pushUndo(file, description);

    if (!operation(file)) {
        undoManager_->undo(file);
        return;
    }

    WaveformCache::instance().invalidate(file);
    waveformView_->rebuild();
    updateUndoButtons();
    emit clipModified();
}

// ── Undo / Redo ──────────────────────────────────────────────────────────────

void AudioClipEditor::onUndo()
{
    if (!clip_) return;
    auto file = clip_->getOriginalFile();
    if (undoManager_->undo(file)) {
        WaveformCache::instance().invalidate(file);
        waveformView_->rebuild();
        updateUndoButtons();
        emit clipModified();
    }
}

void AudioClipEditor::onRedo()
{
    if (!clip_) return;
    auto file = clip_->getOriginalFile();
    if (undoManager_->redo(file)) {
        WaveformCache::instance().invalidate(file);
        waveformView_->rebuild();
        updateUndoButtons();
        emit clipModified();
    }
}

void AudioClipEditor::updateUndoButtons()
{
    if (undoBtn_) {
        undoBtn_->setEnabled(undoManager_->canUndo());
        undoBtn_->setToolTip(undoManager_->canUndo()
            ? QString("Undo: %1").arg(undoManager_->undoDescription())
            : "Nothing to undo");
    }
    if (redoBtn_) {
        redoBtn_->setEnabled(undoManager_->canRedo());
        redoBtn_->setToolTip(undoManager_->canRedo()
            ? QString("Redo: %1").arg(undoManager_->redoDescription())
            : "Nothing to redo");
    }
}

// ── Selection-based operations ───────────────────────────────────────────────

void AudioClipEditor::onCut()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;

    auto file = clip_->getOriginalFile();
    clipboard_ = AudioFileOperations::copySelection(file, sel);

    executeEdit("Cut", [sel](const juce::File& f) {
        return AudioFileOperations::deleteSelection(f, sel);
    });
    waveformView_->clearSelection();
}

void AudioClipEditor::onCopy()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;
    clipboard_ = AudioFileOperations::copySelection(clip_->getOriginalFile(), sel);
}

void AudioClipEditor::onPaste()
{
    if (!clip_ || clipboard_.getNumSamples() == 0) return;
    auto sel = waveformView_->selection();
    juce::int64 pos = sel.isEmpty() ? 0 : sel.startSample;
    double sr = waveformView_->sampleRate();

    executeEdit("Paste", [this, pos, sr](const juce::File& f) {
        return AudioFileOperations::pasteAtPosition(f, pos, clipboard_, sr);
    });
}

void AudioClipEditor::onDeleteSel()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;

    executeEdit("Delete Selection", [sel](const juce::File& f) {
        return AudioFileOperations::deleteSelection(f, sel);
    });
    waveformView_->clearSelection();
}

void AudioClipEditor::onSilence()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;

    executeEdit("Silence Selection", [sel](const juce::File& f) {
        return AudioFileOperations::silenceSelection(f, sel);
    });
}

void AudioClipEditor::onFadeIn()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;

    executeEdit("Fade In", [sel](const juce::File& f) {
        return AudioFileOperations::fadeInSelection(f, sel);
    });
}

void AudioClipEditor::onFadeOut()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;

    executeEdit("Fade Out", [sel](const juce::File& f) {
        return AudioFileOperations::fadeOutSelection(f, sel);
    });
}

void AudioClipEditor::onBakeClipFades()
{
    if (!clip_) return;

    double fadeInSec = clip_->getFadeIn().inSeconds();
    double fadeOutSec = clip_->getFadeOut().inSeconds();

    if (fadeInSec < 0.001 && fadeOutSec < 0.001) return;

    double sr = waveformView_->sampleRate();
    juce::int64 total = waveformView_->totalSamples();
    if (sr <= 0 || total <= 0) return;

    juce::int64 fadeInSamples = static_cast<juce::int64>(fadeInSec * sr);
    juce::int64 fadeOutSamples = static_cast<juce::int64>(fadeOutSec * sr);

    WaveformSelection fadeInSel{0, std::min(fadeInSamples, total)};
    WaveformSelection fadeOutSel{std::max(juce::int64(0), total - fadeOutSamples), total};

    executeEdit("Bake Clip Fades", [fadeInSel, fadeOutSel, fadeInSec, fadeOutSec](const juce::File& f) {
        bool ok = true;
        if (fadeInSec > 0.001)
            ok = ok && AudioFileOperations::fadeInSelection(f, fadeInSel);
        if (fadeOutSec > 0.001)
            ok = ok && AudioFileOperations::fadeOutSelection(f, fadeOutSel);
        return ok;
    });

    clip_->setFadeIn(tracktion::TimeDuration::fromSeconds(0.0));
    clip_->setFadeOut(tracktion::TimeDuration::fromSeconds(0.0));
    waveformView_->rebuild();
}

void AudioClipEditor::onNormalizeSel()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;

    executeEdit("Normalize Selection", [sel](const juce::File& f) {
        return AudioFileOperations::normalizeSelection(f, sel);
    });
}

void AudioClipEditor::onReverseSel()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;

    executeEdit("Reverse Selection", [sel](const juce::File& f) {
        return AudioFileOperations::reverseSelection(f, sel);
    });
}

void AudioClipEditor::onAdjustVolume()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) return;

    bool ok = false;
    double dB = QInputDialog::getDouble(this, "Adjust Volume",
                                         "Gain (dB):", 0.0, -60.0, 24.0, 1, &ok);
    if (!ok) return;

    float fdB = static_cast<float>(dB);
    executeEdit(QString("Adjust Volume %1 dB").arg(dB, 0, 'f', 1),
                [sel, fdB](const juce::File& f) {
                    return AudioFileOperations::adjustSelectionGain(f, sel, fdB);
                });
}

// ── Whole-file operations ────────────────────────────────────────────────────

void AudioClipEditor::onInsertSilence()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    juce::int64 pos = sel.isEmpty() ? 0 : sel.startSample;
    double sr = waveformView_->sampleRate();
    if (sr <= 0) sr = 44100;

    bool ok = false;
    double seconds = QInputDialog::getDouble(this, "Insert Silence",
                                              "Duration (seconds):", 1.0, 0.01, 60.0, 2, &ok);
    if (!ok) return;

    juce::int64 numSamples = static_cast<juce::int64>(seconds * sr);
    executeEdit("Insert Silence", [pos, numSamples](const juce::File& f) {
        return AudioFileOperations::insertSilence(f, pos, numSamples);
    });
}

void AudioClipEditor::onDCRemove()
{
    if (!clip_) return;
    executeEdit("Remove DC Offset", [](const juce::File& f) {
        return AudioFileOperations::removeDCOffset(f);
    });
}

void AudioClipEditor::onSwapChannels()
{
    if (!clip_) return;
    executeEdit("Swap Channels", [](const juce::File& f) {
        return AudioFileOperations::swapChannels(f);
    });
}

void AudioClipEditor::onMixToMono()
{
    if (!clip_) return;
    executeEdit("Mix to Mono", [](const juce::File& f) {
        return AudioFileOperations::mixToMono(f);
    });
}

void AudioClipEditor::onCrossfade()
{
    if (!clip_) return;
    auto sel = waveformView_->selection();
    juce::int64 pos = sel.isEmpty() ? 0 : sel.startSample;
    double sr = waveformView_->sampleRate();
    int crossfadeSamples = static_cast<int>(0.01 * sr);

    executeEdit("Crossfade", [pos, crossfadeSamples](const juce::File& f) {
        return AudioFileOperations::crossfadeAtPoint(f, pos, crossfadeSamples);
    });
}

// ── Context menu ─────────────────────────────────────────────────────────────

void AudioClipEditor::buildContextMenu(const QPoint& globalPos)
{
    QMenu menu;
    menu.setAccessibleName("Waveform Context Menu");

    auto sel = waveformView_->selection();
    bool hasSel = !sel.isEmpty();

    auto* actCut = menu.addAction("Cut", this, &AudioClipEditor::onCut);
    actCut->setEnabled(hasSel);
    auto* actCopy = menu.addAction("Copy", this, &AudioClipEditor::onCopy);
    actCopy->setEnabled(hasSel);
    auto* actPaste = menu.addAction("Paste", this, &AudioClipEditor::onPaste);
    actPaste->setEnabled(clipboard_.getNumSamples() > 0);
    auto* actDel = menu.addAction("Delete", this, &AudioClipEditor::onDeleteSel);
    actDel->setEnabled(hasSel);

    menu.addSeparator();

    auto* actSilence = menu.addAction("Silence", this, &AudioClipEditor::onSilence);
    actSilence->setEnabled(hasSel);
    auto* actFadeIn = menu.addAction("Bake Fade In (selection)", this, &AudioClipEditor::onFadeIn);
    actFadeIn->setEnabled(hasSel);
    auto* actFadeOut = menu.addAction("Bake Fade Out (selection)", this, &AudioClipEditor::onFadeOut);
    actFadeOut->setEnabled(hasSel);
    bool hasFadeHandles = clip_ && (clip_->getFadeIn().inSeconds() > 0.001
                                    || clip_->getFadeOut().inSeconds() > 0.001);
    auto* actBakeFades = menu.addAction("Bake Clip Fade Handles", this, &AudioClipEditor::onBakeClipFades);
    actBakeFades->setEnabled(hasFadeHandles);
    auto* actNorm = menu.addAction("Normalize", this, &AudioClipEditor::onNormalizeSel);
    actNorm->setEnabled(hasSel);
    auto* actRev = menu.addAction("Reverse", this, &AudioClipEditor::onReverseSel);
    actRev->setEnabled(hasSel);
    auto* actVol = menu.addAction("Adjust Volume...", this, &AudioClipEditor::onAdjustVolume);
    actVol->setEnabled(hasSel);

    menu.addSeparator();

    menu.addAction("Insert Silence...", this, &AudioClipEditor::onInsertSilence);
    menu.addAction("Remove DC Offset", this, &AudioClipEditor::onDCRemove);
    menu.addAction("Swap Channels (L/R)", this, &AudioClipEditor::onSwapChannels);
    menu.addAction("Mix to Mono", this, &AudioClipEditor::onMixToMono);
    menu.addAction("Crossfade at Point", this, &AudioClipEditor::onCrossfade);

    menu.addSeparator();
    menu.addAction("Select All", this, [this]() { waveformView_->selectAll(); });

    menu.exec(globalPos);
}

// ── Selection info ───────────────────────────────────────────────────────────

void AudioClipEditor::updateSelectionInfo()
{
    auto sel = waveformView_->selection();
    if (sel.isEmpty()) {
        selectionInfoLabel_->setText("No selection");
        return;
    }
    double sr = waveformView_->sampleRate();
    double seconds = (sr > 0) ? sel.lengthSamples() / sr : 0.0;
    selectionInfoLabel_->setText(QString("Sel: %1s (%2 samples)")
        .arg(seconds, 0, 'f', 3)
        .arg(sel.lengthSamples()));
}

// ── Clip property controls (same as before) ──────────────────────────────────

void AudioClipEditor::setClip(te::WaveAudioClip* clip, EditManager* editMgr)
{
    clip_ = clip;
    editMgr_ = editMgr;

    undoManager_->clear();
    waveformView_->setClip(clip, editMgr);

    if (clip_) {
        positionTimer_->start();
        clipNameLabel_->setText(QString::fromStdString(clip_->getName().toStdString()));

        { QSignalBlocker b(gainSlider_);
          gainSlider_->setValue(static_cast<int>(clip_->getGainDB() * 10.0)); }
        updateGainLabel();

        { QSignalBlocker b(bpmSpinBox_);
          auto& loopInfo = clip_->getLoopInfo();
          auto fileInfo = te::AudioFileInfo::parse(clip_->getAudioFile());
          double clipBpm = loopInfo.getBpm(fileInfo);
          bpmSpinBox_->setValue(clipBpm > 0.0 ? clipBpm
                               : (editMgr_ ? editMgr_->getBpm() : 120.0)); }

        { QSignalBlocker b(autoTempoBtn_);
          autoTempoBtn_->setChecked(clip_->getAutoTempo()); }

        { QSignalBlocker b(loopBtn_);
          loopBtn_->setChecked(clip_->isLooping()); }

        { QSignalBlocker b(reverseBtn_);
          reverseBtn_->setChecked(clip_->getIsReversed()); }

        stereoSplitBtn_->setEnabled(waveformView_->numChannels() >= 2);
        if (waveformView_->numChannels() < 2) {
            QSignalBlocker b(stereoSplitBtn_);
            stereoSplitBtn_->setChecked(false);
        }

        updateInfoBar();
        updateUndoButtons();
        updateWarningBanner();
    } else {
        positionTimer_->stop();
        clipNameLabel_->setText("No clip selected");
        positionLabel_->setText("0:00.000");
        selectionInfoLabel_->setText("No selection");
        warningBanner_->setVisible(false);
        infoBpmLabel_->setText("BPM: --");
        infoBeatsLabel_->setText("Beats: --");
        infoDurationLabel_->setText("Duration: --");
        infoFormatLabel_->setText("Format: --");
        infoFileLabel_->setText("File: --");
        infoModeLabel_->setText("Mode: --");
    }
}

void AudioClipEditor::refresh()
{
    if (!clip_) return;
    waveformView_->rebuild();
    updateInfoBar();
}

void AudioClipEditor::onSnapChanged(int index)
{
    static constexpr SnapMode mapping[] = {
        SnapMode::Off, SnapMode::QuarterBeat, SnapMode::HalfBeat,
        SnapMode::Beat, SnapMode::Bar
    };
    if (index >= 0 && index < 5)
        waveformView_->snapper().setMode(mapping[index]);
}

void AudioClipEditor::onGainSliderChanged(int value)
{
    if (!clip_) return;
    float dB = value / 10.0f;
    clip_->setGainDB(dB);
    updateGainLabel();
    waveformView_->setGainMultiplier(std::pow(10.0, dB / 20.0));
    emit clipModified();
}

void AudioClipEditor::updateGainLabel()
{
    if (!clip_) { gainLabel_->setText("0.0 dB"); return; }
    gainLabel_->setText(QString("%1 dB").arg(clip_->getGainDB(), 0, 'f', 1));
}

void AudioClipEditor::onAutoTempoToggled(bool checked)
{
    if (!clip_) return;
    clip_->setAutoTempo(checked);
    if (checked) {
        double numBeats = clip_->getLoopInfo().getNumBeats();
        if (numBeats > 0.0)
            clip_->setLoopRangeBeats({tracktion::BeatPosition::fromBeats(0.0),
                                      tracktion::BeatDuration::fromBeats(numBeats)});
    }
    waveformView_->rebuild();
    updateInfoBar();
    emit clipModified();
}

void AudioClipEditor::onLoopToggled(bool checked)
{
    if (!clip_) return;
    if (checked) {
        auto& li = clip_->getLoopInfo();
        if (li.isLoopable() && !li.isOneShot() && li.getNumBeats() > 0.0) {
            clip_->setAutoTempo(true);
            clip_->setLoopRangeBeats({tracktion::BeatPosition::fromBeats(0.0),
                                      tracktion::BeatDuration::fromBeats(li.getNumBeats())});
        } else {
            clip_->setLoopRange({tracktion::TimePosition::fromSeconds(0.0),
                                 clip_->getSourceLength()});
        }
    }
    waveformView_->rebuild();
    updateInfoBar();
    emit clipModified();
}

void AudioClipEditor::onReverseToggled(bool checked)
{
    if (!clip_) return;
    clip_->setIsReversed(checked);
    waveformView_->setReversed(checked);
    emit clipModified();
}

void AudioClipEditor::onClipBpmChanged(double bpm)
{
    if (!clip_ || bpm <= 0.0) return;
    auto loopInfo = clip_->getLoopInfo();
    auto sourceLen = clip_->getSourceLength().inSeconds();
    if (sourceLen > 0.0) {
        loopInfo.setNumBeats((bpm / 60.0) * sourceLen);
        auto fi = te::AudioFileInfo::parse(clip_->getAudioFile());
        loopInfo.setBpm(bpm, fi);
    }
    clip_->setLoopInfo(loopInfo);
    waveformView_->rebuild();
    updateInfoBar();
    emit clipModified();
}

void AudioClipEditor::onNormalizeClicked()
{
    if (!clip_) return;
    auto file = clip_->getOriginalFile();
    if (!file.existsAsFile()) return;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
    if (!reader) return;

    float peak = 0.0f;
    const int bs = 8192;
    juce::AudioBuffer<float> buf(static_cast<int>(reader->numChannels), bs);
    juce::int64 read = 0;
    while (read < static_cast<juce::int64>(reader->lengthInSamples)) {
        int toRead = static_cast<int>(std::min(static_cast<juce::int64>(bs),
                     static_cast<juce::int64>(reader->lengthInSamples) - read));
        reader->read(&buf, 0, toRead, read, true, true);
        for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
            auto r = juce::FloatVectorOperations::findMinAndMax(buf.getReadPointer(ch), toRead);
            peak = std::max(peak, std::max(std::abs(r.getStart()), std::abs(r.getEnd())));
        }
        read += toRead;
    }
    if (peak > 0.0001f) {
        float g = -20.0f * std::log10(peak);
        clip_->setGainDB(g);
        { QSignalBlocker b(gainSlider_); gainSlider_->setValue(static_cast<int>(g * 10.0)); }
        updateGainLabel();
        waveformView_->setGainMultiplier(std::pow(10.0, g / 20.0));
        emit clipModified();
    }
}

void AudioClipEditor::onBakeGainClicked()
{
    if (!clip_) return;
    float cg = clip_->getGainDB();
    if (std::abs(cg) < 0.01f) return;

    WaveformSelection all{0, waveformView_->totalSamples()};
    float dB = cg;

    executeEdit("Bake Gain", [all, dB](const juce::File& f) {
        return AudioFileOperations::adjustSelectionGain(f, all, dB);
    });

    clip_->setGainDB(0.0f);
    { QSignalBlocker b(gainSlider_); gainSlider_->setValue(0); }
    updateGainLabel();
    waveformView_->setGainMultiplier(1.0);
}

void AudioClipEditor::onPlayFromStart()
{
    if (!clip_ || !editMgr_) return;
    auto& t = editMgr_->transport();
    t.setPosition(clip_->getPosition().getStart());
    if (!t.isPlaying()) t.play(false);
}

void AudioClipEditor::onPlayPause()
{
    if (!editMgr_) return;
    auto& t = editMgr_->transport();
    if (t.isPlaying()) t.stop(false, false); else t.play(false);
}

void AudioClipEditor::onStop()
{
    if (!editMgr_) return;
    auto& t = editMgr_->transport();
    t.stop(false, false);
    if (clip_) t.setPosition(clip_->getPosition().getStart());
}

void AudioClipEditor::updateTransportButtons()
{
    if (!editMgr_) return;
    bool p = editMgr_->transport().isPlaying();
    playPauseBtn_->setText(p ? QString(icons::fa::Pause) : QString(icons::fa::Play));
    playPauseBtn_->setToolTip(p ? "Pause" : "Play");
}

void AudioClipEditor::updatePositionDisplay()
{
    if (!clip_ || !editMgr_) return;
    double off = std::max(0.0, editMgr_->transport().getPosition().inSeconds()
                          - clip_->getPosition().getStart().inSeconds());
    positionLabel_->setText(QString("%1:%2.%3")
        .arg(static_cast<int>(off) / 60)
        .arg(static_cast<int>(off) % 60, 2, 10, QChar('0'))
        .arg(static_cast<int>((off - std::floor(off)) * 1000.0), 3, 10, QChar('0')));
}

void AudioClipEditor::updateInfoBar()
{
    if (!clip_) return;
    auto& li = clip_->getLoopInfo();
    auto fi = te::AudioFileInfo::parse(clip_->getAudioFile());
    double bpm = li.getBpm(fi);

    infoBpmLabel_->setText(bpm > 0 ? QString("BPM: %1").arg(bpm, 0, 'f', 1)
                           : (editMgr_ ? QString("BPM: %1 (project)").arg(editMgr_->getBpm(), 0, 'f', 1)
                                       : "BPM: --"));

    double nb = li.getNumBeats();
    infoBeatsLabel_->setText(nb > 0 ? QString("Beats: %1").arg(nb, 0, 'f', 1) : "Beats: --");

    infoDurationLabel_->setText(QString("Duration: %1s")
        .arg(clip_->getSourceLength().inSeconds(), 0, 'f', 2));

    int ch = waveformView_->numChannels();
    int bits = waveformView_->bitsPerSample();
    double sr = waveformView_->sampleRate();
    QString chStr = (ch >= 2) ? "Stereo" : "Mono";
    infoFormatLabel_->setText(QString("%1 | %2-bit | %3 kHz")
        .arg(chStr).arg(bits).arg(sr / 1000.0, 0, 'f', 1));

    infoFileLabel_->setText(QString("File: %1")
        .arg(QString::fromStdString(clip_->getOriginalFile().getFileName().toStdString())));

    QString mode = clip_->getAutoTempo() ? "Stretch"
                 : clip_->getIsReversed() ? "Reversed" : "Normal";
    infoModeLabel_->setText(QString("Mode: %1").arg(mode));
}

} // namespace freedaw
