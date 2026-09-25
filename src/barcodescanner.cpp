#include "barcodescanner.h"
#include "barcodecodec.h"

#include <QQuickWindow>
#include <QtConcurrent/QtConcurrentRun>

BarcodeScanner::BarcodeScanner(QObject *parent) : QObject(parent)
{
    m_timer.setInterval(400);
    connect(&m_timer, &QTimer::timeout, this, &BarcodeScanner::capturePreview);
    connect(&m_watcher, &QFutureWatcher<QVariantMap>::finished, this, [this]() {
        const QVariantMap result = m_watcher.result();
        m_busy = false;
        if (!m_active || m_frameGeneration != m_generation || result.isEmpty())
            return;
        if (result.contains("error"))
            emit scanError(result.value("error").toString());
        else
            emit found(result);
    });
}

void BarcodeScanner::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;
        ++m_generation;
        updateTimer();
        emit activeChanged();
    }
}

void BarcodeScanner::setViewfinder(QQuickItem *item)
{
    if (m_viewfinder != item) {
        m_viewfinder = item;
        ++m_generation;
        updateTimer();
        emit viewfinderChanged();
    }
}

void BarcodeScanner::updateTimer()
{
    if (m_active && m_viewfinder)
        m_timer.start();
    else
        m_timer.stop();
}

void BarcodeScanner::capturePreview()
{
    if (m_busy || !m_active || !m_viewfinder || !m_viewfinder->isVisible())
        return;
    QQuickWindow *window = m_viewfinder->window();
    if (!window || !window->isVisible() || window->width() <= 0 || window->height() <= 0)
        return;

    // Sailfish camera frames may be GPU-only buffers which QVideoFrame cannot
    // map. Like FoilAuth, read the rendered window and crop to the viewfinder.
    const QImage screen = window->grabWindow();
    if (screen.isNull())
        return;
    const QRectF rect = m_viewfinder->mapRectToScene(
                QRectF(0, 0, m_viewfinder->width(), m_viewfinder->height()));
    const qreal scaleX = qreal(screen.width()) / window->width();
    const qreal scaleY = qreal(screen.height()) / window->height();
    const QRect crop = QRectF(rect.x() * scaleX, rect.y() * scaleY,
                             rect.width() * scaleX, rect.height() * scaleY)
            .toAlignedRect().intersected(screen.rect());
    if (crop.isEmpty())
        return;
    QImage image = screen.copy(crop);
    if (image.width() > 1600 || image.height() > 1600)
        image = image.scaled(1600, 1600, Qt::KeepAspectRatio, Qt::FastTransformation);
    m_busy = true;
    m_frameGeneration = m_generation;
    m_watcher.setFuture(QtConcurrent::run(decodeCardBarcode, image));
}

bool BarcodeScanner::scanFile(const QUrl &source)
{
    if (!m_active || m_busy)
        return false;
    m_busy = true;
    m_frameGeneration = m_generation;
    m_watcher.setFuture(QtConcurrent::run(decodeCardBarcodeFile, source));
    return true;
}
