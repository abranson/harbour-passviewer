#include <QtTest>
#include <QDataStream>
#include <QTemporaryDir>
#include <QUrl>
#include "../src/passimporter.h"
#include "../src/homescanner.h"

// Small generated ZIPs, including raw DEFLATE as used by real pass bundles.
static QByteArray zip(const QList<QPair<QByteArray, QByteArray>> &entries, bool deflate = false)
{
    QByteArray bytes, directory;
    QDataStream out(&bytes, QIODevice::WriteOnly), central(&directory, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    central.setByteOrder(QDataStream::LittleEndian);
    for (const auto &entry : entries) {
        const QByteArray name = entry.first, data = entry.second;
        QByteArray compressed = data;
        if (deflate) {
            compressed.resize(compressBound(data.size()));
            z_stream stream = {};
            deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
            stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.constData()));
            stream.avail_in = data.size();
            stream.next_out = reinterpret_cast<Bytef *>(compressed.data());
            stream.avail_out = compressed.size();
            ::deflate(&stream, Z_FINISH);
            compressed.resize(stream.total_out);
            deflateEnd(&stream);
        }
        const quint32 crc = crc32(0, reinterpret_cast<const Bytef *>(data.constData()), data.size());
        const quint32 offset = bytes.size();
        out << quint32(0x04034b50) << quint16(20) << quint16(0) << quint16(deflate ? 8 : 0)
            << quint16(0) << quint16(0) << crc << quint32(compressed.size()) << quint32(data.size())
            << quint16(name.size()) << quint16(0);
        out.writeRawData(name.constData(), name.size());
        out.writeRawData(compressed.constData(), compressed.size());
        central << quint32(0x02014b50) << quint16(20) << quint16(20) << quint16(0) << quint16(deflate ? 8 : 0)
                << quint16(0) << quint16(0) << crc << quint32(compressed.size()) << quint32(data.size())
                << quint16(name.size()) << quint16(0) << quint16(0) << quint16(0) << quint16(0)
                << quint32(0) << offset;
        central.writeRawData(name.constData(), name.size());
    }
    const quint32 directoryOffset = bytes.size();
    out.writeRawData(directory.constData(), directory.size());
    out << quint32(0x06054b50) << quint16(0) << quint16(0) << quint16(entries.size()) << quint16(entries.size())
        << quint32(directory.size()) << directoryOffset << quint16(0);
    return bytes;
}

static QByteArray pass(const QString &serial, const QString &name = "Test pass")
{
    return zip({{"pass.json", QJsonDocument(QJsonObject{{"passTypeIdentifier", "pass.test"},
                {"serialNumber", serial}, {"description", name}, {"generic", QJsonObject()}}).toJson()}}, true);
}

static bool write(const QString &path, const QByteArray &bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}

static QByteArray read(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

static QString library()
{
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/passes";
}

class PassImporterTest : public QObject
{
    Q_OBJECT
private slots:
    void cleanup() { QDir(library()).removeRecursively(); }
    void confirmationAndPersistence();
    void bundleAndDuplicate();
    void invalidBundle();
    void failedWrite();
    void boundedZipRead();
};

void PassImporterTest::confirmationAndPersistence()
{
    QTemporaryDir sources;
    const QString source = sources.filePath("a # percent %.pkpass");
    const QByteArray original = pass("one");
    QVERIFY(write(source, original));
    HomeScanner reader;
    QSignalSpy found(&reader, &HomeScanner::passesFound);
    reader.scanHome();
    QVERIFY(found.takeFirst()[0].toList().isEmpty());
    QString staged;
    {
        PassImporter cancelled;
        cancelled.prepare(QUrl::fromLocalFile(source).toString());
        QTRY_VERIFY(!cancelled.busy());
        QVERIFY2(cancelled.error().isEmpty(), qPrintable(cancelled.error()));
        QCOMPARE(cancelled.passes().size(), 1);
        staged = cancelled.passes().first().toMap().value("path").toString();
        QVERIFY(QFile::exists(staged));
        QVERIFY(QDir(library()).entryList({"*.pkpass"}).isEmpty());
    }
    QVERIFY(!QFile::exists(staged));
    QCOMPARE(read(source), original);

    PassImporter importer;
    importer.prepare(source);
    QTRY_VERIFY(!importer.busy());
    QVERIFY(importer.error().isEmpty());
    // Import the previewed snapshot even if the download changes before acceptance.
    QVERIFY(write(source, pass("different")));
    QSignalSpy imported(&importer, &PassImporter::imported);
    importer.importPasses();
    QTRY_VERIFY(!importer.busy());
    QCOMPARE(imported.count(), 1);
    const QString target = imported.first()[0].toStringList().first();
    QCOMPARE(read(target), original);
    QVERIFY(target.startsWith(library() + "/"));
    QVERIFY(!(QFile::permissions(target) & (QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther)));
    QVERIFY(QFile::remove(source));
    reader.scanHome();
    const auto rows = found.takeFirst()[0].toList();
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.first().toMap().value("name").toString(), QString("Test pass"));
    QCOMPARE(rows.first().toMap().value("path").toString(), target);
}

