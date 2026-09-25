#ifndef BARCODECODEC_H
#define BARCODECODEC_H

#include <QImage>
#include <QVariantMap>
#include <QUrl>
#include <QObject>

class BarcodeCodec : public QObject
{
    Q_OBJECT
public:
    explicit BarcodeCodec(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE QVariantMap encodeMessage(const QString &message, const QString &encoding) const;
};

QImage renderPassBarcode(const QString &type, const QByteArray &bytes);

QVariantMap decodeCardBarcode(const QImage &image);
QVariantMap decodeCardBarcodeFile(const QUrl &source);

#endif
