#pragma once

#include "NotationModel.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QFont>
#include <QPen>

namespace OpenDaw {

class ScoreScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit ScoreScene(QObject* parent = nullptr);

    void renderScore(const NotationModel& model);
    void clearScore();

    void setPixelsPerBeat(double ppb);
    double pixelsPerBeat() const { return pixelsPerBeat_; }

    void paintScore(QPainter* p);

private:
    struct StaffMetrics {
        double staffLineSpacing = 10.0;
        double trebleTopY = 40.0;
        double bassTopY = 0.0;
        double staffGap = 50.0;
        double braceX = 20.0;
        double clefX = 42.0;
        double keySigX = 80.0;
        double timeSigX = 80.0;
        double leftMargin = 112.0;
        double measureStartX = 0.0;
        int timeSigNum = 4;
        int timeSigDen = 4;
        int keySig = 0;
    };

    void computeMetrics();
    int glyphFontSize() const;
    double staffLineY(StaffKind staff, int line) const;
    double staffPositionY(StaffKind staff, int pos) const;
    double beatToX(int measure, double beatInMeasure) const;

    void drawPaper(QPainter* p);
    void drawStaffLines(QPainter* p, double startX, double endX);
    void drawBrace(QPainter* p, double x);
    void drawClefs(QPainter* p, double x);
    void drawKeySig(QPainter* p, double x);
    void drawTimeSig(QPainter* p, double x);
    void drawBarLine(QPainter* p, double x);
    void drawMeasureNumber(QPainter* p, int num, double x);

    void drawEvent(QPainter* p, const NotationEvent& evt,
                   int measureIdx, StaffKind staff);
    void drawNoteHead(QPainter* p, const NotationNote& note,
                      double x, double y);
    void drawStem(QPainter* p, double x, double noteY,
                  int stemDir, NoteValue value, StaffKind staff);
    void drawFlag(QPainter* p, double x, double stemTopY, int stemDir, NoteValue value);
    void drawAccidental(QPainter* p, Accidental acc, double x, double y);
    void drawLedgerLines(QPainter* p, StaffKind staff, int staffPos, double x);
    void drawDot(QPainter* p, double x, double y, int staffPos);
    void drawRest(QPainter* p, NoteValue value, bool dotted,
                  double x, StaffKind staff);
    void drawBeamGroup(QPainter* p, const BeamGroup& bg,
                       const std::vector<NotationEvent>& events,
                       int measureIdx);

    StaffMetrics met_;
    double pixelsPerBeat_ = 50.0;
    double totalWidth_ = 800.0;
    double totalHeight_ = 300.0;

    QFont musicFont_;
    QFont textFont_;

    NotationModel model_;
    bool hasScore_ = false;
};

} // namespace OpenDaw
