#include "FreeDawApplication.h"
#include "ui/MainWindow.h"

namespace freedaw {

FreeDawApplication::FreeDawApplication(QObject* parent)
    : QObject(parent)
{
}

FreeDawApplication::~FreeDawApplication() = default;

bool FreeDawApplication::initialize()
{
    bridge_ = std::make_unique<JuceQtBridge>(this);
    bridge_->start();

    audioEngine_ = std::make_unique<AudioEngine>();
    audioEngine_->setDefaultAudioDevice();

    editManager_ = std::make_unique<EditManager>(*audioEngine_);
    pluginScanner_ = std::make_unique<PluginScanner>(*audioEngine_);

    return true;
}

void FreeDawApplication::showMainWindow()
{
    mainWindow_ = std::make_unique<MainWindow>(*this);
    mainWindow_->show();
}

} // namespace freedaw
