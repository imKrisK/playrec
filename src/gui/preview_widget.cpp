#include "gui/preview_widget.h"
#include <QtGui/QPaintEvent>
#include <QtCore/QMutexLocker>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_previewEnabled(true)
    , m_hasFrame(false)
    , m_noPreviewText("No video preview available")
    , m_disabledText("Preview disabled")
{
    setMinimumSize(320, 180);
    setStyleSheet("QWidget { background-color: #2b2b2b; border: 1px solid #555; }");
    setAttribute(Qt::WA_OpaquePaintEvent);
}

PreviewWidget::~PreviewWidget() = default;

void PreviewWidget::setFrame(const QImage& frame)
{
    QMutexLocker locker(&m_frameMutex);
    
    if (frame.isNull() || !m_previewEnabled) {
        return;
    }
    
    m_currentFrame = frame;
    m_hasFrame = true;
    
    // Update scaled pixmap on main thread
    QMetaObject::invokeMethod(this, &PreviewWidget::updateScaledFrame, Qt::QueuedConnection);
}

void PreviewWidget::clearFrame()
{
    QMutexLocker locker(&m_frameMutex);
    m_currentFrame = QImage();
    m_scaledPixmap = QPixmap();
    m_hasFrame = false;
    update();
}

void PreviewWidget::setPreviewEnabled(bool enabled)
{
    m_previewEnabled = enabled;
    if (!enabled) {
        clearFrame();
    }
    update();
}

bool PreviewWidget::isPreviewEnabled() const
{
    return m_previewEnabled;
}

void PreviewWidget::onFrameReceived(const QImage& frame)
{
    setFrame(frame);
}

void PreviewWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QRect rect = event->rect();
    
    // Fill background
    painter.fillRect(rect, QColor(43, 43, 43));
    
    if (!m_previewEnabled) {
        // Draw disabled message
        painter.setPen(QColor(128, 128, 128));
        QFont uiFont;
        uiFont.setFamilies({"SF Pro Display", "Segoe UI", "Arial", "sans-serif"});
        uiFont.setPointSize(14);
        painter.setFont(uiFont);
        painter.drawText(rect, Qt::AlignCenter, m_disabledText);
        return;
    }
    
    QMutexLocker locker(&m_frameMutex);
    
    if (!m_hasFrame || m_scaledPixmap.isNull()) {
        // Draw no preview message
        painter.setPen(QColor(128, 128, 128));
        QFont uiFont;
        uiFont.setFamilies({"SF Pro Display", "Segoe UI", "Arial", "sans-serif"});
        uiFont.setPointSize(14);
        painter.setFont(uiFont);
        painter.drawText(rect, Qt::AlignCenter, m_noPreviewText);
        return;
    }
    
    // Draw scaled frame centered
    QRect pixmapRect = m_scaledPixmap.rect();
    QRect targetRect = rect;
    
    // Center the pixmap
    int x = (targetRect.width() - pixmapRect.width()) / 2;
    int y = (targetRect.height() - pixmapRect.height()) / 2;
    
    painter.drawPixmap(x, y, m_scaledPixmap);
    
    // Draw border around frame
    painter.setPen(QPen(QColor(85, 85, 85), 1));
    painter.drawRect(x - 1, y - 1, pixmapRect.width() + 1, pixmapRect.height() + 1);
}

void PreviewWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateScaledFrame();
}

void PreviewWidget::updateScaledFrame()
{
    QMutexLocker locker(&m_frameMutex);
    
    if (!m_hasFrame || m_currentFrame.isNull()) {
        return;
    }
    
    // Calculate scaled size maintaining aspect ratio
    QSize frameSize = m_currentFrame.size();
    QSize widgetSize = size();
    
    if (frameSize.isEmpty() || widgetSize.isEmpty()) {
        return;
    }
    
    // Calculate scale factor to fit frame in widget while maintaining aspect ratio
    double scaleX = static_cast<double>(widgetSize.width()) / frameSize.width();
    double scaleY = static_cast<double>(widgetSize.height()) / frameSize.height();
    double scale = std::min(scaleX, scaleY);
    
    // Ensure we don't scale up small images too much
    scale = std::min(scale, 1.0);
    
    QSize scaledSize(
        static_cast<int>(frameSize.width() * scale),
        static_cast<int>(frameSize.height() * scale)
    );
    
    // Create scaled pixmap
    QImage scaledImage = m_currentFrame.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_scaledPixmap = QPixmap::fromImage(scaledImage);
    
    update();
}