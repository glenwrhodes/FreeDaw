#include "ExportDialog.h"
#include "utils/ThemeManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QFileInfo>

namespace freedaw {

ExportDialog::ExportDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Export Audio");
    setAccessibleName("Export Audio Dialog");
    setMinimumWidth(480);

    auto& theme = ThemeManager::instance().current();

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto* titleLabel = new QLabel("Export Mix to WAV", this);
    titleLabel->setAccessibleName("Export Dialog Title");
    titleLabel->setStyleSheet(
        QString("font-size: 14px; font-weight: bold; color: %1;")
            .arg(theme.text.name()));
    mainLayout->addWidget(titleLabel);

    auto* form = new QFormLayout();
    form->setSpacing(8);

    auto* pathRow = new QHBoxLayout();
    pathEdit_ = new QLineEdit(this);
    pathEdit_->setAccessibleName("Export File Path");
    pathEdit_->setPlaceholderText("Select output file...");
    pathEdit_->setReadOnly(false);
    pathRow->addWidget(pathEdit_, 1);

    browseBtn_ = new QPushButton("Browse...", this);
    browseBtn_->setAccessibleName("Browse for Export Path");
    connect(browseBtn_, &QPushButton::clicked, this, &ExportDialog::onBrowse);
    pathRow->addWidget(browseBtn_);

    form->addRow("Output File:", pathRow);

    sampleRateCombo_ = new QComboBox(this);
    sampleRateCombo_->setAccessibleName("Sample Rate");
    sampleRateCombo_->addItem("44100 Hz", 44100);
    sampleRateCombo_->addItem("48000 Hz", 48000);
    sampleRateCombo_->addItem("88200 Hz", 88200);
    sampleRateCombo_->addItem("96000 Hz", 96000);
    sampleRateCombo_->setCurrentIndex(0);
    form->addRow("Sample Rate:", sampleRateCombo_);

    bitDepthCombo_ = new QComboBox(this);
    bitDepthCombo_->setAccessibleName("Bit Depth");
    bitDepthCombo_->addItem("16-bit", 16);
    bitDepthCombo_->addItem("24-bit", 24);
    bitDepthCombo_->addItem("32-bit (float)", 32);
    bitDepthCombo_->setCurrentIndex(1);
    form->addRow("Bit Depth:", bitDepthCombo_);

    normalizeCheck_ = new QCheckBox("Normalize output", this);
    normalizeCheck_->setAccessibleName("Normalize Output");
    form->addRow("", normalizeCheck_);

    mainLayout->addLayout(form);

    progressBar_ = new QProgressBar(this);
    progressBar_->setAccessibleName("Export Progress");
    progressBar_->setRange(0, 1000);
    progressBar_->setValue(0);
    progressBar_->setVisible(false);
    mainLayout->addWidget(progressBar_);

    statusLabel_ = new QLabel(this);
    statusLabel_->setAccessibleName("Export Status");
    statusLabel_->setStyleSheet(
        QString("color: %1; font-size: 11px;").arg(theme.textDim.name()));
    statusLabel_->setVisible(false);
    mainLayout->addWidget(statusLabel_);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();

    cancelBtn_ = new QPushButton("Cancel", this);
    cancelBtn_->setAccessibleName("Cancel Export");
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
    buttonRow->addWidget(cancelBtn_);

    exportBtn_ = new QPushButton("Export", this);
    exportBtn_->setAccessibleName("Start Export");
    exportBtn_->setDefault(true);
    exportBtn_->setStyleSheet(
        QString("QPushButton { background: %1; color: #fff; font-weight: bold; "
                "padding: 6px 20px; border-radius: 4px; }"
                "QPushButton:hover { background: %2; }"
                "QPushButton:disabled { background: %3; color: %4; }")
            .arg(theme.accent.name(), theme.accentLight.name(),
                 theme.border.name(), theme.textDim.name()));
    exportBtn_->setEnabled(false);
    connect(exportBtn_, &QPushButton::clicked, this, &QDialog::accept);
    buttonRow->addWidget(exportBtn_);

    connect(pathEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        exportBtn_->setEnabled(!text.trimmed().isEmpty());
    });

    mainLayout->addLayout(buttonRow);

    QSettings qs;
    int srIdx = sampleRateCombo_->findData(qs.value("export/sampleRate", 44100));
    if (srIdx >= 0) sampleRateCombo_->setCurrentIndex(srIdx);
    int bdIdx = bitDepthCombo_->findData(qs.value("export/bitDepth", 24));
    if (bdIdx >= 0) bitDepthCombo_->setCurrentIndex(bdIdx);
    normalizeCheck_->setChecked(qs.value("export/normalize", false).toBool());
}

ExportSettings ExportDialog::settings() const
{
    ExportSettings s;
    s.destFile = juce::File(juce::String(pathEdit_->text().toUtf8().constData()));
    s.sampleRate = sampleRateCombo_->currentData().toDouble();
    s.bitDepth = bitDepthCombo_->currentData().toInt();
    s.normalize = normalizeCheck_->isChecked();

    QSettings qs;
    qs.setValue("export/sampleRate", s.sampleRate);
    qs.setValue("export/bitDepth", s.bitDepth);
    qs.setValue("export/normalize", s.normalize);

    return s;
}

void ExportDialog::setExporting(bool exporting)
{
    exportBtn_->setEnabled(!exporting);
    browseBtn_->setEnabled(!exporting);
    sampleRateCombo_->setEnabled(!exporting);
    bitDepthCombo_->setEnabled(!exporting);
    normalizeCheck_->setEnabled(!exporting);
    progressBar_->setVisible(exporting);
    statusLabel_->setVisible(exporting);

    if (exporting) {
        progressBar_->setValue(0);
        statusLabel_->setText("Exporting...");
        cancelBtn_->setText("Cancel");
    } else {
        cancelBtn_->setText("Close");
    }
}

void ExportDialog::setProgress(float progress)
{
    progressBar_->setValue(static_cast<int>(progress * 1000.0f));
    int pct = static_cast<int>(progress * 100.0f);
    statusLabel_->setText(QString("Exporting... %1%").arg(pct));
}

void ExportDialog::onBrowse()
{
    QSettings settings;
    QString startDir = settings.value("paths/lastExportDir",
                                      QDir::homePath()).toString();
    if (startDir.isEmpty() || !QDir(startDir).exists())
        startDir = QDir::homePath();

    QString path = QFileDialog::getSaveFileName(
        this, "Export Audio", startDir + "/mix.wav",
        "WAV Audio (*.wav)");
    if (path.isEmpty()) return;

    if (!path.endsWith(".wav", Qt::CaseInsensitive))
        path += ".wav";

    settings.setValue("paths/lastExportDir", QFileInfo(path).absolutePath());
    pathEdit_->setText(path);
}

} // namespace freedaw
