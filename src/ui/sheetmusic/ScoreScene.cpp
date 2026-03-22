#include "ui/sheetmusic/ScoreScene.h"
#include "utils/IconFont.h"
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QStyleOptionGraphicsItem>
#include <cmath>

namespace OpenDaw {

// ── SMuFL codepoints (Bravura) ──────────────────────────────────────────────

namespace smufl {
    const QChar trebleClef     {0xE050};
    const QChar bassClef       {0xE062};
    const QChar noteheadWhole  {0xE0A2};
    const QChar noteheadHalf   {0xE0A3};
    const QChar noteheadFilled {0xE0A4};
    const QChar restWhole      {0xE4E3};
    const QChar restHalf       {0xE4E4};
    const QChar restQuarter    {0xE4E5};
    const QChar restEighth     {0xE4E6};
    const QChar rest16th       {0xE4E7};
    const QChar flag8thUp      {0xE240};
    const QChar flag8thDown    {0xE241};
    const QChar flag16thUp     {0xE242};
    const QChar flag16thDown   {0xE243};
    const QChar accSharp       {0xE262};
    const QChar accFlat        {0xE260};
    const QChar accNatural     {0xE261};
    const QChar timeSig0       {0xE080};
}

// ── Colors (black-on-white sheet music) ─────────────────────────────────────

static const QColor kPaperColor     {255, 255, 252};
static const QColor kStaffLineColor { 60,  60,  60};
static const QColor kBarLineColor   { 50,  50,  50};
static const QColor kNoteColor      { 20,  20,  20};
static const QColor kRestColor      { 40,  40,  40};
static const QColor kMeasNumColor   {130, 130, 130};
static const QColor kBraceColor     { 50,  50,  50};

// SMuFL em-square = 4 staff spaces, so font pixel size = 4 * staffLineSpacing
// At staffLineSpacing=10, glyph font size = 40. Notehead width ≈ 1.18 ss = 11.8px.
// We center the notehead on the beat x position.
static constexpr double kNoteHeadHalfW = 6.0;

// ── ScoreSheetItem (renders all notation as a scene item) ───────────────────

class ScoreSheetItem : public QGraphicsItem {
public:
    explicit ScoreSheetItem(ScoreScene* scene)
        : scene_(scene)
    {
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, false);
    }

    QRectF boundingRect() const override
    {
        return scene_->sceneRect();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        scene_->paintScore(painter);
    }

private:
    ScoreScene* scene_;
};

// ── ScoreScene ──────────────────────────────────────────────────────────────

ScoreScene::ScoreScene(QObject* parent)
    : QGraphicsScene(parent)
{
    musicFont_ = icons::bravuraMusic(40);
    textFont_ = QFont("Segoe UI", 9);
    setBackgroundBrush(QColor(220, 220, 215));
}

void ScoreScene::clearScore()
{
    QGraphicsScene::clear();
    hasScore_ = false;
    model_.clear();
    setSceneRect(0, 0, 0, 0);
}

void ScoreScene::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat_ = ppb;
    if (hasScore_) {
        computeMetrics();
        QGraphicsScene::clear();
        setSceneRect(0, 0, totalWidth_, totalHeight_);
        auto* item = new ScoreSheetItem(this);
        addItem(item);
    }
}

void ScoreScene::renderScore(const NotationModel& model)
{
    QGraphicsScene::clear();
    model_ = model;
    hasScore_ = true;

    computeMetrics();
    setSceneRect(0, 0, totalWidth_, totalHeight_);

    auto* item = new ScoreSheetItem(this);
    addItem(item);
}

