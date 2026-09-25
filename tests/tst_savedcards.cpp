#include <QtTest>
#include <QBuffer>
#include <QDir>
#include <QImage>
#include <QStandardPaths>
#include <QTemporaryDir>
#include "../src/savedcards.h"

class SavedCardsTest : public QObject
{
    Q_OBJECT
private slots:
    void persistAndDelete();
    void invalidInput();
    void iconPersistence();
    void writeFailure();
};

static QVariantMap sample()
{
    QImage image(20, 20, QImage::Format_RGB32);
    image.fill(Qt::white);
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return {{"format", "Code128"}, {"text", "00123456"},
            {"bytes", "MDAxMjM0NTY="}, {"png", QString::fromLatin1(png.toBase64())}};
}

void SavedCardsTest::persistAndDelete()
{
    SavedCards store;
    QSignalSpy changed(&store, &SavedCards::changed);
    const QString id = store.save("  Store card  ", sample());
    QVERIFY2(!id.isEmpty(), qPrintable(store.error()));
    QCOMPARE(changed.count(), 1);
    SavedCards reopened;
    QCOMPARE(reopened.card(id).value("name").toString(), QString("Store card"));
    QCOMPARE(reopened.card(id).value("text").toString(), QString("00123456"));
    QCOMPARE(QByteArray::fromBase64(reopened.card(id).value("bytes").toByteArray()), QByteArray("00123456"));
    QCOMPARE(reopened.passes().size(), 1);
    QCOMPARE(reopened.passes().first().toMap().value("path").toString(), "card:" + id);
    QVERIFY(reopened.card(id).value("image").toString().startsWith("data:image/png;base64,"));
    const QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cards/" + id + ".json";
    const auto permissions = QFile::permissions(path);
    QVERIFY(!(permissions & (QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther)));
    QVERIFY(reopened.remove(id));
    QVERIFY(store.card(id).isEmpty());
    QVERIFY(store.passes().isEmpty());
}

void SavedCardsTest::invalidInput()
{
    SavedCards store;
    QVERIFY(store.save(" ", sample()).isEmpty());
    QVERIFY(!store.error().isEmpty());
    auto broken = sample();
    broken["png"] = "not a png";
    QVERIFY(store.save("Card", broken).isEmpty());
    QVERIFY(store.card("../passes.db").isEmpty());
    QVERIFY(!store.remove("../passes.db"));
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cards";
    QDir().mkpath(dir);
    QFile corrupt(dir + "/12345678-1234-1234-1234-123456789012.json");
    QVERIFY(corrupt.open(QIODevice::WriteOnly));
    corrupt.write("{broken");
    corrupt.close();
    QVERIFY(store.passes().isEmpty());
    QVERIFY(corrupt.remove());
}

void SavedCardsTest::writeFailure()
{
    SavedCards store;
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cards";
    QVERIFY(QDir().rmdir(dir));
    QFile blocker(dir);
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    QVERIFY(store.save("Card", sample()).isEmpty());
    QVERIFY(!store.error().isEmpty());
    QVERIFY(store.passes().isEmpty());
    QVERIFY(blocker.remove());
}

void SavedCardsTest::iconPersistence()
{
    SavedCards store;
    const QString id = store.save("Loyalty card", sample());
    QVERIFY(!id.isEmpty());
    const auto original = store.card(id);
    QImage image(256, 256, QImage::Format_RGB32);
    image.fill(Qt::red);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(image.save(&buffer, "PNG"));
    const QVariantMap icon{{"png", bytes.toBase64()}, {"source", "Iconify"},
        {"id", "mdi:store"}, {"author", "Pictogrammers"}, {"license", "Apache 2.0"},
        {"sourceUrl", "https://icon-sets.iconify.design/mdi/store/"},
        {"licenseUrl", "file:///etc/passwd"}, {"unrelated", "not persisted"}};
    QSignalSpy changed(&store, &SavedCards::changed);
    QVERIFY(store.setIcon(id, icon));
    QCOMPARE(changed.count(), 1);
    SavedCards reopened;
    const auto updated = reopened.card(id);
    for (const QString key : {"name", "format", "text", "bytes", "png", "image"})
        QCOMPARE(updated.value(key), original.value(key));
    QCOMPARE(updated.value("iconImage").toString(), "data:image/png;base64," + QString::fromLatin1(bytes.toBase64()));
    QCOMPARE(reopened.passes().first().toMap().value("iconImage"), updated.value("iconImage"));
    const auto metadata = updated.value("icon").toMap();
    QCOMPARE(metadata.value("author").toString(), QString("Pictogrammers"));
    QVERIFY(!metadata.contains("licenseUrl"));
    QVERIFY(!metadata.contains("unrelated"));
    const QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cards/" + id + ".json";
    QVERIFY(!(QFile::permissions(path) & (QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther)));
    QVERIFY(!store.setIcon(id, {{"png", "broken"}}));
    QVERIFY(!store.setIcon(id, {{"png", sample().value("png")}}));
    QVERIFY(!store.setIcon("../passes.db", icon));
    QCOMPARE(store.card(id), updated);
    QVERIFY(store.setIcon(id, {}));
    QCOMPARE(store.card(id), original);
    QVERIFY(store.remove(id));
}

int main(int argc, char **argv)
{
    QTemporaryDir data;
    if (!data.isValid())
        return 1;
    qputenv("XDG_DATA_HOME", data.path().toUtf8());
    QCoreApplication app(argc, argv);
    app.setApplicationName("passviewer-storage-test");
    SavedCardsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_savedcards.moc"
