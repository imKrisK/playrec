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
    setupMenuBar();
    setupStatusBar();
    loadSettings();
    updateControls();
    
    // Create update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::onUpdateStats);
    
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
    
    auto* previewLayout = new QVBoxLayout(m_previewGroup);
    
    // Preview controls
    auto* previewControlsLayout = new QHBoxLayout;
    m_previewCheckBox = new QCheckBox("Enable Preview");
    m_previewCheckBox->setChecked(true);
    previewControlsLayout->addWidget(m_previewCheckBox);
    previewControlsLayout->addStretch();
    
    previewLayout->addLayout(previewControlsLayout);
    previewLayout->addWidget(m_previewWidget, 1);
    
    m_mainSplitter->addWidget(m_previewGroup);
    
    // Right side - Controls, Settings, Stats, Log
    m_rightSplitter = new QSplitter(Qt::Vertical);
    
    setupControlsPanel();
    setupStatsPanel();
    setupLogPanel();
    
    m_mainSplitter->addWidget(m_rightSplitter);
    
    // Set splitter proportions
    m_mainSplitter->setSizes({800, 400});
    m_rightSplitter->setSizes({200, 150, 150});
    
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
    m_logTextEdit->setFont(QFont("Consolas", 9));
    
    logLayout->addWidget(m_logTextEdit);
    
    m_rightSplitter->addWidget(m_logGroup);
}

void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&New Recording", this, &MainWindow::onStartRecording, QKeySequence::New);
    fileMenu->addAction("&Stop Recording", this, &MainWindow::onStopRecording, QKeySequence("Ctrl+S"));
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);
    
    auto* settingsMenu = menuBar()->addMenu("&Settings");
    settingsMenu->addAction("&Preferences...", this, &MainWindow::onSettings, QKeySequence::Preferences);
    
    auto* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", [this]() {
        QMessageBox::about(this, "About PlayRec", 
            "PlayRec v1.0.0\\n\\n"
            "Professional game capture application\\n"
            "Built with C++ and Qt\\n\\n"
            "© 2024 PlayRec Team");
    });
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
    m_startButton->setEnabled(!m_isRecording);
    m_stopButton->setEnabled(m_isRecording);
    m_pauseButton->setEnabled(m_isRecording);
    m_pauseButton->setText(m_isPaused ? "Resume" : "Pause");
    
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
    m_settings->target_fps = settings.value("fps", 60).toInt();
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

void MainWindow::logMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, message);
    m_logTextEdit->append(logEntry);
}

#include "main_window.moc"