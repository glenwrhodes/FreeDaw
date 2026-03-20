#include "RecoveryDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

namespace freedaw {

RecoveryDialog::RecoveryDialog(const QList<EditManager::RecoveryInfo>& files,
                               QWidget* parent)
    : QDialog(parent), recoveryFiles_(files)
{
    setWindowTitle("Session Recovery");
    setAccessibleName("Session Recovery Dialog");
    setMinimumSize(450, 250);

    auto* layout = new QVBoxLayout(this);

    auto* messageLabel = new QLabel(
        "FreeDaw found unsaved session data from a previous run.\n"
        "Would you like to restore it?", this);
    messageLabel->setAccessibleName("Recovery message");
    messageLabel->setWordWrap(true);
    layout->addWidget(messageLabel);

    listWidget_ = new QListWidget(this);
    listWidget_->setAccessibleName("Recovery sessions list");
    for (const auto& info : recoveryFiles_) {
        QString label;
        if (!info.originalPath.isEmpty())
            label = info.originalPath;
        else
            label = "Untitled project";

        if (!info.timestamp.isEmpty()) {
            auto dt = QDateTime::fromString(info.timestamp, Qt::ISODate);
            if (dt.isValid())
                label += "  (" + dt.toString("yyyy-MM-dd hh:mm") + ")";
        }
        listWidget_->addItem(label);
    }
    if (listWidget_->count() > 0)
        listWidget_->setCurrentRow(0);
    layout->addWidget(listWidget_);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    auto* discardBtn = new QPushButton("Discard", this);
    discardBtn->setAccessibleName("Discard recovery data");
    connect(discardBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(discardBtn);

    auto* restoreBtn = new QPushButton("Restore", this);
    restoreBtn->setAccessibleName("Restore session");
    restoreBtn->setDefault(true);
    connect(restoreBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(restoreBtn);

    layout->addLayout(btnLayout);
}

EditManager::RecoveryInfo RecoveryDialog::selectedRecovery() const
{
    int row = listWidget_->currentRow();
    if (row >= 0 && row < recoveryFiles_.size())
        return recoveryFiles_[row];
    return {};
}

} // namespace freedaw
