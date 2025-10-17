#include "gui/main_window.h"
#include "gui/preview_widget.h"
#include "gui/settings_dialog.h"
#include "gui/capture_thread.h"
#include "common.h"

#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QApplication>
#include <QtCore/QStandardPaths>
#include <QtCore/QSettings>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_previewWidget(nullptr)
    , m_previewGroup(nullptr)
    , m_controlsGroup(nullptr)
    , m_startButton(nullptr)
    , m_stopButton(nullptr)
    , m_pauseButton(nullptr)
    , m_settingsButton(nullptr)
    , m_outputButton(nullptr)
    , m_outputLabel(nullptr)
    , m_previewCheckBox(nullptr)
    , m_quickSettingsGroup(nullptr)
    , m_codecCombo(nullptr)
    , m_fpsSpinBox(nullptr)
    , m_qualityCombo(nullptr)
    , m_audioCheckBox(nullptr)
    , m_statsGroup(nullptr)
    , m_statusLabel(nullptr)
    , m_fpsLabel(nullptr)
    , m_framesLabel(nullptr)
    , m_droppedLabel(nullptr)
    , m_sizeLabel(nullptr)
    , m_durationLabel(nullptr)
    , m_cpuProgressBar(nullptr)
    , m_logGroup(nullptr)
    , m_logTextEdit(nullptr)
    , m_statusBarLabel(nullptr)
    , m_statusBarProgress(nullptr)
    , m_captureThread(nullptr)
    , m_settingsDialog(nullptr)
    , m_updateTimer(nullptr)
    , m_isRecording(false)
    , m_isPaused(false)
    , m_outputFilePath("gameplay_capture.mp4")
    , m_settings(std::make_unique<playrec::CaptureSettings>())
    , m_mediaPlayer(nullptr)
    , m_videoWidget(nullptr)
    , m_pauseVideoButton(nullptr)
    , m_playbackTimeLabel(nullptr)
    , m_playbackSlider(nullptr)
    , m_isPlayingVideo(false)
{
    setWindowTitle("PlayRec - Game Capture Application");
    setMinimumSize(1200, 800);
    
    // Initialize default settings
    m_settings->target_fps = 60;
    m_settings->quality = playrec::Quality::HIGH;
    m_settings->capture_audio = true;
    m_settings->capture_cursor = true;
    m_settings->codec = "h264";
    m_settings->output_path = m_outputFilePath.toStdString();
    
    setupUI();
    createMenus();
    setupStatusBar();
    setupVideoPlayer();
    loadSettings();
    updateControls();
    
    // Create update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::onUpdateStats);
    
    // Initialize replay section after UI is fully set up (with delay)
    QTimer::singleShot(100, this, &MainWindow::onRefreshRecordings);
    
    logMessage("PlayRec GUI initialized successfully");
}

MainWindow::~MainWindow()
{
    if (m_isRecording) {
        onStopRecording();
    }
    saveSettings();
}

void MainWindow::setupUI()
{
    // Create central widget with splitter layout
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    auto* layout = new QHBoxLayout(m_centralWidget);
    layout->setContentsMargins(8, 8, 8, 8);
    
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    layout->addWidget(m_mainSplitter);
    
    setupCentralWidget();
}

void MainWindow::setupCentralWidget()
{
    // Left side - Preview
    m_previewGroup = new QGroupBox("Preview");
    m_previewWidget = new PreviewWidget;
    m_previewWidget->setMinimumSize(640, 360);
    m_previewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    auto* previewLayout = new QVBoxLayout(m_previewGroup);
    
    // Preview controls
    auto* previewControlsLayout = new QHBoxLayout;
    m_previewCheckBox = new QCheckBox("Enable Preview");
    m_previewCheckBox->setChecked(true);
    previewControlsLayout->addWidget(m_previewCheckBox);
    previewControlsLayout->addStretch();
    
    previewLayout->addLayout(previewControlsLayout);
    previewLayout->addWidget(m_previewWidget, 1);
    
    // Add video widget for replay (initially hidden)
    // We'll show this when playing videos and hide the preview widget
    // Note: Video widget will be added to the same layout but controlled via show/hide
    
    m_mainSplitter->addWidget(m_previewGroup);
    
    // Right side - Controls, Replay, Stats, Log
    m_rightSplitter = new QSplitter(Qt::Vertical);
    
    setupControlsPanel();
    setupReplayPanel();
    setupStatsPanel();
    setupLogPanel();
    
    m_mainSplitter->addWidget(m_rightSplitter);
    
    // Set splitter proportions
    m_mainSplitter->setSizes({900, 400});  // Give more space to preview/video area
    m_rightSplitter->setSizes({160, 120, 140, 120, 100});  // Controls, QuickSettings, Replay, Stats, Log
    
    // Connect signals
    connect(m_previewCheckBox, &QCheckBox::toggled, this, &MainWindow::onPreviewToggle);
}

