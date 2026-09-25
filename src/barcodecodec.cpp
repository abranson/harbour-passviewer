#include "barcodecodec.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QTextCodec>
#include <ZXing/ReadBarcode.h>
#include <ZXing/CreateBarcode.h>
#include <ZXing/WriteBarcode.h>

QVariantMap BarcodeCodec::encodeMessage(const QString &message, const QString &encoding) const
{
    const QString name = encoding.trimmed().isEmpty() ? QStringLiteral("ISO-8859-1") : encoding.trimmed();
    QTextCodec *codec = QTextCodec::codecForName(name.toLatin1());
    if (!codec)
        return {{"error", tr("This barcode uses an unsupported text encoding.")}};
    // Convert once, from the JSON string directly to the declared bytes. Reject
    // unrepresentable characters instead of silently substituting question marks.
    QTextCodec::ConverterState state(QTextCodec::IgnoreHeader);
    const QByteArray bytes = codec->fromUnicode(message.constData(), message.size(), &state);
    if (state.invalidChars || state.remainingChars || codec->toUnicode(bytes) != message)
        return {{"error", tr("The barcode message does not match its declared encoding.")}};
    if (bytes.isEmpty() || bytes.size() > 8000)
        return {{"error", tr("The barcode message is empty or too large.")}};
    return {{"content", QString::fromLatin1(bytes.toBase64())},
            {"encoding", QString::fromLatin1(codec->name())}};
}

QImage renderPassBarcode(const QString &type, const QByteArray &bytes)
{
    ZXing::BarcodeFormat format;
    if (type == "qr")
        format = ZXing::BarcodeFormat::QRCode;
    else if (type == "aztec")
        format = ZXing::BarcodeFormat::Aztec;
    else if (type == "pdf417")
        format = ZXing::BarcodeFormat::PDF417;
    else if (type == "code128")
        format = ZXing::BarcodeFormat::Code128;
    else
        return {};
    if (bytes.isEmpty() || bytes.size() > 8000)
        return {};
    try {
        // pkpass supplies bytes via messageEncoding, not GS1/ECI metadata. Disable
        // ZXing's automatic binary ECI marker; never guess GS1 from the payload.
        const auto barcode = ZXing::CreateBarcodeFromBytes(bytes.constData(), bytes.size(),
                                                       ZXing::CreatorOptions(format, "eci=0"));
        const auto image = ZXing::WriteBarcodeToImage(barcode, ZXing::WriterOptions().scale(3).addQuietZones(true));
        if (!image.data())
            return {};
        const auto check = ZXing::ReadBarcode(image, ZXing::ReaderOptions().tryHarder(true));
        const auto decoded = check.bytes();
        if (!check.isValid() || check.format() != format || check.contentType() == ZXing::ContentType::GS1
                || QByteArray(reinterpret_cast<const char *>(decoded.data()), int(decoded.size())) != bytes)
            return {};
        return QImage(image.data(), image.width(), image.height(), image.width(), QImage::Format_Grayscale8).copy();
    } catch (const std::exception &) {
        return {};
    }
}

QVariantMap decodeCardBarcodeFile(const QUrl &source)
{
    if (!source.isLocalFile())
        return {{"error", QObject::tr("Please select an image stored on the device.")}};
    const QFileInfo file(source.toLocalFile());
    if (!file.isFile() || file.size() > 64 * 1024 * 1024)
        return {{"error", QObject::tr("The image is unavailable or too large to open.")}};
    QImageReader reader(file.absoluteFilePath());
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (!size.isValid() || qint64(size.width()) * size.height() > 64000000)
        return {{"error", QObject::tr("This image format or size is not supported.")}};
    if (size.width() > 4096 || size.height() > 4096)
        reader.setScaledSize(size.scaled(4096, 4096, Qt::KeepAspectRatio));
    const QImage image = reader.read();
    if (image.isNull())
        return {{"error", QObject::tr("Could not open the selected image.")}};
    const auto result = decodeCardBarcode(image);
    return result.isEmpty()
            ? QVariantMap{{"error", QObject::tr("No barcode found. Try a clearer image or crop it around the code.")}}
            : result;
}

QVariantMap decodeCardBarcode(const QImage &image)
{
    if (image.isNull())
        return {};
    try {
        const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
        const ZXing::ImageView view(gray.constBits(), gray.width(), gray.height(),
                                    ZXing::ImageFormat::Lum, gray.bytesPerLine());
        const auto options = ZXing::ReaderOptions().tryInvert(true).tryHarder(true);
        const auto barcode = ZXing::ReadBarcode(view, options);
        if (!barcode.isValid())
            return {};
        if (barcode.sequenceSize() > 1)
            return {{"error", QObject::tr("Codes split across multiple symbols cannot be saved.")}};

        const auto bytes = barcode.bytes();
        const auto writer = ZXing::WriterOptions().scale(3).addQuietZones(true);
        const auto rendered = [&]() {
            // Matrix readers retain the module pattern, including ECI/GS1 data.
            if (barcode.symbol().data())
                return ZXing::WriteBarcodeToImage(barcode, writer);
            // Linear readers return data only. Preserve bytes (and GS1 semantics)
            // and verify the resulting image before allowing the card to be saved.
            const auto creator = ZXing::CreatorOptions(barcode.format(),
                                barcode.contentType() == ZXing::ContentType::GS1 ? "gs1" : "");
            const auto recreated = barcode.contentType() == ZXing::ContentType::GS1
                    ? ZXing::CreateBarcodeFromText(barcode.text(ZXing::TextMode::HRI), creator)
                    : ZXing::CreateBarcodeFromBytes(bytes.data(), int(bytes.size()), creator);
            return ZXing::WriteBarcodeToImage(recreated, writer);
        }();
        if (!rendered.data() || rendered.width() <= 0 || rendered.height() <= 0)
            return {{"error", QObject::tr("This barcode format cannot be saved.")}};
        const QImage symbol(rendered.data(), rendered.width(), rendered.height(),
                            rendered.width(), QImage::Format_Grayscale8);
        const auto check = ZXing::ReadBarcode(rendered, options);
        if (!check.isValid() || check.format() != barcode.format()
                || check.bytes() != barcode.bytes()
                || check.symbologyIdentifier() != barcode.symbologyIdentifier())
            return {{"error", QObject::tr("The scanned code could not be reproduced reliably. Please scan it again.")}};

        QByteArray png;
        QBuffer buffer(&png);
        if (!buffer.open(QIODevice::WriteOnly) || !symbol.save(&buffer, "PNG"))
            return {{"error", QObject::tr("Could not prepare the barcode image.")}};
        return {{"format", QString::fromStdString(ZXing::ToString(barcode.format()))},
                {"text", QString::fromStdString(barcode.text())},
                {"bytes", QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(bytes.data()),
                                                        int(bytes.size())).toBase64())},
                {"png", QString::fromLatin1(png.toBase64())}};
    } catch (const std::exception &) {
        return {{"error", QObject::tr("Could not read this barcode. Please try again.")}};
    }
}
