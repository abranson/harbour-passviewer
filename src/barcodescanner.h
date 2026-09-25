#ifndef BARCODESCANNER_H
#define BARCODESCANNER_H

#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>
#include <QVariantMap>
#include <QUrl>

// Captures the rendered preview and decodes off the UI thread, one frame at a time.
class BarcodeScanner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QQuickItem *viewfinder READ viewfinder WRITE setViewfinder NOTIFY viewfinderChanged)
public:
    explicit BarcodeScanner(QObject *parent = nullptr);
    bool isActive() const { return m_active; }
    void setActive(bool active);
    QQuickItem *viewfinder() const { return m_viewfinder.data(); }
    void setViewfinder(QQuickItem *item);

public slots:
    bool scanFile(const QUrl &source);

signals:
    void activeChanged();
    void viewfinderChanged();
    void found(const QVariantMap &barcode);
    void scanError(const QString &message);

private:
    void capturePreview();
    void updateTimer();

    bool m_active = true;
    bool m_busy = false;
    unsigned m_generation = 0;
    unsigned m_frameGeneration = 0;
    QPointer<QQuickItem> m_viewfinder;
    QTimer m_timer;
    QFutureWatcher<QVariantMap> m_watcher;
};

#endif
