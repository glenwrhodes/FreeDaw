#include "RoutingView.h"
#include "utils/ThemeManager.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QMenu>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <cmath>

namespace freedaw {

namespace {
const QColor kInputColor(76, 175, 80);
const QColor kTrackColor(66, 133, 244);
const QColor kBusColor(255, 152, 0);
const QColor kMasterColor(211, 47, 47);
const QColor kOutputColor(158, 158, 158);

const QColor kCablePalette[] = {
    QColor(229,  57,  53),   // red
    QColor( 76, 175,  80),   // green
    QColor(255, 179,   0),   // amber
    QColor( 66, 133, 244),   // blue
    QColor(171,  71, 188),   // purple
    QColor(  0, 188, 212),   // cyan
    QColor(255, 112,  67),   // deep orange
    QColor(124, 179,  66),   // light green
};
constexpr int kCablePaletteSize = 8;
}

RoutingView::RoutingView(EditManager* editMgr, QWidget* parent)
    : QWidget(parent), editMgr_(editMgr)
{
    setAccessibleName("Routing View");
    auto& theme = ThemeManager::instance().current();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    toolbar_ = new QToolBar(this);
    toolbar_->setAccessibleName("Routing Toolbar");
    toolbar_->setMovable(false);
    toolbar_->setIconSize(QSize(16, 16));

    auto* addBusBtn = new QPushButton("+ Add Bus", toolbar_);
    addBusBtn->setAccessibleName("Add Bus Track");
    addBusBtn->setStyleSheet(
        QString("QPushButton { background: %1; color: %2; border: 1px solid %3; "
                "border-radius: 3px; padding: 3px 10px; font-size: 11px; }"
                "QPushButton:hover { background: %4; }")
            .arg(kBusColor.darker(140).name(), theme.text.name(),
                 theme.border.name(), kBusColor.darker(120).name()));
    connect(addBusBtn, &QPushButton::clicked, this, [this]() {
        editMgr_->addBusTrack();
    });
    toolbar_->addWidget(addBusBtn);

    auto* autoLayoutBtn = new QPushButton("Auto Layout", toolbar_);
    autoLayoutBtn->setAccessibleName("Auto Layout");
    connect(autoLayoutBtn, &QPushButton::clicked, this, [this]() {
        layoutNodes();
        for (auto* c : cables_) c->updatePath();
        updateSceneRect();
    });
    toolbar_->addWidget(autoLayoutBtn);

    auto* zoomFitBtn = new QPushButton("Zoom to Fit", toolbar_);
    zoomFitBtn->setAccessibleName("Zoom to Fit");
    connect(zoomFitBtn, &QPushButton::clicked, this, &RoutingView::zoomToFit);
    toolbar_->addWidget(zoomFitBtn);

    layout->addWidget(toolbar_);

    scene_ = new QGraphicsScene(this);
    scene_->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene_->setBackgroundBrush(QBrush(theme.background.darker(110)));

    view_ = new QGraphicsView(scene_, this);
    view_->setAccessibleName("Routing Canvas");
    view_->setRenderHint(QPainter::Antialiasing);
    view_->setDragMode(QGraphicsView::RubberBandDrag);
    view_->setRubberBandSelectionMode(Qt::IntersectsItemBoundingRect);
    view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setFrameStyle(0);
    view_->setFocusPolicy(Qt::StrongFocus);
    view_->viewport()->installEventFilter(this);
    view_->installEventFilter(this);
    layout->addWidget(view_, 1);

    rebuildTimer_.setSingleShot(true);
    rebuildTimer_.setInterval(50);
    connect(&rebuildTimer_, &QTimer::timeout, this, &RoutingView::rebuild);

    sceneRectTimer_.setSingleShot(true);
    sceneRectTimer_.setInterval(0);
    connect(&sceneRectTimer_, &QTimer::timeout, this, &RoutingView::updateSceneRect);

    physicsTimer_.setInterval(16);
    connect(&physicsTimer_, &QTimer::timeout, this, &RoutingView::tickCablePhysics);

    connect(scene_, &QGraphicsScene::selectionChanged,
            this, &RoutingView::onSceneSelectionChanged);

    connect(editMgr_, &EditManager::tracksChanged, this, &RoutingView::scheduleRebuild);
    connect(editMgr_, &EditManager::routingChanged, this, &RoutingView::scheduleRebuild);
    connect(editMgr_, &EditManager::editChanged, this, &RoutingView::scheduleRebuild);
    connect(editMgr_, &EditManager::aboutToChangeEdit, this, [this]() { clearAll(); });

    rebuild();
}

RoutingView::~RoutingView()
{
    rebuildTimer_.stop();
    disconnect(scene_, nullptr, this, nullptr);
    disconnect(editMgr_, nullptr, this, nullptr);
}

void RoutingView::clearAll()
{
    saveNodePositions();
    dragCable_ = nullptr;
    dragSourceJack_ = nullptr;
    cables_.clear();
    inputNodes_.clear();
    trackNodes_.clear();
    busNodes_.clear();
    masterNode_ = nullptr;
    outputNodes_.clear();

    disconnect(scene_, &QGraphicsScene::selectionChanged,
               this, &RoutingView::onSceneSelectionChanged);
    scene_->clear();
    connect(scene_, &QGraphicsScene::selectionChanged,
            this, &RoutingView::onSceneSelectionChanged);
}

void RoutingView::flushNodePositions()
{
    saveNodePositions();
}

void RoutingView::scheduleRebuild()
{
    if (!rebuildTimer_.isActive())
        rebuildTimer_.start();
}

void RoutingView::rebuild()
{
    rebuildTimer_.stop();

    if (dragCable_ || panning_ || scene_->mouseGrabberItem()) {
        qDebug() << "[REBUILD] DEFERRED: dragCable=" << (quintptr)dragCable_
                 << "panning=" << panning_
                 << "grabber=" << (quintptr)scene_->mouseGrabberItem();
        rebuildTimer_.start(100);
        return;
    }

    qDebug() << "[REBUILD] executing full rebuild";
    clearAll();
    if (!editMgr_ || !editMgr_->edit()) return;
    buildNodes();
    layoutNodes();
    restoreNodePositions();
    buildCables();
    updateSceneRect();
    qDebug() << "[REBUILD] done: inputs=" << inputNodes_.size()
             << "tracks=" << trackNodes_.size()
             << "cables=" << cables_.size();
}

void RoutingView::updateAllCablePaths()
{
    for (auto* c : cables_)
        c->updatePath();
    startCablePhysics();
}

void RoutingView::startCablePhysics()
{
    if (!physicsTimer_.isActive())
        physicsTimer_.start();
}

void RoutingView::tickCablePhysics()
{
    constexpr qreal dt = 0.016;
    bool anyMoving = false;

    for (auto* c : cables_) {
        if (c->stepPhysics(dt))
            anyMoving = true;
    }
    if (dragCable_ && dragCable_->stepPhysics(dt))
        anyMoving = true;

    if (!anyMoving)
        physicsTimer_.stop();
}

void RoutingView::connectNodeSignals(RoutingNode* node)
{
    connect(node, &RoutingNode::nodeMoved, this, &RoutingView::updateAllCablePaths);
    connect(node, &RoutingNode::nodeMoved, this, [this]() {
        if (!sceneRectTimer_.isActive())
            sceneRectTimer_.start();
    });
    connect(node, &RoutingNode::renameRequested, this, [this, node](const QString& newName) {
        switch (node->nodeType()) {
            case NodeType::Track:
            case NodeType::Bus:
                if (node->track()) {
                    node->track()->setName(juce::String(newName.toStdString()));
                    emit editMgr_->tracksChanged();
                }
                break;
            case NodeType::InputSource:
                editMgr_->setInputDisplayName(node->deviceName(), newName);
                break;
            case NodeType::Master:
            case NodeType::OutputDevice:
                break;
        }
    });
}

QString RoutingView::nodeKey(RoutingNode* node) const
{
    switch (node->nodeType()) {
        case NodeType::InputSource:
            return "input:" + QString::fromStdString(node->deviceName().toStdString());
        case NodeType::Track:
        case NodeType::Bus:
            if (node->track())
                return "track:" + QString::number(node->track()->itemID.getRawID());
            break;
        case NodeType::Master:
            return "master";
        case NodeType::OutputDevice:
            return "output:" + node->name();
    }
    return {};
}

void RoutingView::saveNodePositions()
{
    auto saveList = [this](const std::vector<RoutingNode*>& nodes) {
        for (auto* node : nodes) {
            QString key = nodeKey(node);
            if (!key.isEmpty())
                savedPositions_[key] = node->pos();
        }
    };

    saveList(inputNodes_);
    saveList(trackNodes_);
    saveList(busNodes_);
    saveList(outputNodes_);
    if (masterNode_) {
        QString key = nodeKey(masterNode_);
        if (!key.isEmpty())
            savedPositions_[key] = masterNode_->pos();
    }

    editMgr_->saveRoutingLayout(savedPositions_);
}

void RoutingView::restoreNodePositions()
{
    if (savedPositions_.isEmpty())
        savedPositions_ = editMgr_->loadRoutingLayout();

    if (savedPositions_.isEmpty()) return;

    auto restoreList = [this](const std::vector<RoutingNode*>& nodes) {
        for (auto* node : nodes) {
            QString key = nodeKey(node);
            auto it = savedPositions_.find(key);
            if (it != savedPositions_.end())
                node->setPos(it.value());
        }
    };

    restoreList(inputNodes_);
    restoreList(trackNodes_);
    restoreList(busNodes_);
    restoreList(outputNodes_);
    if (masterNode_) {
        auto it = savedPositions_.find(nodeKey(masterNode_));
        if (it != savedPositions_.end())
            masterNode_->setPos(it.value());
    }
}

void RoutingView::buildNodes()
{
    auto sources = editMgr_->getAvailableInputSources();
    for (const auto& src : sources) {
        auto* node = new RoutingNode(NodeType::InputSource, src.displayName, kInputColor);
        node->setDeviceName(src.deviceName);
        node->setInputJackCount(0);
        node->setOutputJackCount(1);
        scene_->addItem(node);
        connectNodeSignals(node);
        inputNodes_.push_back(node);
    }

    auto tracks = editMgr_->getNonBusAudioTracks();
    for (auto* track : tracks) {
        QString name = QString::fromStdString(track->getName().toStdString());
        bool isMidi = editMgr_->isMidiTrack(track);
        QColor color = isMidi ? QColor(70, 100, 140) : kTrackColor;
        auto* node = new RoutingNode(NodeType::Track, name, color);
        node->setTrack(track);
        node->setEffectNames(getTrackEffectNames(track));

        bool hasSC = trackHasSidechainPlugin(track);
        node->setInputJackCount(hasSC ? 2 : 1);
        node->setOutputJackCount(1);
        if (hasSC)
            node->setInputJackLabels({"IN", "SC"});
        else
            node->setInputJackLabels({"IN"});
        node->setOutputJackLabels({"OUT"});

        if (editMgr_->isTrackOutputDisconnected(track))
            node->setWarning(true);

        scene_->addItem(node);
        connectNodeSignals(node);
        trackNodes_.push_back(node);
    }

    auto buses = editMgr_->getBusTracks();
    for (auto* bus : buses) {
        QString name = QString::fromStdString(bus->getName().toStdString());
        auto* node = new RoutingNode(NodeType::Bus, name, kBusColor);
        node->setTrack(bus);
        node->setEffectNames(getTrackEffectNames(bus));

        bool hasSC = trackHasSidechainPlugin(bus);
        node->setInputJackCount(hasSC ? 2 : 1);
        node->setOutputJackCount(1);
        if (hasSC)
            node->setInputJackLabels({"IN", "SC"});
        else
            node->setInputJackLabels({"IN"});
        node->setOutputJackLabels({"OUT"});

        scene_->addItem(node);
        connectNodeSignals(node);
        busNodes_.push_back(node);
    }

    masterNode_ = new RoutingNode(NodeType::Master, "Master", kMasterColor);
    masterNode_->setEffectNames(getMasterEffectNames());
    masterNode_->setInputJackCount(1);
    masterNode_->setOutputJackCount(1);
    scene_->addItem(masterNode_);
    connectNodeSignals(masterNode_);

    auto& dm = editMgr_->edit()->engine.getDeviceManager();
    for (int i = 0; i < dm.getNumWaveOutDevices(); ++i) {
        if (auto* dev = dm.getWaveOutDevice(i)) {
            QString name = QString::fromStdString(dev->getName().toStdString());
            auto* node = new RoutingNode(NodeType::OutputDevice, name, kOutputColor);
            node->setInputJackCount(1);
            node->setOutputJackCount(0);
            scene_->addItem(node);
            connectNodeSignals(node);
            outputNodes_.push_back(node);
        }
    }
}

void RoutingView::layoutNodes()
{
    auto placeColumn = [](std::vector<RoutingNode*>& nodes, qreal x, qreal startY, qreal spacing) {
        qreal y = startY;
        for (auto* node : nodes) {
            node->setPos(x, y);
            y += node->boundingRect().height() + spacing;
        }
    };

    placeColumn(inputNodes_, COL_INPUTS, ROW_START, ROW_SPACING);
    placeColumn(trackNodes_, COL_TRACKS, ROW_START, ROW_SPACING);
    placeColumn(busNodes_, COL_BUSES, ROW_START, ROW_SPACING);

    if (masterNode_) {
        qreal centerY = ROW_START;
        if (!trackNodes_.empty()) {
            qreal totalH = 0;
            for (auto* n : trackNodes_)
                totalH += n->boundingRect().height() + ROW_SPACING;
            centerY = ROW_START + (totalH - ROW_SPACING) / 2
                       - masterNode_->boundingRect().height() / 2;
            if (centerY < ROW_START) centerY = ROW_START;
        }
        masterNode_->setPos(COL_MASTER, centerY);
    }

    placeColumn(outputNodes_, COL_OUTPUTS, ROW_START, ROW_SPACING);
}

void RoutingView::buildCables()
{
    int colorIdx = 0;

    // Input source -> Track cables
    for (auto* tNode : trackNodes_) {
        if (!tNode->track()) continue;
        QString inputName = editMgr_->getTrackInputName(tNode->track());
        if (inputName.isEmpty()) continue;

        for (auto* iNode : inputNodes_) {
            if (iNode->deviceName().toStdString() ==
                juce::String(inputName.toStdString()).toStdString()) {
                auto* cable = new CableItem(
                    iNode->outputJack(0), tNode->inputJack(0),
                    cableColorForIndex(colorIdx++));
                scene_->addItem(cable);
                cables_.push_back(cable);
                break;
            }
        }
    }

    // Track -> Bus or Master cables
    for (auto* tNode : trackNodes_) {
        if (!tNode->track()) continue;
        auto* dest = editMgr_->getTrackOutputDestination(tNode->track());
        if (dest) {
            for (auto* bNode : busNodes_) {
                if (bNode->track() == dest) {
                    auto* cable = new CableItem(
                        tNode->outputJack(0), bNode->inputJack(0),
                        cableColorForIndex(colorIdx++));
                    scene_->addItem(cable);
                    cables_.push_back(cable);
                    break;
                }
            }
        } else if (masterNode_ && !editMgr_->isTrackOutputDisconnected(tNode->track())) {
            auto* cable = new CableItem(
                tNode->outputJack(0), masterNode_->inputJack(0),
                cableColorForIndex(colorIdx++));
            scene_->addItem(cable);
            cables_.push_back(cable);
        }
    }

    // Bus -> Master cables
    for (auto* bNode : busNodes_) {
        if (!bNode->track() || !masterNode_) continue;
        auto* dest = editMgr_->getTrackOutputDestination(bNode->track());
        if (!dest && !editMgr_->isTrackOutputDisconnected(bNode->track())) {
            auto* cable = new CableItem(
                bNode->outputJack(0), masterNode_->inputJack(0),
                cableColorForIndex(colorIdx++));
            scene_->addItem(cable);
            cables_.push_back(cable);
        }
    }

    // Sidechain cables (track/bus output -> another track/bus's SC jack)
    auto drawSidechainCables = [&](const std::vector<RoutingNode*>& nodes) {
        for (auto* node : nodes) {
            if (!node->track()) continue;
            for (auto* p : node->track()->pluginList.getPlugins()) {
                if (!p->canSidechain()) continue;
                auto scSourceID = p->getSidechainSourceID();
                if (!scSourceID.isValid()) continue;

                RoutingNode* sourceNode = nullptr;
                for (auto* tn : trackNodes_)
                    if (tn->track() && tn->track()->itemID == scSourceID)
                        sourceNode = tn;
                if (!sourceNode)
                    for (auto* bn : busNodes_)
                        if (bn->track() && bn->track()->itemID == scSourceID)
                            sourceNode = bn;

                if (sourceNode && node->inputJackCount() >= 2) {
                    QColor scColor(0, 188, 212);
                    auto* cable = new CableItem(
                        sourceNode->outputJack(0), node->inputJack(1), scColor);
                    scene_->addItem(cable);
                    cables_.push_back(cable);
                }
            }
        }
    };
    drawSidechainCables(trackNodes_);
    drawSidechainCables(busNodes_);

    // Master -> Output device cable
    if (masterNode_ && !outputNodes_.empty()) {
        auto* cable = new CableItem(
            masterNode_->outputJack(0), outputNodes_[0]->inputJack(0),
            kMasterColor.lighter(130));
        scene_->addItem(cable);
        cables_.push_back(cable);
    }
}

QStringList RoutingView::getTrackEffectNames(te::AudioTrack* track) const
{
    QStringList names;
    if (!track) return names;
    for (auto* p : track->pluginList.getPlugins()) {
        if (dynamic_cast<te::VolumeAndPanPlugin*>(p)) continue;
        if (dynamic_cast<te::LevelMeterPlugin*>(p)) continue;
        names.append(QString::fromStdString(p->getName().toStdString()));
    }
    return names;
}

QStringList RoutingView::getMasterEffectNames() const
{
    QStringList names;
    if (!editMgr_ || !editMgr_->edit()) return names;
    for (auto* p : editMgr_->edit()->getMasterPluginList().getPlugins()) {
        if (dynamic_cast<te::VolumeAndPanPlugin*>(p)) continue;
        if (dynamic_cast<te::LevelMeterPlugin*>(p)) continue;
        names.append(QString::fromStdString(p->getName().toStdString()));
    }
    return names;
}

bool RoutingView::trackHasSidechainPlugin(te::AudioTrack* track) const
{
    if (!track) return false;
    for (auto* p : track->pluginList.getPlugins())
        if (p->canSidechain()) return true;
    return false;
}

bool RoutingView::trackHasActiveSidechain(te::AudioTrack* track) const
{
    if (!track) return false;
    for (auto* p : track->pluginList.getPlugins())
        if (p->canSidechain() && p->getSidechainSourceID().isValid()) return true;
    return false;
}

te::Plugin* RoutingView::getFirstSidechainPlugin(te::AudioTrack* track) const
{
    if (!track) return nullptr;
    for (auto* p : track->pluginList.getPlugins())
        if (p->canSidechain()) return p;
    return nullptr;
}

QColor RoutingView::cableColorForIndex(int index) const
{
    return kCablePalette[index % kCablePaletteSize];
}

JackItem* RoutingView::findJackAt(const QPointF& scenePos, bool wantInput) const
{
    constexpr qreal SNAP_RADIUS = 20.0;
    JackItem* closest = nullptr;
    qreal closestDist = SNAP_RADIUS;

    auto check = [&](const std::vector<RoutingNode*>& nodes) {
        for (auto* node : nodes) {
            int count = wantInput ? node->inputJackCount() : node->outputJackCount();
            for (int i = 0; i < count; ++i) {
                auto* jack = wantInput ? node->inputJack(i) : node->outputJack(i);
                if (!jack) continue;
                QPointF jp = jack->sceneCenterPos();
                qreal dx = jp.x() - scenePos.x();
                qreal dy = jp.y() - scenePos.y();
                qreal dist = std::sqrt(dx * dx + dy * dy);
                if (dist < closestDist) {
                    closestDist = dist;
                    closest = jack;
                }
            }
        }
    };

    check(inputNodes_);
    check(trackNodes_);
    check(busNodes_);
    check(outputNodes_);

    if (masterNode_) {
        int count = wantInput ? masterNode_->inputJackCount() : masterNode_->outputJackCount();
        for (int i = 0; i < count; ++i) {
            auto* jack = wantInput ? masterNode_->inputJack(i) : masterNode_->outputJack(i);
            if (!jack) continue;
            QPointF jp = jack->sceneCenterPos();
            qreal dx = jp.x() - scenePos.x();
            qreal dy = jp.y() - scenePos.y();
            qreal dist = std::sqrt(dx * dx + dy * dy);
            if (dist < closestDist) {
                closestDist = dist;
                closest = jack;
            }
        }
    }

    return closest;
}

bool RoutingView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == view_->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            QPointF scenePos = view_->mapToScene(me->pos());

