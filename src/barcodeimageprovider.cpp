#include "barcodeimageprovider.h"
#include "barcodecodec.h"

BarcodeImageProvider::BarcodeImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap BarcodeImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &)
{
    *size = QSize();
    const int firstSlash = id.indexOf('/');
    const int secondSlash = id.indexOf('/', firstSlash + 1);
    if (firstSlash < 0 || secondSlash < 0)
        return {};
    const QString type = id.left(firstSlash);
    const QByteArray encoding = id.mid(firstSlash + 1, secondSlash - firstSlash - 1).toLatin1();
    const QByteArray base64 = id.mid(secondSlash + 1).toLatin1();
    if (base64.size() > 11000 || !QTextCodec::codecForName(encoding))
        return {};
    const QByteArray bytes = QByteArray::fromBase64(base64);
    if (bytes.toBase64() != base64)
        return {};
    const QImage image = renderPassBarcode(type, bytes);
    *size = image.size();
    return QPixmap::fromImage(image);
}
