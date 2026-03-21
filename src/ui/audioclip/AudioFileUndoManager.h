#pragma once

#include <juce_core/juce_core.h>
#include <QString>
#include <QObject>
#include <vector>

namespace OpenDaw {

class AudioFileUndoManager : public QObject {
    Q_OBJECT

public:
    explicit AudioFileUndoManager(QObject* parent = nullptr);
    ~AudioFileUndoManager() override;

    void pushUndo(const juce::File& targetFile, const QString& description);
    bool undo(const juce::File& targetFile);
    bool redo(const juce::File& targetFile);

    bool canUndo() const;
    bool canRedo() const;

    QString undoDescription() const;
    QString redoDescription() const;

    void savePristineBackup(const juce::File& targetFile);
    juce::File pristineBackupPath(const juce::File& targetFile) const;
    bool hasPristineBackup(const juce::File& targetFile) const;

    void clear();
    void setMaxDepth(int depth);

private:
    struct Entry {
        juce::File backupFile;
        QString description;
    };

    juce::File backupDir(const juce::File& targetFile) const;
    juce::File makeBackupPath(const juce::File& targetFile) const;
    void trimToMaxDepth();

    std::vector<Entry> undoStack_;
    std::vector<Entry> redoStack_;
    int maxDepth_ = 20;
};

} // namespace OpenDaw
