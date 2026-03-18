#include "PluginScanner.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <QDebug>

namespace freedaw {

PluginScanWorker::PluginScanWorker(te::Engine& engine)
    : engine_(engine)
{
}

void PluginScanWorker::doScan()
{
    juce::VST3PluginFormat vst3;

    auto defaultPaths = vst3.getDefaultLocationsToSearch();
    auto filesToScan = vst3.searchPathsForPlugins(defaultPaths, true, true);

    int total = filesToScan.size();
    for (int i = 0; i < total; ++i) {
        juce::OwnedArray<juce::PluginDescription> descriptions;
        try {
            vst3.findAllTypesForFile(descriptions, filesToScan[i]);
        } catch (...) {
            qWarning() << "[PluginScanner] plugin crashed during scan:"
                        << filesToScan[i].toRawUTF8();
            continue;
        }

        for (auto* desc : descriptions) {
            emit pluginFound(*desc);
            emit scanProgress(QString::fromStdString(desc->name.toStdString()),
                              i + 1, total);
        }
    }

    emit scanFinished();
}

PluginScanner::PluginScanner(AudioEngine& engine, QObject* parent)
    : QObject(parent), audioEngine_(engine)
{
    loadCachedList();
}

PluginScanner::~PluginScanner()
{
    workerThread_.quit();
    workerThread_.wait();
}

void PluginScanner::startScan()
{
    if (scanning_)
        return;

    if (workerThread_.isRunning()) {
        workerThread_.quit();
        workerThread_.wait();
    }

    scanning_ = true;

    auto* worker = new PluginScanWorker(audioEngine_.engine());
    worker->moveToThread(&workerThread_);

    connect(&workerThread_, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &PluginScanWorker::scanProgress,
            this, &PluginScanner::scanProgress);
    connect(worker, &PluginScanWorker::pluginFound, this,
            [this](const juce::PluginDescription& desc) {
                getPluginList().addType(desc);
            }, Qt::QueuedConnection);
    connect(worker, &PluginScanWorker::scanFinished, this, [this]() {
        scanning_ = false;
        saveCachedList();
        emit scanFinished();
    });

    workerThread_.start();
    QMetaObject::invokeMethod(worker, &PluginScanWorker::doScan);
}

juce::KnownPluginList& PluginScanner::getPluginList()
{
    return audioEngine_.engine().getPluginManager().knownPluginList;
}

juce::File PluginScanner::getCacheFile() const
{
    auto appData = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory);
    return appData.getChildFile("FreeDaw").getChildFile("plugin-cache.xml");
}

void PluginScanner::loadCachedList()
{
    auto cacheFile = getCacheFile();
    if (!cacheFile.existsAsFile()) return;

    auto xml = juce::XmlDocument::parse(cacheFile);
    if (!xml) {
        qWarning() << "[PluginScanner] corrupt plugin cache, deleting";
        cacheFile.deleteFile();
        return;
    }
    getPluginList().recreateFromXml(*xml);

    auto& kpl = getPluginList();
    for (int i = kpl.getNumTypes() - 1; i >= 0; --i) {
        auto desc = kpl.getTypes()[i];
        if (!juce::File(desc.fileOrIdentifier).existsAsFile())
            kpl.removeType(desc);
    }
}

void PluginScanner::saveCachedList()
{
    auto cacheFile = getCacheFile();
    cacheFile.getParentDirectory().createDirectory();

    if (auto xml = getPluginList().createXml()) {
        xml->writeTo(cacheFile);
    }
}

} // namespace freedaw
