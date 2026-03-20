#pragma once

#include "RoutingNode.h"
#include "CableItem.h"
#include "engine/EditManager.h"
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QToolBar>
#include <QPushButton>
#include <QTimer>
#include <QMap>
#include <vector>

namespace freedaw {

class RoutingView : public QWidget {
    Q_OBJECT

public:
    explicit RoutingView(EditManager* editMgr, QWidget* parent = nullptr);
    ~RoutingView() override;

signals:
    void trackSelected(te::AudioTrack* track);

public slots:
    void rebuild();
    void scheduleRebuild();
    void flushNodePositions();

private:
    void onSceneSelectionChanged();
    void buildNodes();
    void buildCables();
    void layoutNodes();
    void clearAll();
    void updateAllCablePaths();
    void connectNodeSignals(RoutingNode* node);
    void saveNodePositions();
    void restoreNodePositions();
    QString nodeKey(RoutingNode* node) const;

    QStringList getTrackEffectNames(te::AudioTrack* track) const;
    QStringList getMasterEffectNames() const;
    bool trackHasSidechainPlugin(te::AudioTrack* track) const;
    bool trackHasActiveSidechain(te::AudioTrack* track) const;
    te::Plugin* getFirstSidechainPlugin(te::AudioTrack* track) const;

    QColor cableColorForIndex(int index) const;
    JackItem* findJackAt(const QPointF& scenePos, bool wantInput) const;

    bool eventFilter(QObject* watched, QEvent* event) override;

    void startCableDrag(JackItem* fromJack, const QPointF& scenePos);
    void updateCableDrag(const QPointF& scenePos);
    void finishCableDrag(const QPointF& scenePos);
    void cancelCableDrag();

    void onCableRightClicked(CableItem* cable, const QPoint& screenPos);
    void onEmptySpaceRightClicked(const QPoint& screenPos);
    void selectAllNodes();
    void deselectAllNodes();
    void deleteSelectedBuses();
    void zoomBy(double factor, const QPoint& viewAnchor);
    void updateSceneRect();
    void zoomToFit();
    void startCablePhysics();
    void tickCablePhysics();

    EditManager* editMgr_;
    QGraphicsScene* scene_;
    QGraphicsView* view_;
    QToolBar* toolbar_;

    std::vector<RoutingNode*> inputNodes_;
    std::vector<RoutingNode*> trackNodes_;
    std::vector<RoutingNode*> busNodes_;
    RoutingNode* masterNode_ = nullptr;
    std::vector<RoutingNode*> outputNodes_;
    std::vector<CableItem*> cables_;

    CableItem* dragCable_ = nullptr;
    JackItem* dragSourceJack_ = nullptr;

    bool panning_ = false;
    QPoint panStartPos_;
    int panStartHScroll_ = 0;
    int panStartVScroll_ = 0;

    QTimer rebuildTimer_;
    QTimer sceneRectTimer_;
    QTimer physicsTimer_;
    QMap<QString, QPointF> savedPositions_;

    static constexpr qreal COL_INPUTS = 30.0;
    static constexpr qreal COL_TRACKS = 230.0;
    static constexpr qreal COL_BUSES  = 430.0;
    static constexpr qreal COL_MASTER = 630.0;
    static constexpr qreal COL_OUTPUTS = 830.0;
    static constexpr qreal ROW_START = 20.0;
    static constexpr qreal ROW_SPACING = 20.0;
};

} // namespace freedaw
