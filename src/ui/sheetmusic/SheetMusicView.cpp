#include "ui/sheetmusic/SheetMusicView.h"
#include "engine/EditManager.h"
#include "utils/IconFont.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QToolBar>
#include <QGraphicsView>
#include <QScrollBar>

namespace OpenDaw {

SheetMusicView::SheetMusicView(QWidget* parent)
    : QWidget(parent)
{
    setAccessibleName("Sheet Music View");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    buildToolbar();
    layout->addWidget(toolbar_);

    scene_ = new ScoreScene(this);
    view_ = new QGraphicsView(scene_, this);
    view_->setAccessibleName("Sheet Music Score Area");
    view_->setRenderHint(QPainter::Antialiasing, true);
    view_->setRenderHint(QPainter::TextAntialiasing, true);
    view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view_->setDragMode(QGraphicsView::ScrollHandDrag);
    view_->setBackgroundBrush(QColor(200, 200, 195));

    layout->addWidget(view_);
}

void SheetMusicView::buildToolbar()
{
    toolbar_ = new QToolBar(this);
    toolbar_->setAccessibleName("Sheet Music Toolbar");
    toolbar_->setMovable(false);
    toolbar_->setIconSize(QSize(18, 18));

    auto* label = new QLabel(" Sheet Music ", toolbar_);
    label->setStyleSheet("color: #ccc; font-weight: bold; padding: 0 8px;");
    toolbar_->addWidget(label);

    clipNameLabel_ = new QLabel(toolbar_);
    clipNameLabel_->setAccessibleName("Sheet Music Clip Name");
    clipNameLabel_->setStyleSheet("color: #999; padding: 0 12px;");
    toolbar_->addWidget(clipNameLabel_);

    toolbar_->addSeparator();

    auto* keyLabel = new QLabel("Key:", toolbar_);
    keyLabel->setStyleSheet("color: #aaa; padding: 0 4px;");
    toolbar_->addWidget(keyLabel);

    keySigCombo_ = new QComboBox(toolbar_);
    keySigCombo_->setAccessibleName("Key Signature");

    struct KeyEntry { int val; const char* name; };
    static const KeyEntry keys[] = {
        {-7, "C\u266D maj / A\u266D min (7\u266D)"},
        {-6, "G\u266D maj / E\u266D min (6\u266D)"},
        {-5, "D\u266D maj / B\u266D min (5\u266D)"},
        {-4, "A\u266D maj / F min (4\u266D)"},
        {-3, "E\u266D maj / C min (3\u266D)"},
        {-2, "B\u266D maj / G min (2\u266D)"},
        {-1, "F maj / D min (1\u266D)"},
        { 0, "C maj / A min"},
        { 1, "G maj / E min (1\u266F)"},
        { 2, "D maj / B min (2\u266F)"},
        { 3, "A maj / F\u266F min (3\u266F)"},
        { 4, "E maj / C\u266F min (4\u266F)"},
        { 5, "B maj / G\u266F min (5\u266F)"},
        { 6, "F\u266F maj / D\u266F min (6\u266F)"},
        { 7, "C\u266F maj / A\u266F min (7\u266F)"},
    };
    for (auto& k : keys)
        keySigCombo_->addItem(QString::fromUtf8(k.name), k.val);
    keySigCombo_->setCurrentIndex(7);

    connect(keySigCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        keySig_ = keySigCombo_->currentData().toInt();
        rebuildScore();
    });

    toolbar_->addWidget(keySigCombo_);

    auto* spacer = new QWidget(toolbar_);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar_->addWidget(spacer);

    auto* zoomLabel = new QLabel("Zoom:", toolbar_);
    zoomLabel->setStyleSheet("color: #aaa; padding: 0 4px;");
    toolbar_->addWidget(zoomLabel);

    zoomSlider_ = new QSlider(Qt::Horizontal, toolbar_);
    zoomSlider_->setAccessibleName("Sheet Music Zoom");
    zoomSlider_->setRange(20, 150);
    zoomSlider_->setValue(50);
    zoomSlider_->setFixedWidth(120);
    toolbar_->addWidget(zoomSlider_);

    connect(zoomSlider_, &QSlider::valueChanged, this, &SheetMusicView::onZoomChanged);
}

void SheetMusicView::setClip(te::MidiClip* clip, EditManager* editMgr)
{
    clip_ = clip;
    editMgr_ = editMgr;
    rebuildScore();
}

void SheetMusicView::refresh()
{
    rebuildScore();
}

void SheetMusicView::rebuildScore()
{
    if (!clip_ || !editMgr_) {
        clipNameLabel_->setText("(no clip selected)");
        scene_->clearScore();
        return;
    }

    clipNameLabel_->setText(QString::fromStdString(clip_->getName().toStdString()));

    NotationModel model;
    model.buildFromClip(clip_,
                        editMgr_->getTimeSigNumerator(),
                        editMgr_->getTimeSigDenominator(),
                        keySig_);

    scene_->setPixelsPerBeat(zoomSlider_->value());
    scene_->renderScore(model);

    view_->viewport()->update();
}

void SheetMusicView::onZoomChanged(int value)
{
    scene_->setPixelsPerBeat(value);
    if (clip_ && editMgr_)
        rebuildScore();
}

} // namespace OpenDaw
