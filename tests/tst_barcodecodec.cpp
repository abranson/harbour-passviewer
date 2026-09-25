#include <QtTest>
#include <QImage>
#include <QPainter>
#include <ZXing/CreateBarcode.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/WriteBarcode.h>
#include "../src/barcodecodec.h"
#include "../src/barcodeimageprovider.h"

class BarcodeCodecTest : public QObject
{
    Q_OBJECT
private slots:
    void importedPass_data();
    void importedPass();
    void invalidPassEncoding();
    void roundTrip_data();
    void roundTrip();
    void blankFrame();
    void rotatedFrame();
    void gs1();
    void binaryQr();
    void imageFile_data();
    void imageFile();
    void invalidImageFile();
};

static QImage barcodeImage(const QString &format, const QByteArray &payload)
{
    const auto barcode = ZXing::CreateBarcodeFromText(std::string(payload.constData(), payload.size()),
                              ZXing::CreatorOptions(ZXing::BarcodeFormatFromString(format.toStdString())));
    const auto image = ZXing::WriteBarcodeToImage(barcode, ZXing::WriterOptions().scale(4).addQuietZones(true));
    return QImage(image.data(), image.width(), image.height(), image.width(), QImage::Format_Grayscale8).copy();
}

void BarcodeCodecTest::importedPass_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("encoding");
    QTest::addColumn<QString>("message");
    QTest::addColumn<QByteArray>("expected");
    for (const QString type : {"qr", "aztec", "pdf417", "code128"}) {
        const QByteArray binary = QByteArray::fromHex("0041424380e9ff001d") + "123456789";
        QTest::newRow((type + " binary").toLatin1().constData()) << type << QString("iso-8859-1")
            << QString::fromLatin1(binary.constData(), binary.size()) << binary;
        QTest::newRow((type + " numeric not GS1").toLatin1().constData()) << type << QString("latin1")
            << QString("12345678901234567890") << QByteArray("12345678901234567890");
    }
    QByteArray rail;
    for (int i = 0; i < 900; ++i)
        rail.append(char((i * 73 + i / 17) & 255));
    QTest::newRow("Aztec long binary") << QString("aztec") << QString("ISO-8859-1")
        << QString::fromLatin1(rail.constData(), rail.size()) << rail;
    QByteArray allBytes;
    for (int i = 0; i < 256; ++i)
        allBytes.append(char(i));
    QTest::newRow("Aztec every byte") << QString("aztec") << QString("ISO-8859-1")
        << QString::fromLatin1(allBytes.constData(), allBytes.size()) << allBytes;
    const QString unicode = QString::fromUtf8("Carte été 日本語 €");
    for (const QString type : {"qr", "aztec", "pdf417"})
        QTest::newRow((type + " UTF-8").toLatin1().constData()) << type << QString("UTF-8") << unicode << unicode.toUtf8();
    QTest::newRow("windows1252") << QString("qr") << QString("Windows-1252")
        << QString::fromUtf8("€") << QByteArray::fromHex("80");
}

void BarcodeCodecTest::importedPass()
{
    QFETCH(QString, type);
    QFETCH(QString, encoding);
    QFETCH(QString, message);
    QFETCH(QByteArray, expected);
    BarcodeCodec codec;
    const auto result = codec.encodeMessage(message, encoding);
    QVERIFY2(!result.contains("error"), qPrintable(result.value("error").toString()));
    QCOMPARE(QByteArray::fromBase64(result.value("content").toByteArray()), expected);
    // Exercise exactly the provider URL used in full, simple and import views.
    BarcodeImageProvider provider;
    QSize size;
    const auto pixmap = provider.requestPixmap(type + "/" + result.value("encoding").toString()
                                               + "/" + result.value("content").toString(), &size, {});
    QVERIFY(!pixmap.isNull());
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_Grayscale8);
    QCOMPARE(image.size(), size);
    const auto check = ZXing::ReadBarcode({image.constBits(), image.width(), image.height(),
                                          ZXing::ImageFormat::Lum, image.bytesPerLine()},
                                          ZXing::ReaderOptions().tryHarder(true));
    QVERIFY(check.isValid());
    const auto bytes = check.bytes();
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(bytes.data()), int(bytes.size())), expected);
    QVERIFY(check.contentType() != ZXing::ContentType::GS1);
    QVERIFY(!check.hasECI());
    const auto format = type == "qr" ? ZXing::BarcodeFormat::QRCode
                      : type == "aztec" ? ZXing::BarcodeFormat::Aztec
                      : type == "pdf417" ? ZXing::BarcodeFormat::PDF417 : ZXing::BarcodeFormat::Code128;
    QVERIFY(check.format() == format);
}