void PassImporterTest::bundleAndDuplicate()
{
    QTemporaryDir sources;
    const QString source = sources.filePath("bundle.PKPASSES");
    const QByteArray one = pass("one"), two = pass("two");
    QVERIFY(write(source, zip({{"../outside.pkpass", one}, {"nested/two.pkpass", two}}, true)));
    PassImporter importer;
    importer.prepare(source);
    QTRY_VERIFY(!importer.busy());
    QVERIFY2(importer.error().isEmpty(), qPrintable(importer.error()));
    QCOMPARE(importer.passes().size(), 2);
    importer.importPasses();
    QTRY_VERIFY(!importer.busy());
    QVERIFY(importer.error().isEmpty());
    QCOMPARE(QDir(library()).entryList({"*.pkpass"}, QDir::Files).size(), 2);
    QVERIFY(!QFile::exists(sources.filePath("outside.pkpass")));
    const QString firstTarget = importer.passes().first().toMap().value("target").toString();
    const QByteArray updated = pass("one", "Updated pass");
    const QString single = sources.filePath("update.pkpass");
    QVERIFY(write(single, updated));
    importer.prepare(single);
    QTRY_VERIFY(!importer.busy());
    QVERIFY(importer.passes().first().toMap().value("replaces").toBool());
    QCOMPARE(importer.passes().first().toMap().value("target").toString(), firstTarget);
    importer.importPasses();
    QTRY_VERIFY(!importer.busy());
    QCOMPARE(read(firstTarget), updated);
    QCOMPARE(QDir(library()).entryList({"*.pkpass"}, QDir::Files).size(), 2);
    QCOMPARE(read(single), updated);
}

void PassImporterTest::invalidBundle()
{
    QTemporaryDir sources;
    const QString source = sources.filePath("bad.pkpasses");
    QVERIFY(write(source, zip({{"good.pkpass", pass("one")}, {"invalid.pkpass", "not a ZIP"}})));
    PassImporter importer;
    importer.prepare(source);
    QTRY_VERIFY(!importer.busy());
    QVERIFY(!importer.error().isEmpty());
    QVERIFY(importer.passes().isEmpty());
    importer.importPasses();
    QVERIFY(!importer.busy());
    QVERIFY(QDir(library()).entryList({"*.pkpass"}, QDir::Files).isEmpty());
    QVERIFY(write(source, zip({{"a.pkpass", pass("one")}, {"b.pkpass", pass("one", "conflict")}})));
    importer.prepare(source);
    QTRY_VERIFY(!importer.busy());
    QVERIFY(!importer.error().isEmpty());
    QVERIFY(importer.passes().isEmpty());
    importer.prepare("https://example.com/pass.pkpass");
    QTRY_VERIFY(!importer.busy());
    QVERIFY(!importer.error().isEmpty());
}

void PassImporterTest::failedWrite()
{
    QTemporaryDir sources;
    const QString source = sources.filePath("pass.pkpass");
    QVERIFY(write(source, pass("one")));
    PassImporter importer;
    importer.prepare(source);
    QTRY_VERIFY(!importer.busy());
    QVERIFY(importer.error().isEmpty());
    QVERIFY(QDir().mkpath(library()));
    const QString target = importer.passes().first().toMap().value("target").toString();
    QVERIFY(QDir().mkpath(target));
    importer.importPasses();
    QTRY_VERIFY(!importer.busy());
    QVERIFY(!importer.error().isEmpty());
    QCOMPARE(read(source), pass("one"));
}

void PassImporterTest::boundedZipRead()
{
    QTemporaryDir sources;
    const QString file = sources.filePath("test.zip");
    QVERIFY(write(file, zip({{"large", QByteArray(1024, 'x')}}, true)));
    ZipFile archive(file);
    QVERIFY(archive.isValid());
    QVERIFY(archive.getFile("large", 100).isEmpty());
    QCOMPARE(archive.getFile("large"), QByteArray(1024, 'x'));
    QByteArray damaged = zip({{"test", "payload"}});
    damaged[34] = 'X'; // Damage the stored payload without changing the central CRC.
    const QString bad = sources.filePath("bad.zip");
    QVERIFY(write(bad, damaged));
    ZipFile broken(bad);
    QVERIFY(broken.getFile("test").isEmpty());
}

int main(int argc, char **argv)
{
    QTemporaryDir data;
    if (!data.isValid())
        return 1;
    qputenv("XDG_DATA_HOME", data.path().toUtf8());
    QCoreApplication app(argc, argv);
    app.setApplicationName("passviewer-import-test");
    PassImporterTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_passimporter.moc"
