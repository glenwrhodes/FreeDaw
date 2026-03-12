#include "MainWindow.h"
#include "utils/ThemeManager.h"
#include <QFileDialog>
#include <QAction>
#include <QLabel>
#include <QKeySequence>

namespace freedaw {

MainWindow::MainWindow(FreeDawApplication& app, QWidget* parent)
    : QMainWindow(parent), app_(app), editMgr_(app.editManager())
{
    setWindowTitle("FreeDaw");
    setAccessibleName("FreeDaw Main Window");
    resize(1280, 800);
    setMinimumSize(900, 600);

    applyGlobalStyle();

    // Transport bar at the top
    transportBar_ = new TransportBar(&editMgr_, this);
    setMenuWidget(nullptr);

    // Timeline as central widget
    timelineView_ = new TimelineView(&editMgr_, this);
    setCentralWidget(timelineView_);

    // Connect snap mode from transport to timeline
    connect(transportBar_, &TransportBar::snapModeRequested,
            this, [this](int mode) {
                timelineView_->snapper().setMode(static_cast<SnapMode>(mode));
            });

    createDocks();
    createMenus();
    createToolBar();
    createStatusBar();
}

MainWindow::~MainWindow() = default;

void MainWindow::applyGlobalStyle()
{
    auto& theme = ThemeManager::instance().current();

    QString globalSS = QString(
        "QMainWindow { background: %1; }"
        "QWidget { background: %1; color: %3; }"
        "QMenuBar { background: %2; color: %3; border-bottom: 1px solid %4; }"
        "QMenuBar::item { background: transparent; }"
        "QMenuBar::item:selected { background: %5; }"
        "QMenu { background: %2; color: %3; border: 1px solid %4; }"
        "QMenu::item:selected { background: %5; }"
        "QToolBar { background: %2; border: none; spacing: 4px; }"
        "QToolButton { background: transparent; color: %3; border: 1px solid transparent; "
        "  border-radius: 3px; padding: 3px 6px; }"
        "QToolButton:hover { background: %6; border: 1px solid %4; }"
        "QToolButton:pressed { background: %5; }"
        "QDockWidget { background: %1; color: %3; font-size: 11px; }"
        "QDockWidget::title { background: %2; padding: 4px; border: 1px solid %4; }"
        "QDockWidget > QWidget { background: %1; }"
        "QStatusBar { background: %2; color: %7; border-top: 1px solid %4; }"
        "QScrollArea { background: %1; border: none; }"
        "QScrollArea > QWidget > QWidget { background: %1; }"
        "QScrollBar:horizontal { background: %1; height: 10px; border: none; }"
        "QScrollBar::handle:horizontal { background: %4; border-radius: 4px; min-width: 20px; }"
        "QScrollBar:vertical { background: %1; width: 10px; border: none; }"
        "QScrollBar::handle:vertical { background: %4; border-radius: 4px; min-height: 20px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: %1; }"
        "QSpinBox, QDoubleSpinBox { background: %1; color: %3; border: 1px solid %4; "
        "  border-radius: 2px; padding: 2px 6px; min-height: 18px; }"
        "QSpinBox::up-button, QDoubleSpinBox::up-button { width: 16px; }"
        "QSpinBox::down-button, QDoubleSpinBox::down-button { width: 16px; }"
        "QComboBox { background: %1; color: %3; border: 1px solid %4; "
        "  border-radius: 2px; padding: 2px 6px; min-height: 18px; }"
        "QComboBox::drop-down { border: none; width: 18px; }"
        "QComboBox QAbstractItemView { background: %2; color: %3; selection-background-color: %5; }"
        "QLabel { background: transparent; }"
        "QFrame { background: %1; }"
        "QHeaderView { background: %2; color: %3; border: none; }"
        "QHeaderView::section { background: %2; color: %3; border: 1px solid %4; padding: 2px 4px; }"
        "QGraphicsView { background: %1; border: none; }"
        "QSplitter::handle { background: %4; }"
        "QPushButton { background: %2; color: %3; border: 1px solid %4; "
        "  border-radius: 3px; padding: 3px 8px; }"
        "QPushButton:hover { background: %6; }"
        "QPushButton:pressed { background: %5; }"
        "QLineEdit { background: %1; color: %3; border: 1px solid %4; "
        "  border-radius: 2px; padding: 2px; }"
    ).arg(
        theme.background.name(),  // %1
        theme.surface.name(),     // %2
        theme.text.name(),        // %3
        theme.border.name(),      // %4
        theme.accent.name(),      // %5
        theme.surfaceLight.name(),// %6
        theme.textDim.name()      // %7
    );

    setStyleSheet(globalSS);
}

void MainWindow::createMenus()
{
    auto* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File menu
    auto* fileMenu = menuBar->addMenu("&File");

    fileMenu->addAction("&New Project", QKeySequence::New, this,
        &MainWindow::onNewProject);

    fileMenu->addAction("&Open Project...", QKeySequence::Open, this,
        &MainWindow::onOpenProject);

    fileMenu->addAction("&Save", QKeySequence::Save, this,
        &MainWindow::onSaveProject);

    fileMenu->addAction("Save &As...", QKeySequence("Ctrl+Shift+S"), this,
        &MainWindow::onSaveProjectAs);

    fileMenu->addSeparator();
    fileMenu->addAction("&Quit", QKeySequence::Quit, this, &QMainWindow::close);

    // Edit menu
    auto* editMenu = menuBar->addMenu("&Edit");

    editMenu->addAction("Add Audio &Track", QKeySequence("Ctrl+T"), this,
        &MainWindow::onAddTrack);

    editMenu->addAction("&Remove Selected Track", this,
        &MainWindow::onRemoveTrack);

    // View menu
    auto* viewMenu = menuBar->addMenu("&View");

    viewMenu->addAction("Zoom &In", QKeySequence("Ctrl+="), timelineView_,
        &TimelineView::zoomIn);
    viewMenu->addAction("Zoom &Out", QKeySequence("Ctrl+-"), timelineView_,
        &TimelineView::zoomOut);
    viewMenu->addSeparator();
    viewMenu->addAction("Toggle &Mixer", this, [this]() {
        mixerDock_->setVisible(!mixerDock_->isVisible());
    });
    viewMenu->addAction("Toggle &Browser", this, [this]() {
        browserDock_->setVisible(!browserDock_->isVisible());
    });
    viewMenu->addAction("Toggle &Effects", this, [this]() {
        effectsDock_->setVisible(!effectsDock_->isVisible());
    });

    // Transport menu
    auto* transportMenu = menuBar->addMenu("&Transport");

    transportMenu->addAction("&Play / Pause", QKeySequence("Space"), this, [this]() {
        auto& t = editMgr_.transport();
        if (t.isPlaying()) t.stop(false, false);
        else t.play(false);
    });

    transportMenu->addAction("&Stop", this, [this]() {
        editMgr_.transport().stop(false, false);
        editMgr_.transport().setPosition(tracktion::TimePosition::fromSeconds(0));
    });

    transportMenu->addAction("&Record", QKeySequence("R"), this, [this]() {
        editMgr_.transport().record(false);
    });
}

void MainWindow::createToolBar()
{
    mainToolBar_ = addToolBar("Main");
    mainToolBar_->setAccessibleName("Main Toolbar");
    mainToolBar_->setMovable(false);
    mainToolBar_->setFloatable(false);
    mainToolBar_->setIconSize(QSize(20, 20));

    mainToolBar_->addWidget(transportBar_);
}

void MainWindow::createDocks()
{
    // Mixer dock (bottom)
    mixerDock_ = new QDockWidget("Mixer", this);
    mixerDock_->setAccessibleName("Mixer Dock");
    mixerView_ = new MixerView(&editMgr_, mixerDock_);
    mixerDock_->setWidget(mixerView_);
    addDockWidget(Qt::BottomDockWidgetArea, mixerDock_);

    connect(mixerView_, &MixerView::effectInsertRequested,
            this, &MainWindow::onEffectInsertRequested);

    // File browser dock (right, collapsible)
    browserDock_ = new QDockWidget("Browser", this);
    browserDock_->setAccessibleName("File Browser Dock");
    fileBrowser_ = new FileBrowserPanel(browserDock_);
    browserDock_->setWidget(fileBrowser_);
    addDockWidget(Qt::RightDockWidgetArea, browserDock_);

    // Effects dock (right, tabbed with browser)
    effectsDock_ = new QDockWidget("Effects", this);
    effectsDock_->setAccessibleName("Effects Dock");
    effectChain_ = new EffectChainWidget(&editMgr_, effectsDock_);
    effectsDock_->setWidget(effectChain_);
    addDockWidget(Qt::RightDockWidgetArea, effectsDock_);

    tabifyDockWidget(browserDock_, effectsDock_);
    browserDock_->raise();
}

void MainWindow::createStatusBar()
{
    auto* status = statusBar();
    auto& theme = ThemeManager::instance().current();

    auto* sampleRateLabel = new QLabel("44100 Hz", this);
    sampleRateLabel->setAccessibleName("Sample Rate");
    sampleRateLabel->setStyleSheet(
        QString("color: %1; font-size: 10px; padding: 0 8px;")
            .arg(theme.textDim.name()));

    auto* bufferLabel = new QLabel("512 samples", this);
    bufferLabel->setAccessibleName("Buffer Size");
    bufferLabel->setStyleSheet(sampleRateLabel->styleSheet());

    auto* cpuLabel = new QLabel("CPU: 0%", this);
    cpuLabel->setAccessibleName("CPU Usage");
    cpuLabel->setStyleSheet(sampleRateLabel->styleSheet());

    status->addPermanentWidget(sampleRateLabel);
    status->addPermanentWidget(bufferLabel);
    status->addPermanentWidget(cpuLabel);

    status->showMessage("Ready");
}

void MainWindow::onNewProject()
{
    editMgr_.newEdit();
}

void MainWindow::onOpenProject()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project", QString(),
        "Tracktion Edit (*.tracktionedit);;All Files (*)");
    if (path.isEmpty()) return;

    juce::File file(path.toStdString());
    editMgr_.loadEdit(file);
}

void MainWindow::onSaveProject()
{
    if (editMgr_.currentFile() == juce::File()) {
        onSaveProjectAs();
        return;
    }
    editMgr_.saveEdit();
}

void MainWindow::onSaveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project", QString(),
        "Tracktion Edit (*.tracktionedit);;All Files (*)");
    if (path.isEmpty()) return;

    juce::File file(path.toStdString());
    editMgr_.saveEditAs(file);
}

void MainWindow::onAddTrack()
{
    editMgr_.addAudioTrack();
}

void MainWindow::onRemoveTrack()
{
    auto tracks = editMgr_.getAudioTracks();
    if (tracks.size() > 1) {
        editMgr_.removeTrack(tracks.getLast());
    }
}

void MainWindow::onEffectInsertRequested(te::AudioTrack* track, int)
{
    if (!track) return;
    effectChain_->setTrack(track);
    effectsDock_->setVisible(true);
    effectsDock_->raise();
}

} // namespace freedaw
