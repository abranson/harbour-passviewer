#ifndef BARCODEIMAGEPROVIDER_H
#define BARCODEIMAGEPROVIDER_H

#include <QByteArray>
#include <QQuickImageProvider>
#include <QTextCodec>
#include <QImage>
#include <QPixmap>

class BarcodeImageProvider : public QQuickImageProvider
{
public:
    explicit BarcodeImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);

signals:

public slots:

};

#endif // BARCODEIMAGEPROVIDER_H
