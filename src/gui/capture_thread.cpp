#include "../../include/gui/capture_thread.h"
#include "../../include/capture_engine.h"
#include "../../include/common.h"
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

CaptureThread::CaptureThread(QObject *parent) 
    : QThread(parent), m_engine(nullptr), m_settings(nullptr), 
      m_capturing(false), m_paused(false), m_shouldStop(false),
      m_frameCount(0), m_droppedFrames(0), m_startTime(0) {}

CaptureThread::~CaptureThread() {
    stopCapture();
    if (m_settings) {
        delete m_settings;
    }
}

void CaptureThread::startCapture(const playrec::CaptureSettings &settings) {
    QMutexLocker locker(&m_mutex);
    
    if (m_capturing) {
        qWarning() << "Capture already in progress";
        return;
    }
    
    // Store settings
    if (m_settings) {
        delete m_settings;
    }
    m_settings = new playrec::CaptureSettings(settings);
    
    m_shouldStop = false;
    start();
}

void CaptureThread::stopCapture() {
    QMutexLocker locker(&m_mutex);
    
    if (!m_capturing) {
        return;
    }
    
    m_shouldStop = true;
    m_condition.wakeAll();
    
    locker.unlock();
    
    if (isRunning()) {
        quit();
        wait(5000); // Wait up to 5 seconds for thread to finish
    }
}

void CaptureThread::pauseCapture() {
    QMutexLocker locker(&m_mutex);
    m_paused = true;
}

void CaptureThread::resumeCapture() {
    QMutexLocker locker(&m_mutex);
    m_paused = false;
    m_condition.wakeAll();
}

bool CaptureThread::isCapturing() const {
    QMutexLocker locker(&m_mutex);
    return m_capturing;
}

bool CaptureThread::isPaused() const {
    QMutexLocker locker(&m_mutex);
    return m_paused;
}

void CaptureThread::run() {
    {
        QMutexLocker locker(&m_mutex);
        m_capturing = true;
        m_frameCount = 0;
        m_droppedFrames = 0;
        m_startTime = QDateTime::currentMSecsSinceEpoch();
    }
    
    try {
        // Initialize capture engine
        m_engine = std::make_unique<playrec::CaptureEngine>();
        
        // Copy capture settings
        playrec::CaptureSettings engineSettings = *m_settings;
        
        // Initialize capture engine
        if (!m_engine->initialize(engineSettings)) {
            emit captureError("Failed to initialize capture engine");
            m_capturing = false;
            return;
        }
        
        emit captureStarted();
        
        // Start capture
        if (!m_engine->start_capture()) {
            emit captureError("Failed to start capture");
            m_capturing = false;
            return;
        }
        
        // Capture loop
        auto startTime = std::chrono::steady_clock::now();
        
        while (!m_shouldStop) {
            {
                QMutexLocker locker(&m_mutex);
                while (m_paused && !m_shouldStop) {
                    m_condition.wait(&m_mutex);
                }
                if (m_shouldStop) break;
            }
            
            // Process events and update statistics
            auto currentTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - startTime).count();
            
            if (duration > 0) {
                m_frameCount++;
                
                // Emit statistics every second
                if (m_frameCount % m_settings->frameRate == 0) {
                    auto stats = m_engine->get_stats();
                    qint64 fileSize = stats.file_size_bytes;
                    int fps = static_cast<int>(stats.average_fps);
                    
                    emit statsUpdated(fps, m_frameCount, static_cast<int>(stats.frames_dropped), fileSize);
                }
            }
            
            // Sleep to maintain frame rate
            msleep(1000 / m_settings->frameRate);
        }
        
        // Stop capture
        m_engine->stop_capture();
        m_engine.reset();
        
        emit captureStopped();
        
    } catch (const std::exception &e) {
        emit captureError(QString("Capture error: %1").arg(e.what()));
        m_capturing = false;
    }
    
    {
        QMutexLocker locker(&m_mutex);
        m_capturing = false;
    }
}

void CaptureThread::processVideoFrame(const playrec::Frame &frame) {
    QImage image = convertFrameToQImage(frame);
    emit frameReady(image);
}

QImage CaptureThread::convertFrameToQImage(const playrec::Frame &frame) {
    // Convert frame data to QImage
    // This is a placeholder implementation
    QImage image(frame.width, frame.height, QImage::Format_RGB32);
    
    // For now, create a placeholder image
    image.fill(Qt::black);
    
    return image;
}

qint64 CaptureThread::getFileSize(const QString &filePath) const {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        return fileInfo.size();
    }
    return 0;
}