void BarcodeCodecTest::invalidPassEncoding()
{
    BarcodeCodec codec;
    QVERIFY(codec.encodeMessage(QString::fromUtf8("€"), "ISO-8859-1").contains("error"));
    QVERIFY(codec.encodeMessage("ABC", "not-a-codec").contains("error"));
    QVERIFY(codec.encodeMessage(QString(QChar(0xd800)), "UTF-8").contains("error"));
    QVERIFY(codec.encodeMessage("", "UTF-8").contains("error"));
    QVERIFY(codec.encodeMessage(QString(8001, 'a'), "UTF-8").contains("error"));
    QCOMPARE(codec.encodeMessage("ABC", "").value("content").toString(), QString("QUJD"));
    QVERIFY(renderPassBarcode("unsupported", "ABC").isNull());
    QVERIFY(renderPassBarcode("aztec", {}).isNull());
    BarcodeImageProvider provider;
    QSize size;
    QVERIFY(provider.requestPixmap("qr/ISO-8859-1/!invalid", &size, {}).isNull());
}

void BarcodeCodecTest::roundTrip_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QByteArray>("payload");
    QTest::newRow("QR ASCII") << QString("QRCode") << QByteArray("00123456");
    QTest::newRow("QR UTF-8") << QString("QRCode") << QString::fromUtf8("Carte été 日本語").toUtf8();
    QTest::newRow("Code128 zeroes") << QString("Code128") << QByteArray("0000123456789");
    QTest::newRow("Code39") << QString("Code39") << QByteArray("STORE-001");
    QTest::newRow("EAN13") << QString("EAN13") << QByteArray("5901234123457");
    QTest::newRow("EAN8") << QString("EAN8") << QByteArray("96385074");
    QTest::newRow("UPCA") << QString("UPCA") << QByteArray("012345678905");
    QTest::newRow("ITF") << QString("ITF") << QByteArray("001234567890");
    QTest::newRow("Aztec") << QString("Aztec") << QByteArray("LOYALTY-000123");
    QTest::newRow("DataMatrix") << QString("DataMatrix") << QByteArray("MEMBER-000456");
    QTest::newRow("PDF417") << QString("PDF417") << QByteArray("CARD-000789");
}

void BarcodeCodecTest::roundTrip()
{
    QFETCH(QString, format);
    QFETCH(QByteArray, payload);
    const QImage input = barcodeImage(format, payload);
    QVERIFY(!input.isNull());
    const QVariantMap result = decodeCardBarcode(input);
    QVERIFY2(!result.isEmpty() && !result.contains("error"), qPrintable(result.value("error").toString()));
    // Retail readers can normalize UPC-A to EAN-13. Compare raw decoded data of
    // the original to the saved symbol, rather than its human-readable label.
    const auto original = ZXing::ReadBarcode({input.constBits(), input.width(), input.height(),
                                              ZXing::ImageFormat::Lum, input.bytesPerLine()});
    const auto bytes = original.bytes();
    const QByteArray expected(reinterpret_cast<const char *>(bytes.data()), int(bytes.size()));
    QCOMPARE(QByteArray::fromBase64(result.value("bytes").toByteArray()), expected);
    if (format != "UPCA")
        QCOMPARE(expected, payload);
    QImage stored = QImage::fromData(QByteArray::fromBase64(result.value("png").toByteArray()), "PNG")
                          .convertToFormat(QImage::Format_Grayscale8);
    QVERIFY(!stored.isNull());
    const auto decoded = ZXing::ReadBarcode({stored.constBits(), stored.width(), stored.height(),
                                             ZXing::ImageFormat::Lum, stored.bytesPerLine()});
    QVERIFY(decoded.isValid());
    QVERIFY(decoded.bytes() == original.bytes());
    QVERIFY(decoded.format() == original.format());
    QCOMPARE(QString::fromStdString(decoded.symbologyIdentifier()), QString::fromStdString(original.symbologyIdentifier()));
}

void BarcodeCodecTest::blankFrame()
{
    QImage blank(600, 600, QImage::Format_RGB32);
    blank.fill(Qt::white);
    QVERIFY(decodeCardBarcode(blank).isEmpty());
    QVERIFY(decodeCardBarcode(QImage()).isEmpty());
}

