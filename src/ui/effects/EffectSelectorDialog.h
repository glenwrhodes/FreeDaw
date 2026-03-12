#pragma once

#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QString>

namespace freedaw {

class EffectSelectorDialog : public QDialog {
    Q_OBJECT

public:
    explicit EffectSelectorDialog(QWidget* parent = nullptr);

    QString selectedEffect() const { return selectedEffect_; }

    static const QStringList& builtInEffects();

private:
    QListWidget* listWidget_;
    QDialogButtonBox* buttonBox_;
    QString selectedEffect_;
};

} // namespace freedaw
