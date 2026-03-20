#pragma once

#include "engine/EditManager.h"
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>

namespace freedaw {

class RecoveryDialog : public QDialog {
    Q_OBJECT

public:
    explicit RecoveryDialog(const QList<EditManager::RecoveryInfo>& files,
                            QWidget* parent = nullptr);

    EditManager::RecoveryInfo selectedRecovery() const;

private:
    QListWidget* listWidget_ = nullptr;
    QList<EditManager::RecoveryInfo> recoveryFiles_;
};

} // namespace freedaw
