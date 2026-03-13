#include "PluginEditorWindow.h"

namespace freedaw {

std::vector<std::unique_ptr<PluginEditorWindow>> PluginEditorWindow::openWindows_;

PluginEditorWindow::PluginEditorWindow(te::ExternalPlugin& plugin)
    : juce::DocumentWindow(plugin.getName(),
                           juce::Colours::darkgrey,
                           juce::DocumentWindow::allButtons),
      plugin_(plugin)
{
    setUsingNativeTitleBar(true);

    constrainer_.setMinimumOnscreenAmounts(40, 30, 40, 40);
    setConstrainer(&constrainer_);
    setResizable(true, true);

    if (auto* instance = plugin.getAudioPluginInstance()) {
        if (auto* editor = instance->createEditorIfNeeded()) {
            setContentOwned(editor, true);
        }
    }

    setTopLeftPosition(0, 0);
    setVisible(true);
    toFront(true);
}

void PluginEditorWindow::moved()
{
    auto pos = getPosition();
    if (pos.y < 0)
        setTopLeftPosition(pos.x, 0);
}

PluginEditorWindow::~PluginEditorWindow()
{
    clearContentComponent();
}

void PluginEditorWindow::closeButtonPressed()
{
    for (auto it = openWindows_.begin(); it != openWindows_.end(); ++it) {
        if (it->get() == this) {
            auto moved = std::move(*it);
            openWindows_.erase(it);
            break;
        }
    }
}

void PluginEditorWindow::showForPlugin(te::ExternalPlugin& plugin)
{
    for (auto& w : openWindows_) {
        if (&w->getPlugin() == &plugin) {
            w->toFront(true);
            return;
        }
    }
    openWindows_.push_back(std::make_unique<PluginEditorWindow>(plugin));
}

void PluginEditorWindow::closeForPlugin(te::ExternalPlugin& plugin)
{
    for (auto it = openWindows_.begin(); it != openWindows_.end(); ++it) {
        if (&(*it)->getPlugin() == &plugin) {
            openWindows_.erase(it);
            return;
        }
    }
}

void PluginEditorWindow::closeAll()
{
    openWindows_.clear();
}

} // namespace freedaw
