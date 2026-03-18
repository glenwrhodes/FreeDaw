#include "RoutingNode.h"
#include "utils/ThemeManager.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QLineEdit>
#include <cmath>

namespace freedaw {

// ── JackItem ─────────────────────────────────────────────────────────────────

JackItem::JackItem(bool isInput, int index, RoutingNode* parentNode)
    : QGraphicsEllipseItem(parentNode)
    , isInput_(isInput), index_(index), parentNode_(parentNode)
{
    const qreal d = RoutingNode::JACK_OUTER_DIAMETER;
    setRect(-d / 2, -d / 2, d, d);
    setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::CrossCursor));
    setZValue(2);
}

QPointF JackItem::sceneCenterPos() const
{
    return mapToScene(0, 0);
}

void JackItem::setConnected(bool c, const QColor& cableColor)
{
    connected_ = c;
    cableColor_ = cableColor;
    update();
}

void JackItem::setHighlighted(bool h)
{
    highlighted_ = h;
    update();
}

void JackItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);
    const qreal outer = RoutingNode::JACK_OUTER_DIAMETER;
    const qreal inner = RoutingNode::JACK_INNER_DIAMETER;

    QColor ringColor = parentNode_->categoryColor();
    if (highlighted_)
        ringColor = ringColor.lighter(160);

    QColor fillColor = ringColor.darker(130);

    QPen ringPen(ringColor, 2.0);
    painter->setPen(ringPen);
    painter->setBrush(QBrush(fillColor));
    painter->drawEllipse(QRectF(-outer / 2, -outer / 2, outer, outer));

    QColor holeColor(15, 15, 15);
    if (connected_)
        holeColor = cableColor_.darker(200);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(holeColor));
    painter->drawEllipse(QRectF(-inner / 2, -inner / 2, inner, inner));

    if (highlighted_) {
        QPen glowPen(ringColor.lighter(180), 1.0);
        glowPen.setCosmetic(true);
        painter->setPen(glowPen);
        painter->setBrush(Qt::NoBrush);
        const qreal g = outer / 2 + 3;
        painter->drawEllipse(QRectF(-g, -g, g * 2, g * 2));
    }
}

// ── RoutingNode ──────────────────────────────────────────────────────────────

RoutingNode::RoutingNode(NodeType type, const QString& name, const QColor& color,
                         QGraphicsItem* parent)
    : QGraphicsObject(parent), type_(type), name_(name), color_(color)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setZValue(1);
    setCursor(Qt::OpenHandCursor);
}

QVariant RoutingNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene()) {
        static bool groupMoveGuard = false;
        if (!groupMoveGuard && isSelected()) {
            groupMoveGuard = true;
            QPointF delta = value.toPointF() - pos();
            for (auto* item : scene()->selectedItems()) {
                if (item != this) {
                    if (auto* node = dynamic_cast<RoutingNode*>(item))
                        node->moveBy(delta.x(), delta.y());
                }
            }
            groupMoveGuard = false;
        }
    }
    if (change == ItemPositionHasChanged)
        emit nodeMoved();
    if (change == ItemSelectedHasChanged)
        update();
    return QGraphicsObject::itemChange(change, value);
}

void RoutingNode::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (type_ == NodeType::OutputDevice) {
        QGraphicsObject::mouseDoubleClickEvent(event);
        return;
    }

    if (event->pos().y() > HEADER_HEIGHT) {
        QGraphicsObject::mouseDoubleClickEvent(event);
        return;
    }

    auto* edit = new QLineEdit();
    edit->setText(name_);
    edit->setAccessibleName("Rename Node");
    edit->selectAll();
    edit->setFixedWidth(static_cast<int>(NODE_WIDTH - 4));
    edit->setFixedHeight(static_cast<int>(HEADER_HEIGHT - 2));
    edit->setStyleSheet(
        "QLineEdit { background: #222; color: #eee; border: 1px solid #888; "
        "font-size: 11px; font-weight: bold; padding: 0 4px; }");

    auto* proxy = new QGraphicsProxyWidget(this);
    proxy->setWidget(edit);
    proxy->setPos(2, 1);
    proxy->setZValue(10);
    edit->setFocus();

    connect(edit, &QLineEdit::editingFinished, this, [this, proxy, edit]() {
        QString newName = edit->text().trimmed();
        if (!newName.isEmpty() && newName != name_) {
            name_ = newName;
            emit renameRequested(newName);
            update();
        }
        proxy->deleteLater();
    });

    event->accept();
}