void MainWindow::setupControlsPanel()
{
    // Recording Controls
    m_controlsGroup = new QGroupBox("Recording Controls");
    auto* controlsLayout = new QVBoxLayout(m_controlsGroup);
    
    // Main control buttons
    auto* buttonLayout = new QHBoxLayout;
    m_startButton = new QPushButton("Start Recording");
    m_startButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; font-weight: bold; padding: 8px 16px; }");
    m_stopButton = new QPushButton("Stop");
    m_stopButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; font-weight: bold; padding: 8px 16px; }");
    m_pauseButton = new QPushButton("Pause");
    m_pauseButton->setStyleSheet("QPushButton { background-color: #ffc107; color: black; font-weight: bold; padding: 8px 16px; }");
    
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_stopButton);
    controlsLayout->addLayout(buttonLayout);
    
    // Output file selection
    auto* outputLayout = new QHBoxLayout;
    outputLayout->addWidget(new QLabel("Output:"));
    m_outputLabel = new QLabel(m_outputFilePath);
    m_outputLabel->setStyleSheet("QLabel { border: 1px solid gray; padding: 4px; background-color: #f8f9fa; }");
    m_outputButton = new QPushButton("Browse...");
    outputLayout->addWidget(m_outputLabel, 1);
    outputLayout->addWidget(m_outputButton);
    controlsLayout->addLayout(outputLayout);
    
    // Settings button
    auto* settingsLayout = new QHBoxLayout;
    m_settingsButton = new QPushButton("Advanced Settings...");
    settingsLayout->addStretch();
    settingsLayout->addWidget(m_settingsButton);
    controlsLayout->addLayout(settingsLayout);
    
    m_rightSplitter->addWidget(m_controlsGroup);
    
    // Quick Settings
    m_quickSettingsGroup = new QGroupBox("Quick Settings");
    auto* quickLayout = new QFormLayout(m_quickSettingsGroup);
    
    m_codecCombo = new QComboBox;
    m_codecCombo->addItems({"H.264 (x264)", "H.265 (x265)"});
    quickLayout->addRow("Codec:", m_codecCombo);
    
    m_fpsSpinBox = new QSpinBox;
    m_fpsSpinBox->setRange(15, 120);
    m_fpsSpinBox->setValue(60);
    m_fpsSpinBox->setSuffix(" FPS");
    quickLayout->addRow("Frame Rate:", m_fpsSpinBox);
    
    m_qualityCombo = new QComboBox;
    m_qualityCombo->addItems({"Low", "Medium", "High", "Ultra"});
    m_qualityCombo->setCurrentText("High");
    quickLayout->addRow("Quality:", m_qualityCombo);
    
    m_audioCheckBox = new QCheckBox("Capture Audio");
    m_audioCheckBox->setChecked(true);
    quickLayout->addRow(m_audioCheckBox);
    
    m_rightSplitter->addWidget(m_quickSettingsGroup);
    
    // Connect control signals
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartRecording);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopRecording);
    connect(m_pauseButton, &QPushButton::clicked, this, &MainWindow::onPauseRecording);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettings);
    connect(m_outputButton, &QPushButton::clicked, this, &MainWindow::onSelectOutputFile);
}

void MainWindow::setupStatsPanel()
{
    m_statsGroup = new QGroupBox("Statistics");
    auto* statsLayout = new QFormLayout(m_statsGroup);
    
    m_statusLabel = new QLabel("Ready");
    m_fpsLabel = new QLabel("0 FPS");
    m_framesLabel = new QLabel("0");
    m_droppedLabel = new QLabel("0");
    m_sizeLabel = new QLabel("0 MB");
    m_durationLabel = new QLabel("00:00:00");
    
    m_cpuProgressBar = new QProgressBar;
    m_cpuProgressBar->setRange(0, 100);
    m_cpuProgressBar->setValue(0);
    m_cpuProgressBar->setFormat("%p% CPU");
    
    statsLayout->addRow("Status:", m_statusLabel);
    statsLayout->addRow("FPS:", m_fpsLabel);
    statsLayout->addRow("Frames:", m_framesLabel);
    statsLayout->addRow("Dropped:", m_droppedLabel);
    statsLayout->addRow("File Size:", m_sizeLabel);
    statsLayout->addRow("Duration:", m_durationLabel);
    statsLayout->addRow("CPU Usage:", m_cpuProgressBar);
    
    m_rightSplitter->addWidget(m_statsGroup);
}

