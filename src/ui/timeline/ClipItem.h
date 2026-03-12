#pragma once

#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <tracktion_engine/tracktion_engine.h>
#include <vector>

namespace te = tracktion::engine;

namespace freedaw {

class GridSnapper;
class EditManager;

class ClipItem : public QGraphicsRectItem {
public:
    ClipItem(te::Clip* clip, int trackIndex, double pixelsPerBeat,
             double trackHeight, QGraphicsItem* parent = nullptr);

    te::Clip* clip() const { return clip_; }
    int trackIndex() const { return trackIndex_; }
    void setTrackIndex(int idx) { trackIndex_ = idx; }

    void setDragContext(GridSnapper* snapper, EditManager* editMgr,
                        double* pixelsPerBeatPtr, double* trackHeightPtr,
                        int trackCount);
    void updateGeometry(double pixelsPerBeat, double trackHeight, double scrollY);
    void loadWaveform(int numPoints);

    enum { Type = QGraphicsItem::UserType + 1 };
    int type() const override { return Type; }

protected:
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    te::Clip* clip_ = nullptr;
    int trackIndex_ = 0;

    std::vector<float> waveMin_;
    std::vector<float> waveMax_;

    bool dragging_ = false;
    QPointF dragStartScene_;
    double dragStartBeat_ = 0;
    int    dragStartTrack_ = 0;

    GridSnapper* snapper_ = nullptr;
    EditManager* editMgr_ = nullptr;
    double* pixelsPerBeatPtr_ = nullptr;
    double* trackHeightPtr_ = nullptr;
    int     trackCount_ = 1;
};

} // namespace freedaw
