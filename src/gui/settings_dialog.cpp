#include "../../include/gui/settings_dialog.h"
#include "../../include/common.h"
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QStandardPaths>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("PlayRec Settings");
    setFixedSize(400, 500);
    setupUI();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    setupVideoTab();
    setupAudioTab();
    setupAdvancedTab();
    
    // Dialog buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_defaultsButton = new QPushButton("Defaults");
    m_buttonBox->addButton(m_defaultsButton, QDialogButtonBox::ResetRole);
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onRejected);
    connect(m_defaultsButton, &QPushButton::clicked, this, &SettingsDialog::onDefaultsClicked);
    
    mainLayout->addWidget(m_buttonBox);
}

void SettingsDialog::setupVideoTab() {
    m_videoTab = new QWidget();
    m_tabWidget->addTab(m_videoTab, "Video");
    
    QVBoxLayout *layout = new QVBoxLayout(m_videoTab);
    
    // Video group
    m_videoGroup = new QGroupBox("Video Settings");
    QFormLayout *videoForm = new QFormLayout(m_videoGroup);
    
    // Codec selection
    m_codecCombo = new QComboBox();
    m_codecCombo->addItems({"H.264", "H.265"});
    videoForm->addRow("Codec:", m_codecCombo);
    
    // Quality preset
    m_qualityCombo = new QComboBox();
    m_qualityCombo->addItems({"Low", "Medium", "High", "Ultra"});
    m_qualityCombo->setCurrentText("High");
    videoForm->addRow("Quality:", m_qualityCombo);
    
    // Frame rate
    m_fpsSpinBox = new QSpinBox();
    m_fpsSpinBox->setRange(15, 120);
    m_fpsSpinBox->setValue(30);
    m_fpsSpinBox->setSuffix(" fps");
    videoForm->addRow("Frame Rate:", m_fpsSpinBox);
    
    // Bitrate
    m_bitrateSpinBox = new QSpinBox();
    m_bitrateSpinBox->setRange(500, 50000);
    m_bitrateSpinBox->setValue(5000);
    m_bitrateSpinBox->setSuffix(" kbps");
    videoForm->addRow("Bitrate:", m_bitrateSpinBox);
    
    // Hardware acceleration
    m_hardwareAccelCheckBox = new QCheckBox("Enable hardware acceleration");
    videoForm->addRow(m_hardwareAccelCheckBox);
    
    // Codec info
    m_codecInfoLabel = new QLabel("H.264 provides good compatibility");
    m_codecInfoLabel->setWordWrap(true);
    videoForm->addRow("Info:", m_codecInfoLabel);
    
    layout->addWidget(m_videoGroup);
    layout->addStretch();
    
    connect(m_codecCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onCodecChanged);
    connect(m_qualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onQualityChanged);
}

void SettingsDialog::setupAudioTab() {
    m_audioTab = new QWidget();
    m_tabWidget->addTab(m_audioTab, "Audio");
    
    QVBoxLayout *layout = new QVBoxLayout(m_audioTab);
    
    // Audio group
    m_audioGroup = new QGroupBox("Audio Settings");
    QFormLayout *audioForm = new QFormLayout(m_audioGroup);
    
    // Enable audio
    m_audioEnabledCheckBox = new QCheckBox("Enable audio recording");
    m_audioEnabledCheckBox->setChecked(true);
    audioForm->addRow(m_audioEnabledCheckBox);
    
    // Audio format
    m_audioFormatCombo = new QComboBox();
    m_audioFormatCombo->addItems({"AAC", "MP3", "PCM"});
    audioForm->addRow("Format:", m_audioFormatCombo);
    
    // Sample rate
    m_sampleRateSpinBox = new QSpinBox();
    m_sampleRateSpinBox->setRange(8000, 192000);
    m_sampleRateSpinBox->setValue(48000);
    m_sampleRateSpinBox->setSuffix(" Hz");
    audioForm->addRow("Sample Rate:", m_sampleRateSpinBox);
    
    // Audio bitrate
    m_audioBitrateSpinBox = new QSpinBox();
    m_audioBitrateSpinBox->setRange(64, 320);
    m_audioBitrateSpinBox->setValue(128);
    m_audioBitrateSpinBox->setSuffix(" kbps");
    audioForm->addRow("Bitrate:", m_audioBitrateSpinBox);
    
    // Channels
    m_channelsSpinBox = new QSpinBox();
    m_channelsSpinBox->setRange(1, 8);
    m_channelsSpinBox->setValue(2);
    audioForm->addRow("Channels:", m_channelsSpinBox);
    
    layout->addWidget(m_audioGroup);
    layout->addStretch();
}