qreal RoutingNode::bodyHeight() const
{
    qreal h = HEADER_HEIGHT;
    h += std::max(1, static_cast<int>(effectNames_.size())) * EFFECT_ROW_HEIGHT;
    h += JACK_AREA_HEIGHT;
    return h;
}

QRectF RoutingNode::boundingRect() const
{
    constexpr qreal margin = 10.0;
    return QRectF(-margin, -margin, NODE_WIDTH + margin * 2, bodyHeight() + margin * 2);
}

void RoutingNode::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);
    auto& theme = ThemeManager::instance().current();

    const QRectF bodyRect(0, 0, NODE_WIDTH, bodyHeight());

    // Selection glow (drawn behind the body, within boundingRect margin)
    if (isSelected()) {
        QColor glowColor = color_.lighter(150);
        glowColor.setAlpha(80);
        QPainterPath glowPath;
        glowPath.addRoundedRect(bodyRect.adjusted(-4, -4, 4, 4), 8, 8);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(glowColor));
        painter->drawPath(glowPath);
    }

    // Body background
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(bodyRect, 6, 6);
    QColor borderColor = isSelected() ? color_.lighter(160) : color_.darker(140);
    painter->setPen(QPen(borderColor, isSelected() ? 2.0 : 1.5));
    painter->setBrush(QBrush(theme.surface));
    painter->drawPath(bodyPath);

    // Header bar (rounded top corners, flat bottom via clip)
    painter->save();
    painter->setClipRect(QRectF(0, 0, NODE_WIDTH, HEADER_HEIGHT));
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(color_.darker(110)));
    painter->drawRoundedRect(QRectF(1, 1, NODE_WIDTH - 2, HEADER_HEIGHT + 6), 5, 5);
    painter->restore();

    QRectF headerRect(0, 0, NODE_WIDTH, HEADER_HEIGHT);

    // Header text
    QFont headerFont;
    headerFont.setPixelSize(11);
    headerFont.setBold(true);
    painter->setFont(headerFont);
    painter->setPen(theme.text);
    painter->drawText(headerRect.adjusted(8, 0, -8, 0),
                      Qt::AlignVCenter | Qt::AlignLeft, name_);

    // Node type badge
    QString badge;
    switch (type_) {
        case NodeType::InputSource:  badge = "IN"; break;
        case NodeType::Track:        badge = "TRK"; break;
        case NodeType::Bus:          badge = "BUS"; break;
        case NodeType::Master:       badge = "MST"; break;
        case NodeType::OutputDevice: badge = "OUT"; break;
    }
    QFont badgeFont;
    badgeFont.setPixelSize(8);
    badgeFont.setBold(true);
    painter->setFont(badgeFont);
    QFontMetrics bfm(badgeFont);
    int bw = bfm.horizontalAdvance(badge) + 8;
    QRectF badgeRect(NODE_WIDTH - bw - 6, 5, bw, 16);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(color_));
    painter->drawRoundedRect(badgeRect, 3, 3);
    painter->setPen(Qt::white);
    painter->drawText(badgeRect, Qt::AlignCenter, badge);

    // Effects list
    QFont fxFont;
    fxFont.setPixelSize(9);
    painter->setFont(fxFont);
    qreal y = HEADER_HEIGHT;
    if (effectNames_.isEmpty()) {
        painter->setPen(theme.textDim);
        painter->drawText(QRectF(10, y, NODE_WIDTH - 20, EFFECT_ROW_HEIGHT),
                          Qt::AlignVCenter | Qt::AlignLeft, "(no effects)");
    } else {
        for (const auto& fx : effectNames_) {
            painter->setPen(theme.textDim);
            QRectF fxRect(10, y, NODE_WIDTH - 20, EFFECT_ROW_HEIGHT);

            QRectF dotRect(12, y + (EFFECT_ROW_HEIGHT - 6) / 2, 6, 6);
            painter->setBrush(QBrush(color_.lighter(130)));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(dotRect);

            painter->setPen(theme.textDim);
            painter->drawText(fxRect.adjusted(14, 0, 0, 0),
                              Qt::AlignVCenter | Qt::AlignLeft, fx);
            y += EFFECT_ROW_HEIGHT;
        }
    }

    // Separator line before jack area
    qreal sepY = bodyHeight() - JACK_AREA_HEIGHT;
    painter->setPen(QPen(theme.border, 0.5));
    painter->drawLine(QPointF(4, sepY), QPointF(NODE_WIDTH - 4, sepY));

    // Jack labels
    QFont jackFont;
    jackFont.setPixelSize(8);
    painter->setFont(jackFont);
    painter->setPen(theme.textDim);
    qreal jackLabelY = sepY + JACK_AREA_HEIGHT / 2;
    if (!inputJacks_.empty())
        painter->drawText(QRectF(JACK_OUTER_DIAMETER + 4, jackLabelY - 8, 30, 16),
                          Qt::AlignVCenter | Qt::AlignLeft, "IN");
    if (!outputJacks_.empty())
        painter->drawText(QRectF(NODE_WIDTH - JACK_OUTER_DIAMETER - 34, jackLabelY - 8, 30, 16),
                          Qt::AlignVCenter | Qt::AlignRight, "OUT");
}