void ScoreScene::computeMetrics()
{
    met_.staffLineSpacing = 10.0;
    met_.trebleTopY = 60.0;
    met_.staffGap = 50.0;
    met_.bassTopY = met_.trebleTopY + 4 * met_.staffLineSpacing + met_.staffGap;

    // Horizontal layout: brace | clef | key sig | time sig | first note
    met_.braceX = 20.0;
    met_.clefX = met_.braceX + 22.0;
    met_.keySigX = met_.clefX + 38.0;

    met_.keySig = model_.keySig();
    int numKeySigAccidentals = std::abs(met_.keySig);
    double keySigWidth = numKeySigAccidentals * 10.0;
    double keySigPad = (numKeySigAccidentals > 0) ? 8.0 : 0.0;

    met_.timeSigX = met_.keySigX + keySigWidth + keySigPad;
    met_.leftMargin = met_.timeSigX + 32.0;

    met_.timeSigNum = model_.timeSigNum();
    met_.timeSigDen = model_.timeSigDen();

    double beatsPerMeasure = met_.timeSigNum * (4.0 / met_.timeSigDen);
    double measureWidth = beatsPerMeasure * pixelsPerBeat_;
    int numMeasures = std::max(1, model_.measureCount());
    totalWidth_ = met_.leftMargin + numMeasures * measureWidth + 40.0;
    totalHeight_ = met_.bassTopY + 4 * met_.staffLineSpacing + 80.0;
    met_.measureStartX = met_.leftMargin;
}

int ScoreScene::glyphFontSize() const
{
    return static_cast<int>(4.0 * met_.staffLineSpacing);
}

double ScoreScene::staffLineY(StaffKind staff, int line) const
{
    double topY = (staff == StaffKind::Treble) ? met_.trebleTopY : met_.bassTopY;
    return topY + line * met_.staffLineSpacing;
}

double ScoreScene::staffPositionY(StaffKind staff, int pos) const
{
    double bottomY = staffLineY(staff, 4);
    return bottomY - pos * (met_.staffLineSpacing / 2.0);
}

double ScoreScene::beatToX(int measure, double beatInMeasure) const
{
    double beatsPerMeasure = met_.timeSigNum * (4.0 / met_.timeSigDen);
    double measureWidth = beatsPerMeasure * pixelsPerBeat_;
    return met_.measureStartX + measure * measureWidth + beatInMeasure * pixelsPerBeat_ + 20.0;
}

// ── main paint entry point ──────────────────────────────────────────────────

void ScoreScene::paintScore(QPainter* painter)
{
    if (!hasScore_) return;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    double beatsPerMeasure = met_.timeSigNum * (4.0 / met_.timeSigDen);
    double measureWidth = beatsPerMeasure * pixelsPerBeat_;
    int numMeasures = model_.measureCount();
    double scoreEndX = met_.measureStartX + numMeasures * measureWidth;

    drawPaper(painter);
    drawStaffLines(painter, met_.braceX + 10.0, scoreEndX);
    drawBrace(painter, met_.braceX);
    drawBarLine(painter, met_.braceX + 10.0);
    drawClefs(painter, met_.clefX);
    drawKeySig(painter, met_.keySigX);
    drawTimeSig(painter, met_.timeSigX);

    for (int m = 0; m <= numMeasures; ++m) {
        double x = met_.measureStartX + m * measureWidth;
        drawBarLine(painter, x);
        if (m < numMeasures)
            drawMeasureNumber(painter, m + 1, x);
    }

    auto& measures = model_.measures();
    for (int m = 0; m < numMeasures; ++m) {
        auto& meas = measures[m];

        for (auto& evt : meas.trebleEvents)
            drawEvent(painter, evt, m, StaffKind::Treble);
        for (auto& evt : meas.bassEvents)
            drawEvent(painter, evt, m, StaffKind::Bass);

        for (auto& bg : meas.trebleBeams)
            drawBeamGroup(painter, bg, meas.trebleEvents, m);
        for (auto& bg : meas.bassBeams)
            drawBeamGroup(painter, bg, meas.bassEvents, m);
    }
}

// ── paper background ────────────────────────────────────────────────────────

void ScoreScene::drawPaper(QPainter* p)
{
    p->setPen(Qt::NoPen);
    p->setBrush(kPaperColor);
    p->drawRect(QRectF(0, 0, totalWidth_, totalHeight_));
    p->setBrush(Qt::NoBrush);
}

