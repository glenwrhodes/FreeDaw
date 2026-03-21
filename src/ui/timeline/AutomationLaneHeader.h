#pragma once

#include "engine/EditManager.h"
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion::engine;

namespace OpenDaw {

class AutomationLaneHeader : public QWidget {
    Q_OBJECT

public:
    AutomationLaneHeader(te::AudioTrack* track, EditManager* editMgr,
                         QWidget* parent = nullptr);

    te::AudioTrack* track() const { return track_; }
    te::AutomatableParameter* selectedParam() const;

    void setLaneHeight(int h);
    void selectParam(te::AutomatableParameter* param);
    void refresh();

signals:
    void parameterChanged(te::AutomatableParameter* param);
    void closeRequested();
    void bypassToggled(bool bypassed);

private:
    void populateParamCombo();

    te::AudioTrack* track_ = nullptr;
    EditManager* editMgr_ = nullptr;
    QComboBox* paramCombo_ = nullptr;
    QPushButton* bypassBtn_ = nullptr;
    QPushButton* closeBtn_ = nullptr;
    QLabel* paramLabel_ = nullptr;

    QVector<te::AutomatableParameter*> params_;
};

} // namespace OpenDaw
