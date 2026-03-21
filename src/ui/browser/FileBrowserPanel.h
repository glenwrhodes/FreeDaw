#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QComboBox>

namespace OpenDaw {

class FileBrowserPanel : public QWidget {
    Q_OBJECT

public:
    explicit FileBrowserPanel(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {220, 400}; }

signals:
    void fileDoubleClicked(const QString& filePath);

private:
    void navigateTo(const QString& path);
    void setupDragSupport();

    QVBoxLayout* layout_;
    QComboBox* locationCombo_;
    QLineEdit* pathEdit_;
    QTreeView* treeView_;
    QFileSystemModel* model_;
    QPushButton* previewBtn_;
};

} // namespace OpenDaw