// ── staff structure ─────────────────────────────────────────────────────────

void ScoreScene::drawStaffLines(QPainter* p, double startX, double endX)
{
    QPen pen(kStaffLineColor, 1.0);
    p->setPen(pen);

    for (int staff = 0; staff < 2; ++staff) {
        StaffKind sk = (staff == 0) ? StaffKind::Treble : StaffKind::Bass;
        for (int line = 0; line < 5; ++line) {
            double y = staffLineY(sk, line);
            p->drawLine(QPointF(startX, y), QPointF(endX, y));
        }
    }
}

void ScoreScene::drawBrace(QPainter* p, double x)
{
    double topY = staffLineY(StaffKind::Treble, 0);
    double botY = staffLineY(StaffKind::Bass, 4);
    double braceH = botY - topY;

    QFont braceFont("Times New Roman", 10);
    braceFont.setPixelSize(static_cast<int>(braceH * 1.1));

    p->save();
    p->setPen(kBraceColor);
    p->setFont(braceFont);

    QFontMetricsF fm(braceFont);
    QRectF glyphBounds = fm.tightBoundingRect("{");

    double scaleY = braceH / glyphBounds.height();
    double scaleX = scaleY * 0.55;

    double centerY = (topY + botY) / 2.0;
    double glyphCenterY = glyphBounds.center().y();

    p->translate(x, centerY);
    p->scale(scaleX, scaleY);
    p->translate(0, -glyphCenterY);

    p->drawText(QPointF(-glyphBounds.width(), 0), "{");

    p->restore();
}

void ScoreScene::drawClefs(QPainter* p, double x)
{
    p->setPen(kNoteColor);
    QFont clefFont = musicFont_;
    clefFont.setPixelSize(glyphFontSize());
    p->setFont(clefFont);

    // treble clef origin sits on G4 line (line index 3)
    double trebleClefY = staffLineY(StaffKind::Treble, 3);
    p->drawText(QPointF(x, trebleClefY), QString(smufl::trebleClef));

    // bass clef origin sits on F3 line (line index 1)
    double bassClefY = staffLineY(StaffKind::Bass, 1);
    p->drawText(QPointF(x, bassClefY), QString(smufl::bassClef));
}

void ScoreScene::drawKeySig(QPainter* p, double x)
{
    if (met_.keySig == 0) return;

    p->setPen(kNoteColor);
    QFont ksFont = musicFont_;
    ksFont.setPixelSize(glyphFontSize());
    p->setFont(ksFont);

    bool sharps = met_.keySig > 0;
    int count = std::abs(met_.keySig);
    QChar glyph = sharps ? smufl::accSharp : smufl::accFlat;

    static const int trebleSharps[7] = {8, 5, 9, 6, 3, 7, 4};
    static const int bassSharps[7]   = {6, 3, 7, 4, 1, 5, 2};
    static const int trebleFlats[7]  = {4, 7, 3, 6, 2, 5, 1};
    static const int bassFlats[7]    = {2, 5, 1, 4, 0, 3, 6};

    const int* treblePos = sharps ? trebleSharps : trebleFlats;
    const int* bassPos   = sharps ? bassSharps   : bassFlats;

    double spacing = 10.0;
    for (int i = 0; i < count; i++) {
        double xPos = x + i * spacing;
        double ty = staffPositionY(StaffKind::Treble, treblePos[i]);
        p->drawText(QPointF(xPos, ty), QString(glyph));

        double by = staffPositionY(StaffKind::Bass, bassPos[i]);
        p->drawText(QPointF(xPos, by), QString(glyph));
    }
}

