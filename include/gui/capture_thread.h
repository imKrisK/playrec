#pragma once

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtGui/QImage>
#include <memory>

namespace playrec {
    class CaptureEngine;
    struct CaptureSettings;
    struct Frame;
}

class CaptureThread : public QThread
{
    Q_OBJECT

public:
    explicit CaptureThread(QObject *parent = nullptr);
    ~CaptureThread();

    void startCapture(const playrec::CaptureSettings& settings);
    void stopCapture();
    void pauseCapture();
    void resumeCapture();
    
    bool isCapturing() const;
    bool isPaused() const;

signals:
    void captureStarted();
    void captureStopped();
    void captureError(const QString& error);
    void frameReady(const QImage& frame);
    void statsUpdated(int fps, int frames, int dropped, qint64 fileSize);

protected:
    void run() override;

private:
    void processVideoFrame(const playrec::Frame& frame);
    QImage convertFrameToQImage(const playrec::Frame& frame);
    qint64 getFileSize(const QString& filePath) const;

    std::unique_ptr<playrec::CaptureEngine> m_engine;
    playrec::CaptureSettings* m_settings;
    
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    
    bool m_capturing;
    bool m_paused;
    bool m_shouldStop;
    
    // Statistics
    int m_frameCount;
    int m_droppedFrames;
    qint64 m_startTime;
};