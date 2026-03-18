#pragma once

#include "engine/EditManager.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTimer>
#include <QComboBox>

namespace freedaw {

class TransportBar : public QWidget {
    Q_OBJECT

public:
    explicit TransportBar(EditManager* editMgr, QWidget* parent = nullptr);

    QSize sizeHint() const override { return {800, 48}; }

signals:
    void snapModeRequested(int mode);

private slots:
    void onPlay();
    void onStop();
    void onRecord();
    void onLoop();
    void onBpmChanged(double bpm);
    void onTimeSigNumChanged(int num);
    void updatePosition();

private:
    void applyButtonStyle(QPushButton* btn, const QColor& activeColor);

    EditManager* editMgr_;

    QPushButton* playBtn_;
    QPushButton* stopBtn_;
    QPushButton* recordBtn_;
    QPushButton* loopBtn_;
    QLabel*      positionLabel_;
    QLabel*      beatLabel_;
    QDoubleSpinBox* bpmSpin_;
    QSpinBox*    timeSigNumSpin_;
    QComboBox*   timeSigDenCombo_;
    QComboBox*   snapCombo_;

    QTimer positionTimer_;
    bool isRecording_ = false;
};

} // namespace freedaw