void ScoreScene::drawTimeSig(QPainter* p, double x)
{
    p->setPen(kNoteColor);
    QFont tsFont = musicFont_;
    tsFont.setPixelSize(glyphFontSize());
    p->setFont(tsFont);

    auto drawDigits = [&](int value, StaffKind staff, int baseLine) {
        QString digits = QString::number(value);
        double y = staffLineY(staff, baseLine);
        double xOff = x;
        for (QChar ch : digits) {
            int digit = ch.digitValue();
            if (digit >= 0 && digit <= 9) {
                QChar glyph(smufl::timeSig0.unicode() + digit);
                p->drawText(QPointF(xOff, y), QString(glyph));
                xOff += 14.0;
            }
        }
    };

    drawDigits(met_.timeSigNum, StaffKind::Treble, 1);
    drawDigits(met_.timeSigDen, StaffKind::Treble, 3);
    drawDigits(met_.timeSigNum, StaffKind::Bass, 1);
    drawDigits(met_.timeSigDen, StaffKind::Bass, 3);
}

void ScoreScene::drawBarLine(QPainter* p, double x)
{
    double topY = staffLineY(StaffKind::Treble, 0);
    double botY = staffLineY(StaffKind::Bass, 4);
    p->setPen(QPen(kBarLineColor, 1.2));
    p->drawLine(QPointF(x, topY), QPointF(x, botY));
}

void ScoreScene::drawMeasureNumber(QPainter* p, int num, double x)
{
    p->setPen(kMeasNumColor);
    p->setFont(textFont_);
    double y = staffLineY(StaffKind::Treble, 0) - 8.0;
    p->drawText(QPointF(x + 4, y), QString::number(num));
}

// ── event rendering ─────────────────────────────────────────────────────────

void ScoreScene::drawEvent(QPainter* p, const NotationEvent& evt,
                           int measureIdx, StaffKind staff)
{
    double x = beatToX(measureIdx, evt.beatInMeasure);

    if (evt.isRest) {
        drawRest(p, evt.value, evt.dotted, x, staff);
        return;
    }

    // draw note heads, ledger lines, accidentals, dots
    for (auto& note : evt.notes) {
        double y = staffPositionY(note.staff, note.staffPosition);

        drawLedgerLines(p, note.staff, note.staffPosition, x);

        if (note.accidental != Accidental::None)
            drawAccidental(p, note.accidental, x - kNoteHeadHalfW - 12.0, y);

        drawNoteHead(p, note, x, y);

        if (note.dotted)
            drawDot(p, x + kNoteHeadHalfW + 4.0, y, note.staffPosition);
    }

    // stem + flag: skip for beamed notes (beam group draws its own stems)
    if (evt.beamed) return;

    if (!evt.notes.empty() && evt.value != NoteValue::Whole) {
        double topNoteY = 1e9, botNoteY = -1e9;
        int avgPos = 0;
        for (auto& n : evt.notes) {
            double ny = staffPositionY(n.staff, n.staffPosition);
            topNoteY = std::min(topNoteY, ny);
            botNoteY = std::max(botNoteY, ny);
            avgPos += n.staffPosition;
        }
        avgPos /= static_cast<int>(evt.notes.size());
        int stemDir = stemDirectionForPosition(avgPos);

        double stemX = (stemDir > 0) ? x + kNoteHeadHalfW : x - kNoteHeadHalfW;
        double minExtension = 3.0 * met_.staffLineSpacing;
        // stem goes from the far note, extending minExtension past the near note
        double stemFromY, stemToY;
        if (stemDir > 0) {
            stemFromY = botNoteY;
            stemToY = std::min(topNoteY, botNoteY) - minExtension;
        } else {
            stemFromY = topNoteY;
            stemToY = std::max(botNoteY, topNoteY) + minExtension;
        }

        p->setPen(QPen(kNoteColor, 1.2));
        p->drawLine(QPointF(stemX, stemFromY), QPointF(stemX, stemToY));

        if (evt.value == NoteValue::Eighth || evt.value == NoteValue::Sixteenth)
            drawFlag(p, stemX, stemToY, stemDir, evt.value);
    }
}

