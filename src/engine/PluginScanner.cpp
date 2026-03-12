#include "PluginScanner.h"

namespace freedaw {

PluginScanWorker::PluginScanWorker(te::Engine& engine)
    : engine_(engine)
{
}

void PluginScanWorker::doScan()
{
    emit scanFinished();
}

PluginScanner::PluginScanner(AudioEngine& engine, QObject* parent)
    : QObject(parent), audioEngine_(engine)
{
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

    scanning_ = true;

    auto* worker = new PluginScanWorker(audioEngine_.engine());
    worker->moveToThread(&workerThread_);

    connect(&workerThread_, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &PluginScanWorker::scanProgress,
            this, &PluginScanner::scanProgress);
    connect(worker, &PluginScanWorker::scanFinished, this, [this]() {
        scanning_ = false;
        emit scanFinished();
    });

    workerThread_.start();
    QMetaObject::invokeMethod(worker, &PluginScanWorker::doScan);
}

juce::KnownPluginList& PluginScanner::getPluginList()
{
    return audioEngine_.engine().getPluginManager().knownPluginList;
}

} // namespace freedaw