            // Right-click: cable context or empty-space context
            if (me->button() == Qt::RightButton) {
                for (auto* cable : cables_) {
                    if (cable->shape().contains(scenePos)) {
                        onCableRightClicked(cable, me->globalPosition().toPoint());
                        return true;
                    }
                }
                onEmptySpaceRightClicked(me->globalPosition().toPoint());
                return true;
            }

            // Middle-click: start panning
            if (me->button() == Qt::MiddleButton) {
                panning_ = true;
                panStartPos_ = me->pos();
                panStartHScroll_ = view_->horizontalScrollBar()->value();
                panStartVScroll_ = view_->verticalScrollBar()->value();
                view_->setCursor(Qt::ClosedHandCursor);
                return true;
            }

            // Left-click on output jack: start cable drag
            if (me->button() == Qt::LeftButton) {
                auto* jack = findJackAt(scenePos, false);
                if (jack) {
                    startCableDrag(jack, scenePos);
                    return true;
                }
            }
            // Everything else (node click, rubber band on empty space)
            // falls through to Qt's built-in handling
        }

        if (event->type() == QEvent::MouseMove) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (dragCable_) {
                updateCableDrag(view_->mapToScene(me->pos()));
                return true;
            }
            if (panning_) {
                QPoint delta = me->pos() - panStartPos_;
                view_->horizontalScrollBar()->setValue(panStartHScroll_ - delta.x());
                view_->verticalScrollBar()->setValue(panStartVScroll_ - delta.y());
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && dragCable_) {
                finishCableDrag(view_->mapToScene(me->pos()));
                return true;
            }
            if (me->button() == Qt::MiddleButton && panning_) {
                panning_ = false;
                view_->setCursor(Qt::ArrowCursor);
                return true;
            }
        }

        // Ctrl+scroll to zoom
        if (event->type() == QEvent::Wheel) {
            auto* we = static_cast<QWheelEvent*>(event);
            if (we->modifiers() & Qt::ControlModifier) {
                double factor = (we->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
                zoomBy(factor, we->position().toPoint());
                return true;
            }
        }
    }

    // Keyboard events come to the view itself (not viewport)
    if (watched == view_ && event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);

        if (ke->matches(QKeySequence::SelectAll)) {
            selectAllNodes();
            return true;
        }
        if (ke->key() == Qt::Key_Escape) {
            if (dragCable_) {
                cancelCableDrag();
            } else {
                deselectAllNodes();
            }
            return true;
        }
        if (ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace) {
            deleteSelectedBuses();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void RoutingView::startCableDrag(JackItem* fromJack, const QPointF& scenePos)
{
    dragSourceJack_ = fromJack;
    QColor c = fromJack->parentNode()->categoryColor();

    dragCable_ = new CableItem(fromJack, nullptr, c);
    dragCable_->setDangling(true);
    dragCable_->setDanglingEnd(scenePos);
    scene_->addItem(dragCable_);
}

void RoutingView::updateCableDrag(const QPointF& scenePos)
{
    if (!dragCable_) return;
    dragCable_->setDanglingEnd(scenePos);
    startCablePhysics();

    auto* target = findJackAt(scenePos, true);
    // Could highlight the target jack here in the future
    (void)target;
}

void RoutingView::finishCableDrag(const QPointF& scenePos)
{
    if (!dragCable_ || !dragSourceJack_) {
        cancelCableDrag();
        return;
    }

    auto* targetJack = findJackAt(scenePos, true);
    if (!targetJack || targetJack == dragSourceJack_) {
        cancelCableDrag();
        return;
    }

    auto* srcNode = dragSourceJack_->parentNode();
    auto* dstNode = targetJack->parentNode();

    if (srcNode == dstNode) {
        cancelCableDrag();
        return;
    }

    bool isSidechainJack = targetJack->isInput() && targetJack->jackIndex() == 1;

    // Sidechain connection: any output -> SC jack (index 1)
    if (isSidechainJack && srcNode->track() && dstNode->track()) {
        auto* scPlugin = getFirstSidechainPlugin(dstNode->track());
        if (scPlugin) {
            scPlugin->setSidechainSourceID(srcNode->track()->itemID);
            scPlugin->guessSidechainRouting();
            emit editMgr_->editChanged();
        }
    }
    // Input Source -> Track or Bus (jack 0)
    else if (srcNode->nodeType() == NodeType::InputSource
        && (dstNode->nodeType() == NodeType::Track || dstNode->nodeType() == NodeType::Bus)
        && dstNode->track()) {
        editMgr_->assignInputToTrack(*dstNode->track(), srcNode->deviceName());
    }
    // Track -> Bus
    else if (srcNode->nodeType() == NodeType::Track && srcNode->track()
             && dstNode->nodeType() == NodeType::Bus && dstNode->track()) {
        editMgr_->setTrackOutputToTrack(*srcNode->track(), *dstNode->track());
    }
    // Bus -> Bus
    else if (srcNode->nodeType() == NodeType::Bus && srcNode->track()
             && dstNode->nodeType() == NodeType::Bus && dstNode->track()
             && srcNode->track() != dstNode->track()) {
        editMgr_->setTrackOutputToTrack(*srcNode->track(), *dstNode->track());
    }
    // Track -> Master
    else if (srcNode->nodeType() == NodeType::Track && srcNode->track()
             && dstNode->nodeType() == NodeType::Master) {
        editMgr_->setTrackOutputToMaster(*srcNode->track());
    }
    // Bus -> Master
    else if (srcNode->nodeType() == NodeType::Bus && srcNode->track()
             && dstNode->nodeType() == NodeType::Master) {
        editMgr_->setTrackOutputToMaster(*srcNode->track());
    }

    cancelCableDrag();
}

void RoutingView::cancelCableDrag()
{
    if (dragCable_) {
        scene_->removeItem(dragCable_);
        delete dragCable_;
        dragCable_ = nullptr;
    }
    dragSourceJack_ = nullptr;
}

void RoutingView::onCableRightClicked(CableItem* cable, const QPoint& screenPos)
{
    auto srcType = cable->sourceJack()->parentNode()->nodeType();
    auto dstType = cable->destJack()->parentNode()->nodeType();
    te::AudioTrack* srcTrack = cable->sourceJack()->parentNode()->track();
    te::AudioTrack* dstTrack = cable->destJack()->parentNode()->track();
    bool isSidechainCable = cable->destJack()->isInput() && cable->destJack()->jackIndex() == 1;

    QMenu menu;
    menu.setAccessibleName("Cable Context Menu");

    if (isSidechainCable && dstTrack) {
        menu.addAction("Remove Sidechain Connection", [this, dstTrack]() {
            auto* scPlugin = getFirstSidechainPlugin(dstTrack);
            if (scPlugin) {
                scPlugin->setSidechainSourceID({});
                emit editMgr_->editChanged();
            }
        });
    }
    else if (srcType == NodeType::InputSource
             && (dstType == NodeType::Track || dstType == NodeType::Bus) && dstTrack) {
        menu.addAction("Remove Input Connection", [this, dstTrack]() {
            editMgr_->clearTrackInput(*dstTrack);
        });
    }
    else if (srcType == NodeType::Track && srcTrack && dstType == NodeType::Bus) {
        menu.addAction("Route to Master Instead", [this, srcTrack]() {
            editMgr_->setTrackOutputToMaster(*srcTrack);
        });
    }
    else if (srcType == NodeType::Bus && srcTrack && dstType == NodeType::Bus) {
        menu.addAction("Route to Master Instead", [this, srcTrack]() {
            editMgr_->setTrackOutputToMaster(*srcTrack);
        });
    }
    else if (srcType == NodeType::Track && srcTrack && dstType == NodeType::Master) {
        menu.addAction("Disconnect from Master", [this, srcTrack]() {
            editMgr_->clearTrackOutput(*srcTrack);
        });
    }
    else if (srcType == NodeType::Bus && srcTrack && dstType == NodeType::Master) {
        menu.addAction("Disconnect from Master", [this, srcTrack]() {
            editMgr_->clearTrackOutput(*srcTrack);
        });
    }
    else if (srcType == NodeType::Master && dstType == NodeType::OutputDevice) {
        auto* a = menu.addAction("Master Output (cannot remove)");
        a->setEnabled(false);
    }

    menu.exec(screenPos);
}

void RoutingView::onEmptySpaceRightClicked(const QPoint& screenPos)
{
    QMenu menu;
    menu.setAccessibleName("Routing Canvas Menu");

    menu.addAction("Add Bus Track", [this]() {
        editMgr_->addBusTrack();
    });

    menu.addSeparator();

    menu.addAction("Select All", [this]() { selectAllNodes(); });
    menu.addAction("Deselect All", [this]() { deselectAllNodes(); });

    menu.addSeparator();

    menu.addAction("Auto Layout", [this]() {
        layoutNodes();
        updateAllCablePaths();
        updateSceneRect();
    });

    menu.addAction("Zoom to Fit", [this]() {
        zoomToFit();
    });

    menu.exec(screenPos);
}

void RoutingView::selectAllNodes()
{
    for (auto* item : scene_->items()) {
        if (auto* node = dynamic_cast<RoutingNode*>(item))
            node->setSelected(true);
    }
}

void RoutingView::deselectAllNodes()
{
    scene_->clearSelection();
}

void RoutingView::deleteSelectedBuses()
{
    std::vector<te::AudioTrack*> toDelete;
    for (auto* item : scene_->selectedItems()) {
        if (auto* node = dynamic_cast<RoutingNode*>(item)) {
            if (node->nodeType() == NodeType::Bus && node->track())
                toDelete.push_back(node->track());
        }
    }
    for (auto* track : toDelete)
        editMgr_->removeTrack(track);
}

void RoutingView::onSceneSelectionChanged()
{
    te::AudioTrack* found = nullptr;
    int trackBusCount = 0;
    bool masterNodeSelected = false;
    for (auto* item : scene_->selectedItems()) {
        if (auto* node = dynamic_cast<RoutingNode*>(item)) {
            if ((node->nodeType() == NodeType::Track ||
                 node->nodeType() == NodeType::Bus) && node->track()) {
                found = node->track();
                if (++trackBusCount > 1) break;
            }
            if (node->nodeType() == NodeType::Master)
                masterNodeSelected = true;
        }
    }
    if (trackBusCount == 1 && found)
        emit trackSelected(found);
    else if (trackBusCount == 0 && masterNodeSelected)
        emit masterSelected();
}

void RoutingView::zoomBy(double factor, const QPoint& viewAnchor)
{
    QPointF sceneAnchor = view_->mapToScene(viewAnchor);
    view_->scale(factor, factor);
    QPointF newAnchor = view_->mapFromScene(sceneAnchor);
    QPointF delta = QPointF(viewAnchor) - newAnchor;
    view_->horizontalScrollBar()->setValue(
        view_->horizontalScrollBar()->value() - static_cast<int>(delta.x()));
    view_->verticalScrollBar()->setValue(
        view_->verticalScrollBar()->value() - static_cast<int>(delta.y()));
}

void RoutingView::updateSceneRect()
{
    constexpr qreal PADDING = 800.0;

    QRectF itemsRect;
    auto allNodes = {&inputNodes_, &trackNodes_, &busNodes_, &outputNodes_};
    for (auto* list : allNodes)
        for (auto* n : *list)
            itemsRect = itemsRect.united(n->sceneBoundingRect());
    if (masterNode_)
        itemsRect = itemsRect.united(masterNode_->sceneBoundingRect());

    if (itemsRect.isNull())
        itemsRect = QRectF(0, 0, 1000, 600);

    scene_->setSceneRect(itemsRect.adjusted(-PADDING, -PADDING, PADDING, PADDING));
}

void RoutingView::zoomToFit()
{
    QRectF itemsRect;
    auto allNodes = {&inputNodes_, &trackNodes_, &busNodes_, &outputNodes_};
    for (auto* list : allNodes)
        for (auto* n : *list)
            itemsRect = itemsRect.united(n->sceneBoundingRect());
    if (masterNode_)
        itemsRect = itemsRect.united(masterNode_->sceneBoundingRect());

    if (itemsRect.isNull()) return;

    constexpr qreal FIT_MARGIN = 40.0;
    itemsRect.adjust(-FIT_MARGIN, -FIT_MARGIN, FIT_MARGIN, FIT_MARGIN);
    view_->fitInView(itemsRect, Qt::KeepAspectRatio);
    updateSceneRect();
}

} // namespace freedaw