void ScoreScene::drawNoteHead(QPainter* p, const NotationNote& note,
                               double x, double y)
{
    p->setPen(kNoteColor);
    QFont nhFont = musicFont_;
    nhFont.setPixelSize(glyphFontSize());
    p->setFont(nhFont);

    QChar glyph;
    switch (note.value) {
    case NoteValue::Whole:     glyph = smufl::noteheadWhole; break;
    case NoteValue::Half:      glyph = smufl::noteheadHalf; break;
    default:                   glyph = smufl::noteheadFilled; break;
    }

    p->drawText(QPointF(x - kNoteHeadHalfW, y), QString(glyph));
}

void ScoreScene::drawStem(QPainter* p, double x, double noteY,
                           int stemDir, NoteValue value, StaffKind /*staff*/)
{
    if (value == NoteValue::Whole) return;

    double stemLen = 3.0 * met_.staffLineSpacing;
    double stemX = (stemDir > 0) ? x + kNoteHeadHalfW : x - kNoteHeadHalfW;
    double stemToY = noteY - stemDir * stemLen;

    p->setPen(QPen(kNoteColor, 1.2));
    p->drawLine(QPointF(stemX, noteY), QPointF(stemX, stemToY));
}

void ScoreScene::drawFlag(QPainter* p, double x, double stemTopY,
                           int stemDir, NoteValue value)
{
    p->setPen(kNoteColor);
    QFont flagFont = musicFont_;
    flagFont.setPixelSize(glyphFontSize());
    p->setFont(flagFont);

    QChar glyph;
    if (value == NoteValue::Eighth)
        glyph = (stemDir > 0) ? smufl::flag8thUp : smufl::flag8thDown;
    else
        glyph = (stemDir > 0) ? smufl::flag16thUp : smufl::flag16thDown;

    p->drawText(QPointF(x - 2, stemTopY), QString(glyph));
}

void ScoreScene::drawAccidental(QPainter* p, Accidental acc, double x, double y)
{
    if (acc == Accidental::None) return;

    p->setPen(kNoteColor);
    QFont accFont = musicFont_;
    accFont.setPixelSize(glyphFontSize());
    p->setFont(accFont);

    QChar glyph;
    switch (acc) {
    case Accidental::Sharp:   glyph = smufl::accSharp; break;
    case Accidental::Flat:    glyph = smufl::accFlat; break;
    case Accidental::Natural: glyph = smufl::accNatural; break;
    default: return;
    }

    p->drawText(QPointF(x, y), QString(glyph));
}

void ScoreScene::drawLedgerLines(QPainter* p, StaffKind staff, int staffPos, double x)
{
    QPen pen(kStaffLineColor, 1.0);
    p->setPen(pen);

    double halfW = kNoteHeadHalfW + 3.0;

    if (staffPos < 0) {
        for (int pos = -2; pos >= staffPos; pos -= 2) {
            double y = staffPositionY(staff, pos);
            p->drawLine(QPointF(x - halfW, y), QPointF(x + halfW, y));
        }
    }

    if (staffPos > 8) {
        for (int pos = 10; pos <= staffPos; pos += 2) {
            double y = staffPositionY(staff, pos);
            p->drawLine(QPointF(x - halfW, y), QPointF(x + halfW, y));
        }
    }
}

void ScoreScene::drawDot(QPainter* p, double x, double y, int staffPos)
{
    double dotY = y;
    if (staffPos % 2 == 0)
        dotY -= met_.staffLineSpacing / 4.0;

    p->setPen(Qt::NoPen);
    p->setBrush(kNoteColor);
    p->drawEllipse(QPointF(x, dotY), 2.0, 2.0);
    p->setBrush(Qt::NoBrush);
}

void ScoreScene::drawRest(QPainter* p, NoteValue value, bool dotted,
                           double x, StaffKind staff)
{
    p->setPen(kRestColor);
    QFont restFont = musicFont_;
    restFont.setPixelSize(glyphFontSize());
    p->setFont(restFont);

    QChar glyph;
    int baseLine = 2;
    switch (value) {
    case NoteValue::Whole:     glyph = smufl::restWhole;   baseLine = 1; break;
    case NoteValue::Half:      glyph = smufl::restHalf;    baseLine = 2; break;
    case NoteValue::Quarter:   glyph = smufl::restQuarter; baseLine = 2; break;
    case NoteValue::Eighth:    glyph = smufl::restEighth;  baseLine = 2; break;
    case NoteValue::Sixteenth: glyph = smufl::rest16th;    baseLine = 2; break;
    }

    double y = staffLineY(staff, baseLine);
    p->drawText(QPointF(x - 4, y), QString(glyph));

    if (dotted)
        drawDot(p, x + 12.0, y, 3);
}

