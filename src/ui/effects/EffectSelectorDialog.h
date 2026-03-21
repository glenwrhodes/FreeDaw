#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QString>
#include <juce_audio_processors/juce_audio_processors.h>

namespace OpenDaw {

class EffectSelectorDialog : public QDialog {
    Q_OBJECT

public:
    explicit EffectSelectorDialog(juce::KnownPluginList* pluginList = nullptr,
                                  QWidget* parent = nullptr);

    QString selectedEffect() const { return selectedEffect_; }
    bool isVstEffect() const { return isVst_; }
    juce::PluginDescription selectedVstPlugin() const { return vstDesc_; }

    static const QStringList& builtInEffects();

private:
    void populateList();
    void filterList(const QString& text);

    juce::KnownPluginList* pluginList_;
    QLineEdit* searchField_;
    QTreeWidget* treeWidget_;
    QDialogButtonBox* buttonBox_;
    QString selectedEffect_;
    bool isVst_ = false;
    juce::PluginDescription vstDesc_;

    struct EffectEntry {
        QString name;
        QString category;
        QString manufacturer;
        bool isVst = false;
        int vstIndex = -1;
    };
    std::vector<EffectEntry> allEntries_;
    std::vector<juce::PluginDescription> vstDescs_;
};

} // namespace OpenDaw