void SettingsDialog::setupAdvancedTab() {
    m_advancedTab = new QWidget();
    m_tabWidget->addTab(m_advancedTab, "Advanced");
    
    QVBoxLayout *layout = new QVBoxLayout(m_advancedTab);
    
    // Advanced group
    m_advancedGroup = new QGroupBox("Advanced Settings");
    QFormLayout *advancedForm = new QFormLayout(m_advancedGroup);
    
    // Cursor capture
    m_cursorCheckBox = new QCheckBox("Capture cursor");
    m_cursorCheckBox->setChecked(true);
    advancedForm->addRow(m_cursorCheckBox);
    
    // Buffer size
    m_bufferSizeSpinBox = new QSpinBox();
    m_bufferSizeSpinBox->setRange(1, 100);
    m_bufferSizeSpinBox->setValue(10);
    m_bufferSizeSpinBox->setSuffix(" MB");
    advancedForm->addRow("Buffer Size:", m_bufferSizeSpinBox);
    
    // Thread count
    m_threadCountSpinBox = new QSpinBox();
    m_threadCountSpinBox->setRange(1, 16);
    m_threadCountSpinBox->setValue(4);
    advancedForm->addRow("Encoder Threads:", m_threadCountSpinBox);
    
    // Custom arguments
    m_customArgsLineEdit = new QLineEdit();
    m_customArgsLineEdit->setPlaceholderText("Additional FFmpeg arguments");
    advancedForm->addRow("Custom Args:", m_customArgsLineEdit);
    
    layout->addWidget(m_advancedGroup);
    layout->addStretch();
}

void SettingsDialog::onCodecChanged() {
    updateCodecSettings();
}

void SettingsDialog::onQualityChanged() {
    // Update bitrate based on quality preset
    int quality = m_qualityCombo->currentIndex();
    switch (quality) {
        case 0: // Low
            m_bitrateSpinBox->setValue(2000);
            break;
        case 1: // Medium
            m_bitrateSpinBox->setValue(5000);
            break;
        case 2: // High
            m_bitrateSpinBox->setValue(8000);
            break;
        case 3: // Ultra
            m_bitrateSpinBox->setValue(15000);
            break;
    }
}

void SettingsDialog::onDefaultsClicked() {
    loadDefaults();
}

void SettingsDialog::onAccepted() {
    accept();
}

void SettingsDialog::onRejected() {
    reject();
}

void SettingsDialog::updateCodecSettings() {
    QString codec = m_codecCombo->currentText();
    if (codec == "H.264") {
        m_codecInfoLabel->setText("H.264 provides good compatibility and performance");
    } else if (codec == "H.265") {
        m_codecInfoLabel->setText("H.265 provides better compression but requires more CPU");
    }
}

void SettingsDialog::loadDefaults() {
    // Video defaults
    m_codecCombo->setCurrentText("H.264");
    m_qualityCombo->setCurrentText("High");
    m_fpsSpinBox->setValue(30);
    m_bitrateSpinBox->setValue(5000);
    m_hardwareAccelCheckBox->setChecked(false);
    
    // Audio defaults
    m_audioEnabledCheckBox->setChecked(true);
    m_audioFormatCombo->setCurrentText("AAC");
    m_sampleRateSpinBox->setValue(48000);
    m_audioBitrateSpinBox->setValue(128);
    m_channelsSpinBox->setValue(2);
    
    // Advanced defaults
    m_cursorCheckBox->setChecked(true);
    m_bufferSizeSpinBox->setValue(10);
    m_threadCountSpinBox->setValue(4);
    m_customArgsLineEdit->clear();
    
    updateCodecSettings();
}

playrec::CaptureSettings SettingsDialog::getSettings() const {
    playrec::CaptureSettings settings;
    
    // Video settings
    settings.frameRate = m_fpsSpinBox->value();
    settings.videoBitrate = m_bitrateSpinBox->value() * 1000; // Convert to bps
    settings.videoCodec = m_codecCombo->currentText().toStdString();
    settings.capture_cursor = m_cursorCheckBox->isChecked();
    
    // Audio settings
    settings.capture_audio = m_audioEnabledCheckBox->isChecked();
    settings.sampleRate = m_sampleRateSpinBox->value();
    settings.audioBitrate = m_audioBitrateSpinBox->value() * 1000; // Convert to bps
    settings.channels = m_channelsSpinBox->value();
    
    return settings;
}

void SettingsDialog::setSettings(const playrec::CaptureSettings &settings) {
    // Video settings
    m_fpsSpinBox->setValue(settings.frameRate);
    m_bitrateSpinBox->setValue(settings.videoBitrate / 1000); // Convert to kbps
    m_codecCombo->setCurrentText(QString::fromStdString(settings.videoCodec));
    m_cursorCheckBox->setChecked(settings.capture_cursor);
    
    // Audio settings
    m_audioEnabledCheckBox->setChecked(settings.capture_audio);
    m_sampleRateSpinBox->setValue(settings.sampleRate);
    m_audioBitrateSpinBox->setValue(settings.audioBitrate / 1000); // Convert to kbps
    m_channelsSpinBox->setValue(settings.channels);
    
    updateCodecSettings();
}