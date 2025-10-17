#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSplitter>
#include <QtCore/QTimer>
#include <QtCore/QProcess>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#include <memory>

// Forward declarations
class PreviewWidget;
class SettingsDialog;
class CaptureThread;

namespace playrec {
    class CaptureEngine;
    struct CaptureSettings;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartRecording();
    void onStopRecording();
    void onPauseRecording();
    void onSettings();
    void onSelectOutputFile();
    void onPreviewToggle(bool enabled);
    void onUpdateStats();
    void onCaptureStarted();
    void onCaptureStopped();
    void onCaptureError(const QString& error);
    void onFrameCaptured(const QImage& frame);
    
    // Replay functionality
    void onPlayRecording();
    void onStopPlayback();
    void onSelectRecordingFile();
    void onRefreshRecordings();
    
    // Built-in video player
    void onPauseVideo();
    void onVideoPositionChanged(qint64 position);
    void onVideoDurationChanged(qint64 duration);
    void onVideoStateChanged(QMediaPlayer::PlaybackState state);
    void onSeekVideo(int position);

private:
    void setupUI();
    void createMenus();
    void setupStatusBar();
    void setupCentralWidget();
    void setupControlsPanel();
    void setupStatsPanel();
    void setupLogPanel();
    void setupReplayPanel();
    void updateControls();
    void updateSettings();
    void loadSettings();
    void saveSettings();
    void logMessage(const QString& message);
    void updateCurrentRecordingInfo();
    void loadRecordingPreview(const QString& filePath);
    void setupVideoPlayer();
    void switchToVideoMode();
    void switchToPreviewMode();

    // UI Components
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    QSplitter* m_rightSplitter;
    
    // Preview
    PreviewWidget* m_previewWidget;
    QGroupBox* m_previewGroup;
    
    // Controls
    QGroupBox* m_controlsGroup;
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QPushButton* m_pauseButton;
    QPushButton* m_settingsButton;
    QPushButton* m_outputButton;
    QLabel* m_outputLabel;
    QCheckBox* m_previewCheckBox;
    
    // Settings Quick Access
    QGroupBox* m_quickSettingsGroup;
    QComboBox* m_codecCombo;
    QSpinBox* m_fpsSpinBox;
    QComboBox* m_qualityCombo;
    QCheckBox* m_audioCheckBox;
    
    // Statistics
    QGroupBox* m_statsGroup;
    QLabel* m_statusLabel;
    QLabel* m_fpsLabel;
    QLabel* m_framesLabel;
    QLabel* m_droppedLabel;
    QLabel* m_sizeLabel;
    QLabel* m_durationLabel;
    QProgressBar* m_cpuProgressBar;
    
    // Log
    QGroupBox* m_logGroup;
    QTextEdit* m_logTextEdit;
    
    // Replay Panel
    QGroupBox* m_replayGroup;
    QPushButton* m_playButton;
    QPushButton* m_stopPlaybackButton;
    QPushButton* m_browseRecordingButton;
    QPushButton* m_refreshRecordingsButton;
    QComboBox* m_recordingsComboBox;
    QLabel* m_currentRecordingLabel;
    
    // Built-in Video Player
    QMediaPlayer* m_mediaPlayer;
    QVideoWidget* m_videoWidget;
    QPushButton* m_pauseVideoButton;
    QLabel* m_playbackTimeLabel;
    QSlider* m_playbackSlider;
    bool m_isPlayingVideo;
    
    // Status Bar
    QLabel* m_statusBarLabel;
    QProgressBar* m_statusBarProgress;
    
    // Backend
    std::unique_ptr<CaptureThread> m_captureThread;
    std::unique_ptr<SettingsDialog> m_settingsDialog;
    
    // Update timer
    QTimer* m_updateTimer;
    
    // State
    bool m_isRecording;
    bool m_isPaused;
    QString m_outputFilePath;
    std::unique_ptr<playrec::CaptureSettings> m_settings;
};