// ── beams ───────────────────────────────────────────────────────────────────

void ScoreScene::drawBeamGroup(QPainter* p, const BeamGroup& bg,
                                const std::vector<NotationEvent>& events,
                                int measureIdx)
{
    if (bg.eventIndices.size() < 2) return;

    int totalPos = 0;
    int noteCount = 0;
    for (int idx : bg.eventIndices) {
        auto& evt = events[idx];
        for (auto& n : evt.notes) {
            totalPos += n.staffPosition;
            noteCount++;
        }
    }
    int stemDir = (noteCount > 0 && totalPos / noteCount >= 4) ? -1 : 1;

    double minExtension = 3.0 * met_.staffLineSpacing;
    double beamThickness = 4.0;

    // First pass: collect note positions and stem X for each event
    struct StemInfo { double stemX; double stemStartY; };
    std::vector<StemInfo> stems;

    for (int idx : bg.eventIndices) {
        auto& evt = events[idx];
        double x = beatToX(measureIdx, evt.beatInMeasure);

        double topNoteY = 1e9, botNoteY = -1e9;
        for (auto& n : evt.notes) {
            double ny = staffPositionY(n.staff, n.staffPosition);
            topNoteY = std::min(topNoteY, ny);
            botNoteY = std::max(botNoteY, ny);
        }

        double stemX = (stemDir > 0) ? x + kNoteHeadHalfW : x - kNoteHeadHalfW;
        double stemStartY = (stemDir > 0) ? botNoteY : topNoteY;
        stems.push_back({stemX, stemStartY});
    }

    // Second pass: compute a single flat beam Y that every stem can reach.
    // The beam must be at least minExtension away from every note's stem-start.
    double beamY;
    if (stemDir > 0) {
        beamY = 1e9;
        for (auto& s : stems)
            beamY = std::min(beamY, s.stemStartY - minExtension);
    } else {
        beamY = -1e9;
        for (auto& s : stems)
            beamY = std::max(beamY, s.stemStartY + minExtension);
    }

    // Draw stems -- all reaching to beamY
    p->setPen(QPen(kNoteColor, 1.2));
    for (auto& s : stems)
        p->drawLine(QPointF(s.stemX, s.stemStartY), QPointF(s.stemX, beamY));

    // Primary beam (flat, at beamY)
    p->setPen(Qt::NoPen);
    p->setBrush(kNoteColor);

    double beamLeft = stems.front().stemX;
    double beamRight = stems.back().stemX;

    QPolygonF beam;
    beam << QPointF(beamLeft,  beamY)
         << QPointF(beamRight, beamY)
         << QPointF(beamRight, beamY + stemDir * beamThickness)
         << QPointF(beamLeft,  beamY + stemDir * beamThickness);
    p->drawPolygon(beam);

    // Secondary beam for sixteenth notes
    bool allSixteenths = true;
    for (int idx : bg.eventIndices) {
        if (events[idx].value != NoteValue::Sixteenth) {
            allSixteenths = false;
            break;
        }
    }
    if (allSixteenths && stems.size() >= 2) {
        double offset = stemDir * (beamThickness + 3.0);
        QPolygonF beam2;
        beam2 << QPointF(beamLeft,  beamY + offset)
              << QPointF(beamRight, beamY + offset)
              << QPointF(beamRight, beamY + offset + stemDir * beamThickness)
              << QPointF(beamLeft,  beamY + offset + stemDir * beamThickness);
        p->drawPolygon(beam2);
    }

    p->setBrush(Qt::NoBrush);
}

} // namespace OpenDaw
