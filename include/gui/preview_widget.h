#pragma once

#include <QtWidgets/QWidget>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtCore/QMutex>

class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget();

    void setFrame(const QImage& frame);
    void clearFrame();
    void setPreviewEnabled(bool enabled);
    bool isPreviewEnabled() const;

public slots:
    void onFrameReceived(const QImage& frame);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateScaledFrame();

    QImage m_currentFrame;
    QPixmap m_scaledPixmap;
    QMutex m_frameMutex;
    bool m_previewEnabled;
    bool m_hasFrame;
    
    // UI elements
    QString m_noPreviewText;
    QString m_disabledText;
};