void MainWindow::setupLogPanel()
{
    m_logGroup = new QGroupBox("Log");
    auto* logLayout = new QVBoxLayout(m_logGroup);
    
    m_logTextEdit = new QTextEdit;
    m_logTextEdit->setMaximumHeight(120);
    m_logTextEdit->setReadOnly(true);
    
    // Use monospace font with fallbacks for better cross-platform support
    QFont monoFont;
    monoFont.setFamilies({"Monaco", "Courier New", "monospace"});
    monoFont.setPointSize(9);
    m_logTextEdit->setFont(monoFont);
    
    logLayout->addWidget(m_logTextEdit);
    
    m_rightSplitter->addWidget(m_logGroup);
}

void MainWindow::createMenus()
{
    auto* fileMenu = menuBar()->addMenu("&File");
    
    // Modern Qt6 syntax for addAction
    auto* newRecordingAction = new QAction("&New Recording", this);
    newRecordingAction->setShortcut(QKeySequence::New);
    connect(newRecordingAction, &QAction::triggered, this, &MainWindow::onStartRecording);
    fileMenu->addAction(newRecordingAction);
    
    auto* stopRecordingAction = new QAction("&Stop Recording", this);
    stopRecordingAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(stopRecordingAction, &QAction::triggered, this, &MainWindow::onStopRecording);
    fileMenu->addAction(stopRecordingAction);
    
    fileMenu->addSeparator();
    
    auto* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
    
    auto* settingsMenu = menuBar()->addMenu("&Settings");
    auto* preferencesAction = new QAction("&Preferences...", this);
    preferencesAction->setShortcut(QKeySequence::Preferences);
    connect(preferencesAction, &QAction::triggered, this, &MainWindow::onSettings);
    settingsMenu->addAction(preferencesAction);
    
    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About PlayRec", 
            "PlayRec v1.0.0\n\n"
            "Professional game capture application\n"
            "Built with C++ and Qt6\n\n"
            "© 2024 PlayRec Team");
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupStatusBar()
{
    m_statusBarLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusBarLabel, 1);
    
    m_statusBarProgress = new QProgressBar;
    m_statusBarProgress->setVisible(false);
    m_statusBarProgress->setMaximumWidth(200);
    statusBar()->addWidget(m_statusBarProgress);
}

void MainWindow::onStartRecording()
{
    if (m_isRecording) return;
    
    // Stop any video playback first
    if (m_isPlayingVideo) {
        onStopPlayback();
    }
    
    updateSettings();
    
    // Create capture thread if needed
    if (!m_captureThread) {
        m_captureThread = std::make_unique<CaptureThread>(this);
        connect(m_captureThread.get(), &CaptureThread::captureStarted, this, &MainWindow::onCaptureStarted);
        connect(m_captureThread.get(), &CaptureThread::captureStopped, this, &MainWindow::onCaptureStopped);
        connect(m_captureThread.get(), &CaptureThread::captureError, this, &MainWindow::onCaptureError);
        connect(m_captureThread.get(), &CaptureThread::frameReady, this, &MainWindow::onFrameCaptured);
    }
    
    try {
        m_captureThread->startCapture(*m_settings);
        logMessage("Starting capture...");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Capture Error", QString("Failed to start capture: %1").arg(e.what()));
        logMessage(QString("Error: %1").arg(e.what()));
    }
}

void MainWindow::onStopRecording()
{
    if (!m_isRecording) return;
    
    if (m_captureThread) {
        m_captureThread->stopCapture();
        logMessage("Stopping capture...");
    }
}

void MainWindow::onPauseRecording()
{
    if (!m_isRecording) return;
    
    if (m_captureThread) {
        if (m_isPaused) {
            m_captureThread->resumeCapture();
            m_pauseButton->setText("Pause");
            m_isPaused = false;
            logMessage("Capture resumed");
        } else {
            m_captureThread->pauseCapture();
            m_pauseButton->setText("Resume");
            m_isPaused = true;
            logMessage("Capture paused");
        }
    }
}

