#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLabel>

namespace playrec {
    struct CaptureSettings;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void setSettings(const playrec::CaptureSettings& settings);
    playrec::CaptureSettings getSettings() const;

private slots:
    void onCodecChanged();
    void onQualityChanged();
    void onDefaultsClicked();
    void onAccepted();
    void onRejected();

private:
    void setupUI();
    void setupVideoTab();
    void setupAudioTab();
    void setupAdvancedTab();
    void updateCodecSettings();
    void loadDefaults();

    // Tabs
    QTabWidget* m_tabWidget;
    QWidget* m_videoTab;
    QWidget* m_audioTab;
    QWidget* m_advancedTab;
    
    // Video Settings
    QGroupBox* m_videoGroup;
    QComboBox* m_codecCombo;
    QComboBox* m_qualityCombo;
    QSpinBox* m_fpsSpinBox;
    QSpinBox* m_bitrateSpinBox;
    QCheckBox* m_hardwareAccelCheckBox;
    QLabel* m_codecInfoLabel;
    
    // Audio Settings
    QGroupBox* m_audioGroup;
    QCheckBox* m_audioEnabledCheckBox;
    QComboBox* m_audioFormatCombo;
    QSpinBox* m_sampleRateSpinBox;
    QSpinBox* m_audioBitrateSpinBox;
    QSpinBox* m_channelsSpinBox;
    
    // Advanced Settings
    QGroupBox* m_advancedGroup;
    QCheckBox* m_cursorCheckBox;
    QSpinBox* m_bufferSizeSpinBox;
    QSpinBox* m_threadCountSpinBox;
    QLineEdit* m_customArgsLineEdit;
    
    // Buttons
    QDialogButtonBox* m_buttonBox;
    QPushButton* m_defaultsButton;
};