void RoutingNode::setEffectNames(const QStringList& names)
{
    effectNames_ = names;
    rebuildJacks();
    prepareGeometryChange();
    update();
}

void RoutingNode::setInputJackCount(int count)
{
    for (auto* j : inputJacks_) delete j;
    inputJacks_.clear();
    for (int i = 0; i < count; ++i)
        inputJacks_.push_back(new JackItem(true, i, this));
    rebuildJacks();
}

void RoutingNode::setOutputJackCount(int count)
{
    for (auto* j : outputJacks_) delete j;
    outputJacks_.clear();
    for (int i = 0; i < count; ++i)
        outputJacks_.push_back(new JackItem(false, i, this));
    rebuildJacks();
}

JackItem* RoutingNode::inputJack(int index) const
{
    if (index >= 0 && index < static_cast<int>(inputJacks_.size()))
        return inputJacks_[index];
    return nullptr;
}

JackItem* RoutingNode::outputJack(int index) const
{
    if (index >= 0 && index < static_cast<int>(outputJacks_.size()))
        return outputJacks_[index];
    return nullptr;
}

void RoutingNode::rebuildJacks()
{
    qreal h = bodyHeight();

    int inCount = static_cast<int>(inputJacks_.size());
    for (int i = 0; i < inCount; ++i) {
        qreal spacing = JACK_AREA_HEIGHT / (inCount + 1);
        qreal yOff = h - JACK_AREA_HEIGHT + spacing * (i + 1);
        inputJacks_[i]->setPos(JACK_OUTER_DIAMETER / 2 + 2, yOff);
    }

    int outCount = static_cast<int>(outputJacks_.size());
    for (int i = 0; i < outCount; ++i) {
        qreal spacing = JACK_AREA_HEIGHT / (outCount + 1);
        qreal yOff = h - JACK_AREA_HEIGHT + spacing * (i + 1);
        outputJacks_[i]->setPos(NODE_WIDTH - JACK_OUTER_DIAMETER / 2 - 2, yOff);
    }
}

} // namespace freedaw
