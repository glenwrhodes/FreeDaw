#include "JuceQtBridge.h"
#include <juce_events/juce_events.h>

namespace freedaw {

JuceQtBridge::JuceQtBridge(QObject* parent)
    : QObject(parent)
{
    connect(&pumpTimer_, &QTimer::timeout, this, []() {
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
            mm->runDispatchLoopUntil(4);
    });
}

JuceQtBridge::~JuceQtBridge()
{
    stop();
}

void JuceQtBridge::start()
{
    pumpTimer_.start(5);
}

void JuceQtBridge::stop()
{
    pumpTimer_.stop();
}

} // namespace freedaw