void MainWindow::onSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = std::make_unique<SettingsDialog>(this);
    }
    
    m_settingsDialog->setSettings(*m_settings);
    if (m_settingsDialog->exec() == QDialog::Accepted) {
        *m_settings = m_settingsDialog->getSettings();
        updateControls();
        logMessage("Settings updated");
    }
}

void MainWindow::onSelectOutputFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Select Output File", m_outputFilePath, 
        "Video Files (*.mp4 *.avi *.mov);;MP4 Files (*.mp4)");
    
    if (!fileName.isEmpty()) {
        m_outputFilePath = fileName;
        m_outputLabel->setText(QFileInfo(fileName).fileName());
        m_settings->output_path = fileName.toStdString();
        logMessage(QString("Output file: %1").arg(fileName));
    }
}

void MainWindow::onPreviewToggle(bool enabled)
{
    m_previewWidget->setPreviewEnabled(enabled);
    logMessage(QString("Preview %1").arg(enabled ? "enabled" : "disabled"));
}

void MainWindow::onUpdateStats()
{
    // This will be called by timer to update statistics
    // Implementation depends on capture thread providing stats
}

void MainWindow::onCaptureStarted()
{
    m_isRecording = true;
    m_isPaused = false;
    updateControls();
    m_updateTimer->start(1000); // Update every second
    m_statusBarLabel->setText("Recording...");
    m_statusLabel->setText("Recording");
    logMessage("Capture started successfully");
}

void MainWindow::onCaptureStopped()
{
    m_isRecording = false;
    m_isPaused = false;
    updateControls();
    m_updateTimer->stop();
    m_statusBarLabel->setText("Ready");
    m_statusLabel->setText("Stopped");
    logMessage("Capture stopped");
}

void MainWindow::onCaptureError(const QString& error)
{
    QMessageBox::critical(this, "Capture Error", error);
    logMessage(QString("Error: %1").arg(error));
    onCaptureStopped();
}

void MainWindow::onFrameCaptured(const QImage& frame)
{
    if (m_previewCheckBox->isChecked()) {
        m_previewWidget->setFrame(frame);
    }
}

void MainWindow::updateControls()
{
    bool canRecord = !m_isRecording && !m_isPlayingVideo;
    bool canPlayVideo = !m_isRecording;
    
    m_startButton->setEnabled(canRecord);
    m_stopButton->setEnabled(m_isRecording);
    m_pauseButton->setEnabled(m_isRecording);
    m_pauseButton->setText(m_isPaused ? "Resume" : "Pause");
    
    // Replay controls
    m_playButton->setEnabled(canPlayVideo && !m_recordingsComboBox->currentData().toString().isEmpty());
    m_pauseVideoButton->setEnabled(m_isPlayingVideo);
    m_stopPlaybackButton->setEnabled(m_isPlayingVideo);
    
    // Update quick settings from current settings
    m_codecCombo->setCurrentText(QString::fromStdString(m_settings->codec) == "h264" ? "H.264 (x264)" : "H.265 (x265)");
    m_fpsSpinBox->setValue(m_settings->target_fps);
    
    QString quality;
    switch (m_settings->quality) {
        case playrec::Quality::LOW: quality = "Low"; break;
        case playrec::Quality::MEDIUM: quality = "Medium"; break;
        case playrec::Quality::HIGH: quality = "High"; break;
        case playrec::Quality::ULTRA: quality = "Ultra"; break;
    }
    m_qualityCombo->setCurrentText(quality);
    m_audioCheckBox->setChecked(m_settings->capture_audio);
}

void MainWindow::updateSettings()
{
    // Update settings from quick controls
    m_settings->codec = m_codecCombo->currentText().contains("H.264") ? "h264" : "h265";
    m_settings->target_fps = m_fpsSpinBox->value();
    
    QString quality = m_qualityCombo->currentText();
    if (quality == "Low") m_settings->quality = playrec::Quality::LOW;
    else if (quality == "Medium") m_settings->quality = playrec::Quality::MEDIUM;
    else if (quality == "High") m_settings->quality = playrec::Quality::HIGH;
    else if (quality == "Ultra") m_settings->quality = playrec::Quality::ULTRA;
    
    m_settings->capture_audio = m_audioCheckBox->isChecked();
    m_settings->output_path = m_outputFilePath.toStdString();
}

