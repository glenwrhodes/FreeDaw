#include "PianoRollEditor.h"
#include "utils/IconFont.h"
#include "utils/ThemeManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QPushButton>

namespace freedaw {

PianoRollEditor::PianoRollEditor(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Piano Roll Editor");
    auto& theme = ThemeManager::instance().current();

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(34);
    toolbar->setAutoFillBackground(true);
    QPalette tbPal;
    tbPal.setColor(QPalette::Window, theme.surface);
    toolbar->setPalette(tbPal);

    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(6, 3, 6, 3);
    tbLayout->setSpacing(8);

    clipNameLabel_ = new QLabel("No clip selected", toolbar);
    clipNameLabel_->setAccessibleName("Clip Name");
    clipNameLabel_->setStyleSheet(
        QString("color: %1; font-weight: bold; font-size: 11px;")
            .arg(theme.text.name()));
    tbLayout->addWidget(clipNameLabel_);

    tbLayout->addStretch();

    const int modeIconSize = 16;
    const int modeButtonSize = 24;
    const auto modeFont = icons::fontAudio(modeIconSize);
    const QString modeButtonStyle = QString(
        "QPushButton { min-width: 24px; min-height: 24px; font-size: 12px; }"
        "QPushButton:checked { background: %1; border-color: %2; }")
        .arg(theme.surfaceLight.name(), theme.accent.name());

    editModeBtn_ = new QPushButton(toolbar);
    editModeBtn_->setAccessibleName("Edit Mode");
    editModeBtn_->setCheckable(true);
    editModeBtn_->setChecked(true);
    editModeBtn_->setFont(modeFont);
    editModeBtn_->setText(QString(icons::fa::Pointer));
    editModeBtn_->setToolTip("Edit mode");
    editModeBtn_->setFixedSize(modeButtonSize, modeButtonSize);
    editModeBtn_->setStyleSheet(modeButtonStyle);
    tbLayout->addWidget(editModeBtn_);

    drawModeBtn_ = new QPushButton(toolbar);
    drawModeBtn_->setAccessibleName("Draw Mode");
    drawModeBtn_->setCheckable(true);
    drawModeBtn_->setChecked(false);
    drawModeBtn_->setFont(modeFont);
    drawModeBtn_->setText(QString(icons::fa::Pen));
    drawModeBtn_->setToolTip("Draw mode");
    drawModeBtn_->setFixedSize(modeButtonSize, modeButtonSize);
    drawModeBtn_->setStyleSheet(modeButtonStyle);
    tbLayout->addWidget(drawModeBtn_);

    connect(drawModeBtn_, &QPushButton::toggled,
            this, &PianoRollEditor::onDrawModeToggled);
    connect(editModeBtn_, &QPushButton::toggled,
            this, &PianoRollEditor::onEditModeToggled);

    auto* snapLabel = new QLabel("Snap:", toolbar);
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
            this, &PianoRollEditor::onSnapModeChanged);
    tbLayout->addWidget(snapCombo_);

    auto* quantizeBtn = new QPushButton("Quantize", toolbar);
    quantizeBtn->setAccessibleName("Quantize Notes");
    connect(quantizeBtn, &QPushButton::clicked, this, [this]() {
        noteGrid_->quantizeNotes();
    });
    tbLayout->addWidget(quantizeBtn);

    tbLayout->addSpacing(8);

    auto* zoomInBtn = new QPushButton("+", toolbar);
    zoomInBtn->setAccessibleName("Zoom In");
    zoomInBtn->setFixedSize(24, 24);
    connect(zoomInBtn, &QPushButton::clicked, this, [this]() {
        noteGrid_->setPixelsPerBeat(noteGrid_->pixelsPerBeat() * 1.3);
    });
    tbLayout->addWidget(zoomInBtn);

    auto* zoomOutBtn = new QPushButton("-", toolbar);
    zoomOutBtn->setAccessibleName("Zoom Out");
    zoomOutBtn->setFixedSize(24, 24);
    connect(zoomOutBtn, &QPushButton::clicked, this, [this]() {
        noteGrid_->setPixelsPerBeat(noteGrid_->pixelsPerBeat() / 1.3);
    });
    tbLayout->addWidget(zoomOutBtn);

    mainLayout->addWidget(toolbar);

    auto* bodyWidget = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    auto* gridRow = new QHBoxLayout();
    gridRow->setContentsMargins(0, 0, 0, 0);
    gridRow->setSpacing(0);

    keyboard_ = new PianoKeyboard(bodyWidget);
    gridRow->addWidget(keyboard_);

    noteGrid_ = new NoteGrid(bodyWidget);
    gridRow->addWidget(noteGrid_, 1);

    bodyLayout->addLayout(gridRow, 1);

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

    connect(noteGrid_, &NoteGrid::verticalScrollChanged,
            this, &PianoRollEditor::syncKeyboardScroll);
    connect(noteGrid_, &NoteGrid::horizontalScrollChanged,
            this, &PianoRollEditor::syncVelocityScroll);
    connect(noteGrid_, &NoteGrid::zoomChanged,
            this, &PianoRollEditor::syncVelocityScroll);
    connect(noteGrid_, &NoteGrid::notesChanged,
            this, &PianoRollEditor::onNotesChanged);
    connect(velocityLane_, &VelocityLane::velocityChanged,
            this, [this]() { noteGrid_->rebuildNotes(); });

    onSnapModeChanged(3);
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
    auto mode = static_cast<SnapMode>(index);
    noteGrid_->snapper().setMode(mode);
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
