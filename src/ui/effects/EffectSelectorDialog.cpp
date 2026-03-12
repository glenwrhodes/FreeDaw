#include "EffectSelectorDialog.h"
#include "utils/ThemeManager.h"
#include <QVBoxLayout>
#include <QLabel>

namespace freedaw {

const QStringList& EffectSelectorDialog::builtInEffects()
{
    static QStringList effects = {
        "Reverb",
        "EQ",
        "Compressor",
        "Delay",
        "Chorus",
        "Phaser",
        "Low Pass Filter",
        "Pitch Shift",
    };
    return effects;
}

EffectSelectorDialog::EffectSelectorDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Add Effect");
    setAccessibleName("Effect Selector");
    setMinimumSize(300, 400);

    auto& theme = ThemeManager::instance().current();
    setStyleSheet(
        QString("QDialog { background: %1; }"
                "QLabel { color: %2; }"
                "QListWidget { background: %3; color: %2; border: 1px solid %4; }"
                "QListWidget::item:selected { background: %5; }")
            .arg(theme.surface.name(), theme.text.name(),
                 theme.background.name(), theme.border.name(),
                 theme.accent.name()));

    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel("Select an effect to add:", this);
    label->setStyleSheet(QString("font-weight: bold; font-size: 13px; color: %1;")
                             .arg(theme.text.name()));
    layout->addWidget(label);

    listWidget_ = new QListWidget(this);
    listWidget_->setAccessibleName("Effect List");

    for (const auto& fx : builtInEffects()) {
        auto* item = new QListWidgetItem(fx, listWidget_);
        item->setSizeHint(QSize(0, 32));
    }

    layout->addWidget(listWidget_);

    buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox_);

    connect(buttonBox_, &QDialogButtonBox::accepted, this, [this]() {
        auto* item = listWidget_->currentItem();
        if (item)
            selectedEffect_ = item->text();
        accept();
    });
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(listWidget_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                selectedEffect_ = item->text();
                accept();
            });
}

} // namespace freedaw