void MainWindow::loadSettings()
{
    QSettings settings;
    m_outputFilePath = settings.value("outputPath", "gameplay_capture.mp4").toString();
    m_settings->target_fps = settings.value("fps", 30).toInt();  // Match framerate sync
    m_settings->codec = settings.value("codec", "h264").toString().toStdString();
    m_settings->capture_audio = settings.value("audio", true).toBool();
    
    m_outputLabel->setText(QFileInfo(m_outputFilePath).fileName());
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("outputPath", m_outputFilePath);
    settings.setValue("fps", m_settings->target_fps);
    settings.setValue("codec", QString::fromStdString(m_settings->codec));
    settings.setValue("audio", m_settings->capture_audio);
}

void MainWindow::setupReplayPanel()
{
    m_replayGroup = new QGroupBox("Replay Recordings");
    auto* replayLayout = new QVBoxLayout(m_replayGroup);
    
    // Recording selection
    auto* selectionLayout = new QHBoxLayout;
    selectionLayout->addWidget(new QLabel("Recording:"));
    m_recordingsComboBox = new QComboBox;
    m_recordingsComboBox->setMinimumWidth(200);
    m_refreshRecordingsButton = new QPushButton("↻");
    m_refreshRecordingsButton->setMaximumWidth(30);
    m_refreshRecordingsButton->setToolTip("Refresh recordings list");
    
    selectionLayout->addWidget(m_recordingsComboBox, 1);
    selectionLayout->addWidget(m_refreshRecordingsButton);
    replayLayout->addLayout(selectionLayout);
    
    // Current recording info
    m_currentRecordingLabel = new QLabel("No recording selected");
    m_currentRecordingLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    replayLayout->addWidget(m_currentRecordingLabel);
    
    // Playback controls
    auto* playbackLayout = new QHBoxLayout;
    m_playButton = new QPushButton("▶ Play");
    m_playButton->setStyleSheet("QPushButton { background-color: #007bff; color: white; font-weight: bold; padding: 6px 12px; }");
    m_pauseVideoButton = new QPushButton("⏸ Pause");
    m_pauseVideoButton->setStyleSheet("QPushButton { background-color: #ffc107; color: black; font-weight: bold; padding: 6px 12px; }");
    m_stopPlaybackButton = new QPushButton("⏹ Stop");
    m_stopPlaybackButton->setStyleSheet("QPushButton { background-color: #6c757d; color: white; font-weight: bold; padding: 6px 12px; }");
    m_browseRecordingButton = new QPushButton("📁 Browse...");
    
    playbackLayout->addWidget(m_playButton);
    playbackLayout->addWidget(m_pauseVideoButton);
    playbackLayout->addWidget(m_stopPlaybackButton);
    playbackLayout->addWidget(m_browseRecordingButton);
    replayLayout->addLayout(playbackLayout);
    
    // Video playback timeline
    auto* timelineLayout = new QHBoxLayout;
    m_playbackTimeLabel = new QLabel("00:00 / 00:00");
    m_playbackSlider = new QSlider(Qt::Horizontal);
    m_playbackSlider->setEnabled(false);
    
    timelineLayout->addWidget(m_playbackTimeLabel);
    timelineLayout->addWidget(m_playbackSlider, 1);
    replayLayout->addLayout(timelineLayout);
    
    m_rightSplitter->addWidget(m_replayGroup);
    
    // Connect signals
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::onPlayRecording);
    connect(m_pauseVideoButton, &QPushButton::clicked, this, &MainWindow::onPauseVideo);
    connect(m_stopPlaybackButton, &QPushButton::clicked, this, &MainWindow::onStopPlayback);
    connect(m_browseRecordingButton, &QPushButton::clicked, this, &MainWindow::onSelectRecordingFile);
    connect(m_refreshRecordingsButton, &QPushButton::clicked, this, &MainWindow::onRefreshRecordings);
    connect(m_playbackSlider, &QSlider::sliderMoved, this, &MainWindow::onSeekVideo);
    connect(m_recordingsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, [this]() { 
                updateCurrentRecordingInfo();
                // Show preview of selected recording
                QString selectedFile = m_recordingsComboBox->currentData().toString();
                if (!selectedFile.isEmpty()) {
                    loadRecordingPreview(selectedFile);
                }
            });
    
    // Note: onRefreshRecordings() will be called after full UI initialization
}

