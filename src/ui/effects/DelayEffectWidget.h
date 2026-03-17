#pragma once

#include "engine/EditManager.h"
#include "ui/controls/RotaryKnob.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <tracktion_engine/tracktion_engine.h>
#include <vector>

namespace freedaw {

class DelayEffectWidget : public QWidget {
    Q_OBJECT

public:
    DelayEffectWidget(te::DelayPlugin* plugin, EditManager* editMgr,
                      QWidget* parent = nullptr);

    te::Plugin* plugin() const { return plugin_; }

signals:
    void removeRequested(te::Plugin* plugin);

private:
    struct Division {
        QString name;
        double beats; // duration in quarter-note beats
    };

    void setTimeMode(bool sync);
    void applySync();
    void updateSyncDisplay();
    static const std::vector<Division>& divisions();

    te::DelayPlugin* plugin_;
    EditManager* editMgr_;
    bool syncMode_ = false;

    QPushButton* freeBtn_;
    QPushButton* syncBtn_;
    QSpinBox* msSpin_;
    QComboBox* divCombo_;
    QLabel* syncMsLabel_;
    QStackedWidget* timeStack_;
};

} // namespace freedaw
