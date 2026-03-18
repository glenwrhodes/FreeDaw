#include "EffectSelectorDialog.h"
#include "utils/ThemeManager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>

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

EffectSelectorDialog::EffectSelectorDialog(juce::KnownPluginList* pluginList,
                                           QWidget* parent)
    : QDialog(parent), pluginList_(pluginList)
{
    setWindowTitle("Add Effect");
    setAccessibleName("Effect Selector");
    setMinimumSize(500, 500);
    resize(550, 600);

    auto& theme = ThemeManager::instance().current();
    setStyleSheet(
        QString("QDialog { background: %1; }"
                "QLabel { color: %2; }"
                "QLineEdit { background: %3; color: %2; border: 1px solid %4;"
                "  padding: 6px; border-radius: 3px; }"
                "QTreeWidget { background: %3; color: %2; border: 1px solid %4; }"
                "QTreeWidget::item:selected { background: %5; }"
                "QHeaderView::section { background: %1; color: %2; border: 1px solid %4;"
                "  padding: 4px; }")
            .arg(theme.surface.name(), theme.text.name(),
                 theme.background.name(), theme.border.name(),
                 theme.accent.name()));

    auto* layout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel("Select an effect to add:", this);
    titleLabel->setStyleSheet(
        QString("font-weight: bold; font-size: 13px; color: %1;")
            .arg(theme.text.name()));
    layout->addWidget(titleLabel);

    searchField_ = new QLineEdit(this);
    searchField_->setPlaceholderText("Search effects...");
    searchField_->setAccessibleName("Effect Search");
    layout->addWidget(searchField_);

    treeWidget_ = new QTreeWidget(this);
    treeWidget_->setAccessibleName("Effect List");
    treeWidget_->setHeaderLabels({"Name", "Type", "Manufacturer"});
    treeWidget_->setRootIsDecorated(false);
    treeWidget_->setAlternatingRowColors(true);
    treeWidget_->header()->setStretchLastSection(false);
    treeWidget_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeWidget_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    treeWidget_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    layout->addWidget(treeWidget_, 1);

    buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox_);

    connect(searchField_, &QLineEdit::textChanged,
            this, &EffectSelectorDialog::filterList);

    connect(buttonBox_, &QDialogButtonBox::accepted, this, [this]() {
        auto* item = treeWidget_->currentItem();
        if (item) {
            int idx = item->data(0, Qt::UserRole).toInt();
            if (idx >= 0 && idx < static_cast<int>(allEntries_.size())) {
                auto& entry = allEntries_[static_cast<size_t>(idx)];
                selectedEffect_ = entry.name;
                isVst_ = entry.isVst;
                if (isVst_ && entry.vstIndex >= 0
                    && entry.vstIndex < static_cast<int>(vstDescs_.size()))
                    vstDesc_ = vstDescs_[static_cast<size_t>(entry.vstIndex)];
            }
        }
        accept();
    });

    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(treeWidget_, &QTreeWidget::itemDoubleClicked,
            this, [this](QTreeWidgetItem* item, int) {
                int idx = item->data(0, Qt::UserRole).toInt();
                if (idx >= 0 && idx < static_cast<int>(allEntries_.size())) {
                    auto& entry = allEntries_[static_cast<size_t>(idx)];
                    selectedEffect_ = entry.name;
                    isVst_ = entry.isVst;
                    if (isVst_ && entry.vstIndex >= 0
                        && entry.vstIndex < static_cast<int>(vstDescs_.size()))
                        vstDesc_ = vstDescs_[static_cast<size_t>(entry.vstIndex)];
                }
                accept();
            });

    populateList();
}

void EffectSelectorDialog::populateList()
{
    allEntries_.clear();
    vstDescs_.clear();

    for (const auto& fx : builtInEffects()) {
        EffectEntry e;
        e.name = fx;
        e.category = "Built-in";
        e.manufacturer = "FreeDaw";
        e.isVst = false;
        allEntries_.push_back(e);
    }

    if (pluginList_) {
        for (auto& desc : pluginList_->getTypes()) {
            if (desc.isInstrument)
                continue;
            int vstIdx = static_cast<int>(vstDescs_.size());
            vstDescs_.push_back(desc);

            EffectEntry e;
            e.name = QString::fromStdString(desc.name.toStdString());
            e.category = QString::fromStdString(desc.pluginFormatName.toStdString());
            e.manufacturer = QString::fromStdString(desc.manufacturerName.toStdString());
            e.isVst = true;
            e.vstIndex = vstIdx;
            allEntries_.push_back(e);
        }
    }

    filterList(searchField_->text());
}

void EffectSelectorDialog::filterList(const QString& text)
{
    treeWidget_->clear();

    for (size_t i = 0; i < allEntries_.size(); ++i) {
        auto& e = allEntries_[i];

        if (!text.isEmpty()) {
            if (!e.name.contains(text, Qt::CaseInsensitive) &&
                !e.manufacturer.contains(text, Qt::CaseInsensitive) &&
                !e.category.contains(text, Qt::CaseInsensitive))
                continue;
        }

        auto* item = new QTreeWidgetItem(treeWidget_);
        item->setText(0, e.name);
        item->setText(1, e.category);
        item->setText(2, e.manufacturer);
        item->setData(0, Qt::UserRole, static_cast<int>(i));
    }
}

} // namespace freedaw