void BarcodeCodecTest::rotatedFrame()
{
    const QImage input = barcodeImage("Code128", "0000123456789");
    const QImage rotated = input.transformed(QTransform().rotate(90));
    const auto result = decodeCardBarcode(rotated);
    QVERIFY2(!result.isEmpty() && !result.contains("error"), qPrintable(result.value("error").toString()));
    QCOMPARE(QByteArray::fromBase64(result.value("bytes").toByteArray()), QByteArray("0000123456789"));
}

void BarcodeCodecTest::gs1()
{
    const auto code = ZXing::CreateBarcodeFromText("(01)09501101530003(10)LOT123",
                           ZXing::CreatorOptions(ZXing::BarcodeFormat::Code128, "gs1"));
    const auto image = ZXing::WriteBarcodeToImage(code, ZXing::WriterOptions().scale(4).addQuietZones(true));
    const QImage input(image.data(), image.width(), image.height(), image.width(), QImage::Format_Grayscale8);
    const auto result = decodeCardBarcode(input);
    QVERIFY2(!result.isEmpty() && !result.contains("error"), qPrintable(result.value("error").toString()));
    QImage stored = QImage::fromData(QByteArray::fromBase64(result.value("png").toByteArray()), "PNG")
                          .convertToFormat(QImage::Format_Grayscale8);
    const auto decoded = ZXing::ReadBarcode({stored.constBits(), stored.width(), stored.height(),
                                             ZXing::ImageFormat::Lum, stored.bytesPerLine()});
    QCOMPARE(QString::fromStdString(decoded.symbologyIdentifier()), QString("]C1"));
    QCOMPARE(QByteArray::fromBase64(result.value("bytes").toByteArray()), QByteArray("010950110153000310LOT123"));
}

void BarcodeCodecTest::binaryQr()
{
    const QByteArray bytes = QByteArray::fromHex("000102ff805a");
    const auto code = ZXing::CreateBarcodeFromBytes(bytes.constData(), bytes.size(),
                                                   ZXing::CreatorOptions(ZXing::BarcodeFormat::QRCode));
    const auto image = ZXing::WriteBarcodeToImage(code, ZXing::WriterOptions().scale(4).addQuietZones(true));
    const QImage input(image.data(), image.width(), image.height(), image.width(), QImage::Format_Grayscale8);
    const auto result = decodeCardBarcode(input);
    QVERIFY2(!result.isEmpty() && !result.contains("error"), qPrintable(result.value("error").toString()));
    QCOMPARE(QByteArray::fromBase64(result.value("bytes").toByteArray()), bytes);
}

void BarcodeCodecTest::imageFile_data()
{
    QTest::addColumn<QString>("extension");
    QTest::newRow("screenshot") << QString("png");
    QTest::newRow("photo") << QString("jpg");
}

void BarcodeCodecTest::imageFile()
{
    QFETCH(QString, extension);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("card." + extension);
    QImage screenshot(1080, 1920, QImage::Format_RGB32);
    screenshot.fill(Qt::white);
    {
        QPainter painter(&screenshot);
        painter.drawImage(120, 800, barcodeImage("QRCode", "0000123456789"));
    }
    QVERIFY(screenshot.save(path));
    const auto result = decodeCardBarcodeFile(QUrl::fromLocalFile(path));
    QVERIFY2(!result.contains("error"), qPrintable(result.value("error").toString()));
    QCOMPARE(QByteArray::fromBase64(result.value("bytes").toByteArray()), QByteArray("0000123456789"));
}

void BarcodeCodecTest::invalidImageFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto url = QUrl::fromLocalFile(directory.filePath("image.png"));
    QVERIFY(decodeCardBarcodeFile(url).contains("error"));
    QVERIFY(decodeCardBarcodeFile(QUrl("https://example.org/image.png")).contains("error"));
    QFile corrupt(url.toLocalFile());
    QVERIFY(corrupt.open(QIODevice::WriteOnly));
    corrupt.write("invalid image");
    corrupt.close();
    QVERIFY(decodeCardBarcodeFile(url).contains("error"));
    QImage blank(200, 200, QImage::Format_RGB32);
    blank.fill(Qt::white);
    QVERIFY(blank.save(url.toLocalFile()));
    QVERIFY(decodeCardBarcodeFile(url).contains("error"));
}

QTEST_MAIN(BarcodeCodecTest)
#include "tst_barcodecodec.moc"