void MainWindow::onPlayRecording()
{
    QString selectedFile = m_recordingsComboBox->currentData().toString();
    if (selectedFile.isEmpty()) {
        logMessage("No recording selected for playback");
        return;
    }
    
    logMessage(QString("Playing recording: %1").arg(selectedFile));
    
    // Switch to video mode and start playback
    switchToVideoMode();
    
    // Load and play the video
    m_mediaPlayer->setSource(QUrl::fromLocalFile(selectedFile));
    m_mediaPlayer->play();
    m_isPlayingVideo = true;
    
    // Update UI state
    m_playButton->setText("⏸ Playing...");
    m_playButton->setEnabled(false);
    m_pauseVideoButton->setEnabled(true);
    m_stopPlaybackButton->setEnabled(true);
    m_playbackSlider->setEnabled(true);
    
    // Ensure proper video widget sizing
    m_videoWidget->updateGeometry();
}

void MainWindow::onStopPlayback()
{
    if (m_isPlayingVideo) {
        m_mediaPlayer->stop();
        switchToPreviewMode();
        m_isPlayingVideo = false;
        
        // Reset UI state
        m_playButton->setText("▶ Play");
        m_playButton->setEnabled(true);
        m_pauseVideoButton->setEnabled(false);
        m_stopPlaybackButton->setEnabled(false);
        m_playbackSlider->setEnabled(false);
        m_playbackSlider->setValue(0);
        m_playbackTimeLabel->setText("00:00 / 00:00");
        
        logMessage("Video playback stopped");
    }
}

void MainWindow::onPauseVideo()
{
    if (m_isPlayingVideo) {
        if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            m_mediaPlayer->pause();
            m_pauseVideoButton->setText("▶ Resume");
            logMessage("Video paused");
        } else {
            m_mediaPlayer->play();
            m_pauseVideoButton->setText("⏸ Pause");
            logMessage("Video resumed");
        }
    }
}

void MainWindow::onVideoPositionChanged(qint64 position)
{
    if (!m_playbackSlider->isSliderDown()) {
        m_playbackSlider->setValue(static_cast<int>(position));
    }
    
    // Update time label
    int seconds = static_cast<int>(position / 1000);
    int minutes = seconds / 60;
    seconds = seconds % 60;
    
    int totalSeconds = static_cast<int>(m_mediaPlayer->duration() / 1000);
    int totalMinutes = totalSeconds / 60;
    totalSeconds = totalSeconds % 60;
    
    QString timeText = QString("%1:%2 / %3:%4")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(totalMinutes, 2, 10, QChar('0'))
        .arg(totalSeconds, 2, 10, QChar('0'));
    
    m_playbackTimeLabel->setText(timeText);
}

void MainWindow::onVideoDurationChanged(qint64 duration)
{
    m_playbackSlider->setRange(0, static_cast<int>(duration));
}

void MainWindow::onVideoStateChanged(QMediaPlayer::PlaybackState state)
{
    switch (state) {
        case QMediaPlayer::PlayingState:
            m_pauseVideoButton->setText("⏸ Pause");
            break;
        case QMediaPlayer::PausedState:
            m_pauseVideoButton->setText("▶ Resume");
            break;
        case QMediaPlayer::StoppedState:
            m_pauseVideoButton->setText("⏸ Pause");
            break;
    }
}

void MainWindow::onSeekVideo(int position)
{
    m_mediaPlayer->setPosition(position);
}

void MainWindow::switchToVideoMode()
{
    // Hide preview widget and show video widget
    m_previewWidget->hide();
    m_videoWidget->show();
    logMessage("Switched to video playback mode");
}

void MainWindow::switchToPreviewMode()
{
    // Hide video widget and show preview widget
    m_videoWidget->hide();
    m_previewWidget->show();
    logMessage("Switched to preview mode");
}

void MainWindow::onSelectRecordingFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select Recording to Play", 
        QDir::currentPath(),
        "Video Files (*.mp4 *.avi *.mkv *.mov);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        // Add to combo box if not already there
        bool found = false;
        for (int i = 0; i < m_recordingsComboBox->count(); ++i) {
            if (m_recordingsComboBox->itemData(i).toString() == fileName) {
                m_recordingsComboBox->setCurrentIndex(i);
                found = true;
                break;
            }
        }
        
        if (!found) {
            QFileInfo fileInfo(fileName);
            m_recordingsComboBox->addItem(fileInfo.fileName(), fileName);
            m_recordingsComboBox->setCurrentIndex(m_recordingsComboBox->count() - 1);
        }
        
        updateCurrentRecordingInfo();
        logMessage("Selected recording: " + QFileInfo(fileName).fileName());
    }
}

