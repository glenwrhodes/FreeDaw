#include "PianoRollEditor.h"
#include "utils/IconFont.h"
#include "utils/ThemeManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QPushButton>
#include <QFrame>

namespace freedaw {

QPushButton* PianoRollEditor::makeIconButton(QWidget* parent, const QFont& font,
                                              const QChar& glyph, const QString& tooltip,
                                              const QString& accessible, int size)
{
    auto& theme = ThemeManager::instance().current();
    auto* btn = new QPushButton(parent);
    btn->setAccessibleName(accessible);
    btn->setFont(font);
    btn->setText(QString(glyph));
    btn->setToolTip(tooltip);
    btn->setFixedSize(size, size);
    btn->setStyleSheet(QString(
        "QPushButton { min-width: %1px; min-height: %1px; font-size: 14px; }"
        "QPushButton:hover { background: %2; }")
        .arg(size).arg(theme.surfaceLight.name()));
    return btn;
}

PianoRollEditor::PianoRollEditor(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Piano Roll Editor");
    auto& theme = ThemeManager::instance().current();

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(40);
    toolbar->setAutoFillBackground(true);
    QPalette tbPal;
    tbPal.setColor(QPalette::Window, theme.surface);
    toolbar->setPalette(tbPal);

    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(6, 4, 6, 4);
    tbLayout->setSpacing(4);

    clipNameLabel_ = new QLabel("No clip selected", toolbar);
    clipNameLabel_->setAccessibleName("Clip Name");
    clipNameLabel_->setStyleSheet(
        QString("color: %1; font-weight: bold; font-size: 11px;")
            .arg(theme.text.name()));
    tbLayout->addWidget(clipNameLabel_);

    tbLayout->addStretch();

    const int iconSize = 16;
    const int btnSize = 26;
    const auto faFont = icons::fontAudio(iconSize);

    const QString modeButtonStyle = QString(
        "QPushButton { min-width: 26px; min-height: 26px; font-size: 14px; }"
        "QPushButton:checked { background: %1; border-color: %2; }"
        "QPushButton:hover { background: %3; }")
        .arg(theme.surfaceLight.name(), theme.accent.name(), theme.surfaceLight.name());

    editModeBtn_ = new QPushButton(toolbar);
    editModeBtn_->setAccessibleName("Edit Mode");
    editModeBtn_->setCheckable(true);
    editModeBtn_->setChecked(true);
    editModeBtn_->setFont(faFont);
    editModeBtn_->setText(QString(icons::fa::Pointer));
    editModeBtn_->setToolTip("Edit mode (select & move)");
    editModeBtn_->setFixedSize(btnSize, btnSize);
    editModeBtn_->setStyleSheet(modeButtonStyle);
    tbLayout->addWidget(editModeBtn_);

    drawModeBtn_ = new QPushButton(toolbar);
    drawModeBtn_->setAccessibleName("Draw Mode");
    drawModeBtn_->setCheckable(true);
    drawModeBtn_->setChecked(false);
    drawModeBtn_->setFont(faFont);
    drawModeBtn_->setText(QString(icons::fa::Pen));
    drawModeBtn_->setToolTip("Draw mode (pencil)");
    drawModeBtn_->setFixedSize(btnSize, btnSize);
    drawModeBtn_->setStyleSheet(modeButtonStyle);
    tbLayout->addWidget(drawModeBtn_);

    connect(drawModeBtn_, &QPushButton::toggled,
            this, &PianoRollEditor::onDrawModeToggled);
    connect(editModeBtn_, &QPushButton::toggled,
            this, &PianoRollEditor::onEditModeToggled);

    // Separator
    auto* sep1 = new QFrame(toolbar);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet(QString("color: %1;").arg(theme.surfaceLight.name()));
    tbLayout->addWidget(sep1);

    // Clipboard icon buttons
    auto* cutBtn = makeIconButton(toolbar, faFont, icons::fa::Scissors,
                                   "Cut (Ctrl+X)", "Cut", btnSize);
    tbLayout->addWidget(cutBtn);
    auto* copyBtn = makeIconButton(toolbar, faFont, icons::fa::Copy,
                                    "Copy (Ctrl+C)", "Copy", btnSize);
    tbLayout->addWidget(copyBtn);
    auto* pasteBtn = makeIconButton(toolbar, faFont, icons::fa::Paste,
                                     "Paste (Ctrl+V)", "Paste", btnSize);
    tbLayout->addWidget(pasteBtn);
    auto* dupBtn = makeIconButton(toolbar, faFont, icons::fa::Duplicate,
                                   "Duplicate (Ctrl+D)", "Duplicate", btnSize);
    tbLayout->addWidget(dupBtn);

    auto* sep2 = new QFrame(toolbar);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setStyleSheet(QString("color: %1;").arg(theme.surfaceLight.name()));
    tbLayout->addWidget(sep2);

    // Transform icon buttons
    auto* quantizeBtn = makeIconButton(toolbar, faFont, icons::fa::Timeselect,
                                        "Quantize... (Ctrl+Q)", "Quantize", btnSize);
    tbLayout->addWidget(quantizeBtn);

    auto* legatoBtn = makeIconButton(toolbar, faFont, icons::fa::HExpand,
                                      "Legato (Ctrl+L)", "Legato", btnSize);
    tbLayout->addWidget(legatoBtn);

    auto* reverseBtn = makeIconButton(toolbar, faFont, icons::fa::Shuffle,
                                       "Reverse", "Reverse", btnSize);
    tbLayout->addWidget(reverseBtn);

    auto* transpUpBtn = makeIconButton(toolbar, faFont, icons::fa::ArrowsVert,
                                        "Transpose... (dialog)", "Transpose", btnSize);
    tbLayout->addWidget(transpUpBtn);

    auto* humanizeBtn = makeIconButton(toolbar, faFont, icons::fa::Drumpad,
                                        "Humanize...", "Humanize", btnSize);
    tbLayout->addWidget(humanizeBtn);

    auto* swingBtn = makeIconButton(toolbar, faFont, icons::fa::Metronome,
                                     "Swing...", "Swing", btnSize);
    tbLayout->addWidget(swingBtn);

    auto* sep3 = new QFrame(toolbar);
    sep3->setFrameShape(QFrame::VLine);
    sep3->setStyleSheet(QString("color: %1;").arg(theme.surfaceLight.name()));
    tbLayout->addWidget(sep3);

    // Musical typing + step record toggle buttons
    const QString toggleStyle = QString(
        "QPushButton { min-width: 26px; min-height: 26px; font-size: 14px; }"
        "QPushButton:checked { background: %1; border-color: %2; }"
        "QPushButton:hover { background: %3; }")
        .arg(theme.accent.darker(120).name(), theme.accent.name(), theme.surfaceLight.name());

    musicalTypingBtn_ = new QPushButton(toolbar);
    musicalTypingBtn_->setAccessibleName("Musical Typing");
    musicalTypingBtn_->setCheckable(true);
    musicalTypingBtn_->setFont(faFont);
    musicalTypingBtn_->setText(QString(icons::fa::Keyboard));
    musicalTypingBtn_->setToolTip("Musical Typing (AWSEDFTGYHUJK)");
    musicalTypingBtn_->setFixedSize(btnSize, btnSize);
    musicalTypingBtn_->setStyleSheet(toggleStyle);
    tbLayout->addWidget(musicalTypingBtn_);

    stepRecordBtn_ = new QPushButton(toolbar);
    stepRecordBtn_->setAccessibleName("Step Record");
    stepRecordBtn_->setCheckable(true);
    stepRecordBtn_->setFont(faFont);
    stepRecordBtn_->setText(QString(icons::fa::PunchIn));
    stepRecordBtn_->setToolTip("Step Record");
    stepRecordBtn_->setFixedSize(btnSize, btnSize);
    stepRecordBtn_->setStyleSheet(toggleStyle);
    tbLayout->addWidget(stepRecordBtn_);

    auto* sep4 = new QFrame(toolbar);
    sep4->setFrameShape(QFrame::VLine);
    sep4->setStyleSheet(QString("color: %1;").arg(theme.surfaceLight.name()));
    tbLayout->addWidget(sep4);

    // Snap
    auto* snapLabel = new QLabel("Snap:", toolbar);
    snapLabel->setStyleSheet(QString("color: %1; font-size: 10px;").arg(theme.textDim.name()));
    tbLayout->addWidget(snapLabel);

    snapCombo_ = new QComboBox(toolbar);
    snapCombo_->setAccessibleName("Snap Mode");
    snapCombo_->addItem("Off");
    snapCombo_->addItem("1/32");
    snapCombo_->addItem("1/16");
    snapCombo_->addItem("1/8");
    snapCombo_->addItem("1/4");
    snapCombo_->addItem("Bar");
    snapCombo_->setCurrentIndex(4);
    snapCombo_->setFixedWidth(70);
    connect(snapCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PianoRollEditor::onSnapModeChanged);
    tbLayout->addWidget(snapCombo_);

    // Zoom
    auto* zoomInBtn = makeIconButton(toolbar, faFont, icons::fa::ZoomIn,
                                      "Zoom In", "Zoom In", btnSize);
    tbLayout->addWidget(zoomInBtn);
    auto* zoomOutBtn = makeIconButton(toolbar, faFont, icons::fa::ZoomOut,
                                       "Zoom Out", "Zoom Out", btnSize);
    tbLayout->addWidget(zoomOutBtn);

    mainLayout->addWidget(toolbar);

    auto* bodyWidget = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Ruler row (spacer for keyboard width + ruler aligned with grid)
    auto* rulerRow = new QHBoxLayout();
    rulerRow->setContentsMargins(0, 0, 0, 0);
    rulerRow->setSpacing(0);

    keyboard_ = new PianoKeyboard(bodyWidget);

    auto* rulerSpacer = new QWidget(bodyWidget);
    rulerSpacer->setFixedWidth(keyboard_->sizeHint().width());
    rulerSpacer->setFixedHeight(24);
    rulerRow->addWidget(rulerSpacer);

    ruler_ = new PianoRollRuler(bodyWidget);
    rulerRow->addWidget(ruler_, 1);

    bodyLayout->addLayout(rulerRow);

    // Grid row (keyboard + note grid)
    auto* gridRow = new QHBoxLayout();
    gridRow->setContentsMargins(0, 0, 0, 0);
    gridRow->setSpacing(0);

    gridRow->addWidget(keyboard_);

    noteGrid_ = new NoteGrid(bodyWidget);
    gridRow->addWidget(noteGrid_, 1);

    bodyLayout->addLayout(gridRow, 1);

    // Velocity row
    auto* velRow = new QHBoxLayout();
    velRow->setContentsMargins(0, 0, 0, 0);
    velRow->setSpacing(0);

    auto* velSpacer = new QWidget(bodyWidget);
    velSpacer->setFixedWidth(keyboard_->sizeHint().width());
    velRow->addWidget(velSpacer);

    velocityLane_ = new VelocityLane(bodyWidget);
    velRow->addWidget(velocityLane_, 1);

    bodyLayout->addLayout(velRow);

    mainLayout->addWidget(bodyWidget, 1);

    // ── Connections ──

    connect(keyboard_, &PianoKeyboard::noteClicked, this, [this](int note) {
        noteGrid_->playNotePreview(note);
    });
    connect(keyboard_, &PianoKeyboard::noteReleased, this, [this](int note) {
        noteGrid_->stopNotePreview(note);
    });

    connect(noteGrid_, &NoteGrid::verticalScrollChanged,
            this, &PianoRollEditor::syncKeyboardScroll);
    connect(noteGrid_, &NoteGrid::horizontalScrollChanged,
            this, &PianoRollEditor::syncVelocityScroll);
    connect(noteGrid_, &NoteGrid::zoomChanged,
            this, &PianoRollEditor::syncVelocityScroll);
    connect(noteGrid_, &NoteGrid::notesChanged,
            this, &PianoRollEditor::onNotesChanged);
    connect(noteGrid_, &NoteGrid::editModeRequested,
            this, [this]() { editModeBtn_->setChecked(true); });
    connect(velocityLane_, &VelocityLane::velocityChanged,
            this, [this]() { noteGrid_->rebuildNotes(); });

    // Ruler sync
    connect(noteGrid_, &NoteGrid::horizontalScrollChanged, this, [this](int value) {
        ruler_->setScrollX(static_cast<double>(value));
    });
    connect(noteGrid_, &NoteGrid::zoomChanged, this, [this]() {
        ruler_->setPixelsPerBeat(noteGrid_->pixelsPerBeat());
    });
    connect(noteGrid_, &NoteGrid::typingCursorMoved, this, [this](double beat) {
        ruler_->setCursorBeat(beat);
    });
    connect(ruler_, &PianoRollRuler::cursorPositionClicked, this, [this](double beat) {
        noteGrid_->setTypingCursorBeat(beat);
    });
    connect(ruler_, &PianoRollRuler::cursorPositionDragged, this, [this](double beat) {
        noteGrid_->setTypingCursorBeat(beat);
    });
    ruler_->setPixelsPerBeat(noteGrid_->pixelsPerBeat());
    ruler_->setSnapFunction([this](double beat) { return noteGrid_->snapper().snapBeat(beat); });

    // Musical typing / step record sync
    connect(musicalTypingBtn_, &QPushButton::toggled, this, [this](bool checked) {
        noteGrid_->setMusicalTypingEnabled(checked);
    });
    connect(stepRecordBtn_, &QPushButton::toggled, this, [this](bool checked) {
        noteGrid_->setStepRecordEnabled(checked);
    });
    connect(noteGrid_, &NoteGrid::musicalTypingToggled, this, [this](bool enabled) {
        musicalTypingBtn_->setChecked(enabled);
    });
    connect(noteGrid_, &NoteGrid::stepRecordToggled, this, [this](bool enabled) {
        stepRecordBtn_->setChecked(enabled);
    });

    // Clipboard buttons
    connect(cutBtn, &QPushButton::clicked, this, [this]() { noteGrid_->cutSelectedNotes(); });
    connect(copyBtn, &QPushButton::clicked, this, [this]() { noteGrid_->copySelectedNotes(); });
    connect(pasteBtn, &QPushButton::clicked, this, [this]() { noteGrid_->pasteNotes(0.0); });
    connect(dupBtn, &QPushButton::clicked, this, [this]() { noteGrid_->duplicateSelectedNotes(); });

    // Transform buttons
    connect(quantizeBtn, &QPushButton::clicked, this, [this]() { noteGrid_->showQuantizeDialog(); });
    connect(legatoBtn, &QPushButton::clicked, this, [this]() { noteGrid_->legatoSelectedNotes(); });
    connect(reverseBtn, &QPushButton::clicked, this, [this]() { noteGrid_->reverseSelectedNotes(); });
    connect(transpUpBtn, &QPushButton::clicked, this, [this]() { noteGrid_->showTransposeDialog(); });
    connect(humanizeBtn, &QPushButton::clicked, this, [this]() { noteGrid_->showHumanizeDialog(); });
    connect(swingBtn, &QPushButton::clicked, this, [this]() { noteGrid_->showSwingDialog(); });

    // Zoom buttons
    connect(zoomInBtn, &QPushButton::clicked, this, [this]() {
        noteGrid_->setPixelsPerBeat(noteGrid_->pixelsPerBeat() * 1.3);
    });
    connect(zoomOutBtn, &QPushButton::clicked, this, [this]() {
        noteGrid_->setPixelsPerBeat(noteGrid_->pixelsPerBeat() / 1.3);
    });

    onSnapModeChanged(4);
    noteGrid_->setEditMode(NoteGrid::EditMode::Edit);
}

void PianoRollEditor::setClip(te::MidiClip* clip)
{
    clip_ = clip;
    noteGrid_->setClip(clip);
    velocityLane_->setClip(clip);
    velocityLane_->setPixelsPerBeat(noteGrid_->pixelsPerBeat());

    if (clip) {
        clipNameLabel_->setText(
            QString::fromStdString(clip->getName().toStdString()));
    } else {
        clipNameLabel_->setText("No clip selected");
    }
}

void PianoRollEditor::refresh()
{
    if (!clip_) return;
    noteGrid_->rebuildNotes();
    velocityLane_->setPixelsPerBeat(noteGrid_->pixelsPerBeat());
    velocityLane_->refresh();
    velocityLane_->repaint();
}

void PianoRollEditor::syncKeyboardScroll()
{
    int vScroll = noteGrid_->verticalScrollBar()->value();
    keyboard_->setScrollOffset(vScroll);
}

void PianoRollEditor::syncVelocityScroll()
{
    int hScroll = noteGrid_->horizontalScrollBar()->value();
    velocityLane_->setScrollOffset(hScroll);
    velocityLane_->setPixelsPerBeat(noteGrid_->pixelsPerBeat());
}

void PianoRollEditor::onSnapModeChanged(int index)
{
    static constexpr SnapMode mapping[] = {
        SnapMode::Off, SnapMode::EighthBeat, SnapMode::QuarterBeat,
        SnapMode::HalfBeat, SnapMode::Beat, SnapMode::Bar
    };
    if (index >= 0 && index < 6)
        noteGrid_->snapper().setMode(mapping[index]);
}

void PianoRollEditor::onDrawModeToggled(bool checked)
{
    if (!checked) {
        if (!editModeBtn_->isChecked())
            editModeBtn_->setChecked(true);
        return;
    }

    if (editModeBtn_->isChecked())
        editModeBtn_->setChecked(false);
    noteGrid_->setEditMode(NoteGrid::EditMode::Draw);
}

void PianoRollEditor::onEditModeToggled(bool checked)
{
    if (!checked) {
        if (!drawModeBtn_->isChecked())
            drawModeBtn_->setChecked(true);
        return;
    }

    if (drawModeBtn_->isChecked())
        drawModeBtn_->setChecked(false);
    noteGrid_->setEditMode(NoteGrid::EditMode::Edit);
}

void PianoRollEditor::onNotesChanged()
{
    velocityLane_->setPixelsPerBeat(noteGrid_->pixelsPerBeat());
    velocityLane_->setScrollOffset(noteGrid_->horizontalScrollBar()->value());
    velocityLane_->refresh();
    velocityLane_->repaint();
    emit notesChanged();
}

} // namespace freedaw
