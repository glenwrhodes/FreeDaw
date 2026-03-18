#pragma once

#include "engine/EditManager.h"
#include <QGraphicsObject>
#include <QGraphicsEllipseItem>
#include <QGraphicsProxyWidget>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QString>
#include <vector>

namespace freedaw {

enum class NodeType { InputSource, Track, Bus, Master, OutputDevice };

class RoutingNode;

class JackItem : public QGraphicsEllipseItem {
public:
    JackItem(bool isInput, int index, RoutingNode* parentNode);

    bool isInput() const { return isInput_; }
    int jackIndex() const { return index_; }
    RoutingNode* parentNode() const { return parentNode_; }
    QPointF sceneCenterPos() const;

    void setConnected(bool c, const QColor& cableColor = Qt::black);
    void setHighlighted(bool h);

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

private:
    bool isInput_;
    int index_;
    RoutingNode* parentNode_;
    bool connected_ = false;
    bool highlighted_ = false;
    QColor cableColor_ = Qt::black;
};

class RoutingNode : public QGraphicsObject {
    Q_OBJECT

public:
    RoutingNode(NodeType type, const QString& name, const QColor& color,
                QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    NodeType nodeType() const { return type_; }
    QString name() const { return name_; }
    QColor categoryColor() const { return color_; }

    void setEffectNames(const QStringList& names);
    void setInputJackCount(int count);
    void setOutputJackCount(int count);

    JackItem* inputJack(int index) const;
    JackItem* outputJack(int index) const;
    int inputJackCount() const { return static_cast<int>(inputJacks_.size()); }
    int outputJackCount() const { return static_cast<int>(outputJacks_.size()); }

    te::AudioTrack* track() const { return track_; }
    void setTrack(te::AudioTrack* t) { track_ = t; }

    juce::String deviceName() const { return deviceName_; }
    void setDeviceName(const juce::String& d) { deviceName_ = d; }

    static constexpr qreal NODE_WIDTH = 150.0;
    static constexpr qreal HEADER_HEIGHT = 26.0;
    static constexpr qreal EFFECT_ROW_HEIGHT = 18.0;
    static constexpr qreal JACK_AREA_HEIGHT = 28.0;
    static constexpr qreal JACK_OUTER_DIAMETER = 16.0;
    static constexpr qreal JACK_INNER_DIAMETER = 8.0;

signals:
    void nodeMoved();
    void renameRequested(const QString& newName);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void rebuildJacks();
    qreal bodyHeight() const;

    NodeType type_;
    QString name_;
    QColor color_;
    QStringList effectNames_;
    std::vector<JackItem*> inputJacks_;
    std::vector<JackItem*> outputJacks_;
    te::AudioTrack* track_ = nullptr;
    juce::String deviceName_;
};

} // namespace freedaw
