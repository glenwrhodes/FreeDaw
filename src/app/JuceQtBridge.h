#pragma once

#include <QObject>
#include <QTimer>

namespace freedaw {

class JuceQtBridge : public QObject {
    Q_OBJECT

public:
    explicit JuceQtBridge(QObject* parent = nullptr);
    ~JuceQtBridge() override;

    void start();
    void stop();

private:
    QTimer pumpTimer_;
};

} // namespace freedaw
