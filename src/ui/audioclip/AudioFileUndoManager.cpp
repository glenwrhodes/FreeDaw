#include "AudioFileUndoManager.h"
#include <QDateTime>

namespace OpenDaw {

AudioFileUndoManager::AudioFileUndoManager(QObject* parent)
    : QObject(parent)
{
}

AudioFileUndoManager::~AudioFileUndoManager()
{
    clear();
}

juce::File AudioFileUndoManager::backupDir(const juce::File& targetFile) const
{
    return targetFile.getParentDirectory()
        .getChildFile(".OpenDaw_undo")
        .getChildFile(targetFile.getFileNameWithoutExtension());
}

juce::File AudioFileUndoManager::makeBackupPath(const juce::File& targetFile) const
{
    auto dir = backupDir(targetFile);
    dir.createDirectory();

    auto timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    auto name = targetFile.getFileNameWithoutExtension() + "_"
                + juce::String(timestamp.toUtf8().constData()) + ".wav";
    return dir.getChildFile(name);
}

juce::File AudioFileUndoManager::pristineBackupPath(const juce::File& targetFile) const
{
    auto dir = backupDir(targetFile);
    return dir.getChildFile(targetFile.getFileNameWithoutExtension() + "_ORIGINAL.wav");
}

void AudioFileUndoManager::savePristineBackup(const juce::File& targetFile)
{
    auto pristine = pristineBackupPath(targetFile);
    if (pristine.existsAsFile())
        return;

    auto dir = backupDir(targetFile);
    dir.createDirectory();
    targetFile.copyFileTo(pristine);
}

bool AudioFileUndoManager::hasPristineBackup(const juce::File& targetFile) const
{
    return pristineBackupPath(targetFile).existsAsFile();
}

void AudioFileUndoManager::pushUndo(const juce::File& targetFile, const QString& description)
{
    auto backupPath = makeBackupPath(targetFile);
    if (targetFile.copyFileTo(backupPath)) {
        undoStack_.push_back({backupPath, description});
        trimToMaxDepth();

        for (auto& entry : redoStack_)
            entry.backupFile.deleteFile();
        redoStack_.clear();
    }
}

bool AudioFileUndoManager::undo(const juce::File& targetFile)
{
    if (undoStack_.empty()) return false;

    auto entry = undoStack_.back();
    undoStack_.pop_back();

    auto redoBackup = makeBackupPath(targetFile);
    if (!targetFile.copyFileTo(redoBackup))
        return false;
    redoStack_.push_back({redoBackup, entry.description});

    bool ok = entry.backupFile.moveFileTo(targetFile);
    return ok;
}

bool AudioFileUndoManager::redo(const juce::File& targetFile)
{
    if (redoStack_.empty()) return false;

    auto entry = redoStack_.back();
    redoStack_.pop_back();

    auto undoBackup = makeBackupPath(targetFile);
    if (!targetFile.copyFileTo(undoBackup))
        return false;
    undoStack_.push_back({undoBackup, entry.description});

    bool ok = entry.backupFile.moveFileTo(targetFile);
    return ok;
}

bool AudioFileUndoManager::canUndo() const { return !undoStack_.empty(); }
bool AudioFileUndoManager::canRedo() const { return !redoStack_.empty(); }

QString AudioFileUndoManager::undoDescription() const
{
    return undoStack_.empty() ? QString() : undoStack_.back().description;
}

QString AudioFileUndoManager::redoDescription() const
{
    return redoStack_.empty() ? QString() : redoStack_.back().description;
}

void AudioFileUndoManager::clear()
{
    for (auto& e : undoStack_)
        e.backupFile.deleteFile();
    for (auto& e : redoStack_)
        e.backupFile.deleteFile();
    undoStack_.clear();
    redoStack_.clear();
    // Pristine _ORIGINAL backups are intentionally preserved forever
}

void AudioFileUndoManager::setMaxDepth(int depth)
{
    maxDepth_ = std::max(1, depth);
    trimToMaxDepth();
}

void AudioFileUndoManager::trimToMaxDepth()
{
    while (static_cast<int>(undoStack_.size()) > maxDepth_) {
        undoStack_.front().backupFile.deleteFile();
        undoStack_.erase(undoStack_.begin());
    }
}

} // namespace OpenDaw
