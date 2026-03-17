#include "PianoRollEditor.h"
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
    toolbar->setFixedHeight(30);
    toolbar->setAutoFillBackground(true);
    QPalette tbPal;
    tbPal.setColor(QPalette::Window, theme.surface);
    toolbar->setPalette(tbPal);

    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(6, 2, 6, 2);
    tbLayout->setSpacing(8);

    clipNameLabel_ = new QLabel("No clip selected", toolbar);
    clipNameLabel_->setAccessibleName("Clip Name");
    clipNameLabel_->setStyleSheet(
        QString("color: %1; font-weight: bold; font-size: 11px;")
            .arg(theme.text.name()));
    tbLayout->addWidget(clipNameLabel_);

    tbLayout->addStretch();

    auto* modeLabel = new QLabel("Mode:", toolbar);
    modeLabel->setStyleSheet(QString("color: %1;").arg(theme.textDim.name()));
    tbLayout->addWidget(modeLabel);

    modeCombo_ = new QComboBox(toolbar);
    modeCombo_->setAccessibleName("Piano Roll Edit Mode");
    modeCombo_->addItem("Draw");
    modeCombo_->addItem("Edit");
    modeCombo_->setCurrentIndex(1);
    modeCombo_->setFixedWidth(90);
    connect(modeCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PianoRollEditor::onEditModeChanged);
    tbLayout->addWidget(modeCombo_);

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
    onEditModeChanged(1);
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

void PianoRollEditor::onEditModeChanged(int index)
{
    auto mode = index == 0 ? NoteGrid::EditMode::Draw : NoteGrid::EditMode::Edit;
    noteGrid_->setEditMode(mode);
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