void MainWindow::onRefreshRecordings()
{
    if (!m_recordingsComboBox) {
        return; // UI not fully initialized yet
    }
    
    m_recordingsComboBox->clear();
    
    // Find MP4 files in current directory and subdirectories
    QStringList recordings;
    QDirIterator it(".", QStringList() << "*.mp4", QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        
        // Skip very small files (likely incomplete)
        if (fileInfo.size() > 1024) {
            // Store absolute path for reliable playback
            recordings << fileInfo.absoluteFilePath();
        }
    }
    
    // Sort by modification time (newest first)
    std::sort(recordings.begin(), recordings.end(), [](const QString& a, const QString& b) {
        return QFileInfo(a).lastModified() > QFileInfo(b).lastModified();
    });
    
    // Add to combo box
    for (const QString& recording : recordings) {
        QFileInfo fileInfo(recording);
        QString displayName = QString("%1 (%2)")
            .arg(fileInfo.fileName())
            .arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm"));
        m_recordingsComboBox->addItem(displayName, recording);
    }
    
    if (recordings.isEmpty()) {
        m_recordingsComboBox->addItem("No recordings found", "");
        m_currentRecordingLabel->setText("No recordings available");
    } else {
        updateCurrentRecordingInfo();
        logMessage(QString("Found %1 recordings").arg(recordings.size()));
    }
}

void MainWindow::updateCurrentRecordingInfo()
{
    QString selectedFile = m_recordingsComboBox->currentData().toString();
    if (selectedFile.isEmpty()) {
        m_currentRecordingLabel->setText("No recording selected");
        return;
    }
    
    QFileInfo fileInfo(selectedFile);
    QString info = QString("Size: %1 MB, Modified: %2")
        .arg(fileInfo.size() / (1024.0 * 1024.0), 0, 'f', 1)
        .arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm"));
    
    m_currentRecordingLabel->setText(info);
}

void MainWindow::loadRecordingPreview(const QString& filePath)
{
    // For now, show a placeholder indicating a recording is selected
    // In a full implementation, this would extract a frame from the video file
    QImage previewImage(640, 360, QImage::Format_RGB32);
    previewImage.fill(QColor(40, 40, 60));
    
    // Draw some text to indicate this is a recording preview
    QPainter painter(&previewImage);
    painter.setPen(Qt::white);
    QFont font;
    font.setPointSize(16);
    painter.setFont(font);
    
    QFileInfo fileInfo(filePath);
    QString text = QString("Recording Preview\n%1\n\nSize: %2 MB\nClick Play to open in video player")
        .arg(fileInfo.fileName())
        .arg(fileInfo.size() / (1024.0 * 1024.0), 0, 'f', 1);
    
    painter.drawText(previewImage.rect(), Qt::AlignCenter, text);
    
    // Set this preview in the preview widget
    m_previewWidget->setFrame(previewImage);
    
    logMessage("Loaded preview for: " + fileInfo.fileName());
}

void MainWindow::setupVideoPlayer()
{
    // Create media player
    m_mediaPlayer = new QMediaPlayer(this);
    m_videoWidget = new QVideoWidget(this);
    
    // Set video widget size constraints to match preview widget
    m_videoWidget->setMinimumSize(640, 360);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    
    // Set a preferred size for better display quality
    m_videoWidget->resize(800, 450);  // 16:9 aspect ratio
    
    // Set up video output
    m_mediaPlayer->setVideoOutput(m_videoWidget);
    
    // Add video widget to the preview layout now (but keep it hidden initially)
    auto* previewLayout = qobject_cast<QVBoxLayout*>(m_previewGroup->layout());
    if (previewLayout) {
        previewLayout->addWidget(m_videoWidget, 1);
    }
    
    // Initially hide the video widget (we start in preview mode)
    m_videoWidget->hide();
    
    // Set aspect ratio mode to keep video proportions
    m_videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    
    // Connect media player signals
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::onVideoPositionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::onVideoDurationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onVideoStateChanged);
}

void MainWindow::logMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, message);
    m_logTextEdit->append(logEntry);
}