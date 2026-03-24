#include "OpenDawApplication.h"
#include "ui/MainWindow.h"
#include "ui/dialogs/RecoveryDialog.h"
#include <QDebug>
#include <QFile>

namespace OpenDaw {

OpenDawApplication::OpenDawApplication(QObject* parent)
    : QObject(parent)
{
}

OpenDawApplication::~OpenDawApplication() = default;

bool OpenDawApplication::initialize()
{
    qDebug() << "[init] creating JuceQtBridge";
    bridge_ = std::make_unique<JuceQtBridge>(this);
    bridge_->start();

    qDebug() << "[init] creating AudioEngine";
    audioEngine_ = std::make_unique<AudioEngine>();
    audioEngine_->setDefaultAudioDevice();
    audioEngine_->restoreSavedAudioSettings();

    qDebug() << "[init] creating EditManager";
    editManager_ = std::make_unique<EditManager>(*audioEngine_);
    qDebug() << "[init] EditManager created";
    pluginScanner_ = std::make_unique<PluginScanner>(*audioEngine_);
    qDebug() << "[init] initialize complete";

    return true;
}

void OpenDawApplication::checkRecovery(QWidget* splashToHide)
{
    auto recoveryFiles = EditManager::findRecoveryFiles();
    if (recoveryFiles.isEmpty())
        return;

    if (splashToHide)
        splashToHide->hide();

    RecoveryDialog dlg(recoveryFiles);
    if (dlg.exec() == QDialog::Accepted) {
        auto info = dlg.selectedRecovery();
        juce::File autosaveFile(juce::String(info.autosavePath.toUtf8().constData()));
        editManager_->loadEdit(autosaveFile);
        if (!info.originalPath.isEmpty()) {
            juce::File origFile(juce::String(info.originalPath.toUtf8().constData()));
            editManager_->edit()->editFileRetriever = [origFile]() { return origFile; };
        }
    }

    for (const auto& info : recoveryFiles) {
        QFile::remove(info.autosavePath);
        auto baseName = info.autosavePath.left(info.autosavePath.lastIndexOf('.'));
        QFile::remove(baseName + ".json");
    }

    if (splashToHide)
        splashToHide->show();
}

void OpenDawApplication::showMainWindow()
{
    qDebug() << "[init] creating MainWindow";
    mainWindow_ = std::make_unique<MainWindow>(*this);
    qDebug() << "[init] MainWindow created, showing";
    mainWindow_->show();
    qDebug() << "[init] MainWindow shown";
}

} // namespace OpenDaw
