#include <QtTest>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <QDir>
#include <QBuffer>
#include <ZXing/CreateBarcode.h>
#include <ZXing/WriteBarcode.h>
#include "../src/passsharer.h"
#include "../src/savedcards.h"
#include "../src/barcodecodec.h"
#include "../src/zipfile.h"
#include "../src/passimporter.h"

static QByteArray read(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

class PassSharerTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("PassViewerTests");
        QCoreApplication::setApplicationName("sharing");
    }
    void cleanup() {
        QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).removeRecursively();
        QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).removeRecursively();
    }
    void cardRoundTrip_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QByteArray>("payload");
        QTest::addColumn<bool>("gs1");
        QTest::addColumn<bool>("standard");
        QTest::newRow("QR") << QString("QRCode") << QByteArray("000123") << false << true;
        QTest::newRow("Aztec binary") << QString("Aztec") << QByteArray("A\0\xff\x80Z", 5) << false << true;
        QTest::newRow("PDF417") << QString("PDF417") << QByteArray("SYNTHETIC") << false << true;
        QTest::newRow("Code128") << QString("Code128") << QByteArray("000000124") << false << true;
        QTest::newRow("EAN image") << QString("EAN13") << QByteArray("5901234123457") << false << false;
        QTest::newRow("GS1 image") << QString("Code128") << QByteArray("(01)09501101530003(10)ABC123") << true << false;
        QTest::newRow("ECI image") << QString("QRCode") << QByteArray("ECI marker") << false << false;
    }
    void cardRoundTrip() {
        QFETCH(QString, format);
        QFETCH(QByteArray, payload);
        QFETCH(bool, gs1);
        QFETCH(bool, standard);
        const auto options = ZXing::CreatorOptions(ZXing::BarcodeFormatFromString(format.toStdString()),
                             gs1 ? "gs1" : QByteArray(QTest::currentDataTag()) == "ECI image" ? "eci=899" : "eci=0");
        const auto code = gs1 ? ZXing::CreateBarcodeFromText(payload.toStdString(), options)
                             : ZXing::CreateBarcodeFromBytes(payload.constData(), payload.size(), options);
        const auto rendered = ZXing::WriteBarcodeToImage(code, ZXing::WriterOptions().scale(3).addQuietZones(true));
        const QImage image(rendered.data(), rendered.width(), rendered.height(), rendered.width(), QImage::Format_Grayscale8);
        QVariantMap barcode = decodeCardBarcode(image);
        // Save the synthetic input symbol exactly, including its ECI markers.
        QByteArray originalPng;
        QBuffer originalBuffer(&originalPng);
        QVERIFY(originalBuffer.open(QIODevice::WriteOnly));
        QVERIFY(image.save(&originalBuffer, "PNG"));
        barcode.insert("png", QString::fromLatin1(originalPng.toBase64()));
        QVERIFY2(!barcode.contains("error") && !barcode.isEmpty(), qPrintable(barcode.value("error").toString()));
        SavedCards cards;
        const QString id = cards.save("Test / card", barcode);
        QVERIFY2(!id.isEmpty(), qPrintable(cards.error()));
        QImage icon(256, 256, QImage::Format_RGB32);
        icon.fill(Qt::red);
        QByteArray iconPng;
        QBuffer buffer(&iconPng);
        QVERIFY(buffer.open(QIODevice::WriteOnly));
        QVERIFY(icon.save(&buffer, "PNG"));
        QVERIFY(cards.setIcon(id, {{"png", QString::fromLatin1(iconPng.toBase64())},
                                  {"author", "Synthetic author"}, {"license", "CC0"}}));
        PassSharer sharer(&cards);
        const QString path = sharer.prepare("card:" + id);
        QVERIFY2(!path.isEmpty(), qPrintable(sharer.error()));
        QVERIFY(QFileInfo(path).fileName().endsWith(".pkpass"));
        QVERIFY(!QFileInfo(path).fileName().contains('/'));
        const QByteArray snapshot = read(path);
        QVERIFY(!snapshot.isEmpty());
        QCOMPARE(sharer.prepare("card:" + id), path);
        ZipFile zip(path);
        QVERIFY(zip.isValid());
        const auto pass = QJsonDocument::fromJson(zip.getFile("pass.json")).object();
        QCOMPARE(pass.value("description").toString(), QString("Test / card"));
        QCOMPARE(pass.value("serialNumber").toString(), id);
        QCOMPARE(pass.contains("barcode"), standard);
        QCOMPARE(zip.getFileList().contains("strip.png"), !standard);
        QCOMPARE(zip.getFile("barcode.png"), QByteArray::fromBase64(barcode.value("png").toByteArray()));
        QCOMPARE(decodeCardBarcode(QImage::fromData(zip.getFile("barcode.png"))).value("bytes"), barcode.value("bytes"));
        QVERIFY(!zip.getFileList().contains("signature"));
        QCOMPARE(zip.getFile("icon.png"), iconPng);
        QCOMPARE(pass.value("userInfo").toObject().value("iconCredits").toObject().value("license").toString(), QString("CC0"));
        if (standard) {
            BarcodeCodec codec;
            const auto value = pass.value("barcode").toObject();
            QCOMPARE(codec.encodeMessage(value.value("message").toString(), value.value("messageEncoding").toString())
                     .value("content"), barcode.value("bytes"));
        }
        const auto manifest = QJsonDocument::fromJson(zip.getFile("manifest.json")).object();
        for (const QString &file : manifest.keys())
            QCOMPARE(manifest.value(file).toString(), QString::fromLatin1(QCryptographicHash::hash(zip.getFile(file), QCryptographicHash::Sha1).toHex()));
        QVERIFY(!(QFileInfo(path).permissions() & (QFile::ReadGroup | QFile::ReadOther | QFile::WriteOther)));
        PassImporter importer;
        importer.prepare(path);
        QTRY_VERIFY(!importer.busy());
        QVERIFY2(importer.error().isEmpty(), qPrintable(importer.error()));
        QSignalSpy imported(&importer, &PassImporter::imported);
        importer.importPasses();
        QTRY_VERIFY(!importer.busy());
        QCOMPARE(imported.size(), 1);
        QCOMPARE(read(imported.first().first().toStringList().first()), snapshot);
        // Sharing imported packages preserves all bytes, including any signature.
        QTemporaryDir originals;
        const QString original = QDir(originals.path()).filePath("original.pkpass");
        QVERIFY(QFile::copy(path, original));
        const QString shared = sharer.prepare(QUrl::fromLocalFile(original).toString());
        QCOMPARE(read(shared), snapshot);
        QVERIFY(QFile::remove(original));
        QVERIFY(cards.remove(id));
        QCOMPARE(read(shared), snapshot);
    }
    void writeFailure() {
        SavedCards cards;
        const QString id = cards.save("Test", decodeCardBarcode(renderPassBarcode("qr", "TEST")));
        QVERIFY2(!id.isEmpty(), qPrintable(cards.error()));
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QVERIFY(QDir().mkpath(directory));
        QFile blocker(directory + "/shared-passes");
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        PassSharer sharer(&cards);
        QVERIFY(sharer.prepare("card:" + id).isEmpty());
        QVERIFY(!sharer.error().isEmpty());
        QVERIFY(!cards.card(id).isEmpty());
    }
    void errors() {
        SavedCards cards;
        PassSharer sharer(&cards);
        QVERIFY(sharer.prepare("card:missing").isEmpty());
        QVERIFY(!sharer.error().isEmpty());
        QVERIFY(sharer.prepare("/nonexistent.pkpass").isEmpty());
        QVERIFY(!sharer.error().isEmpty());
    }
};
int main(int argc, char **argv)
{
    QTemporaryDir storage;
    if (!storage.isValid())
        return 1;
    qputenv("XDG_DATA_HOME", (storage.path() + "/data").toUtf8());
    qputenv("XDG_CACHE_HOME", (storage.path() + "/cache").toUtf8());
    QCoreApplication app(argc, argv);
    PassSharerTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_passsharer.moc"
